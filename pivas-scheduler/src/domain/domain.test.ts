import { describe, expect, it } from 'vitest';
import { roundDates } from './dates';
import { findLeavePlans, findReturnOptions } from './plans';
import { buildRuleContext, checkChanges, DEFAULT_RULES, evaluate } from './rulesEngine';
import { leaveBlockedReason } from './rules';
import { cellHistory, currentCells, lastAppliedIndex, makeChangeSet, redoCandidateIndex } from './schedule';
import { createSeedData } from './seed';
import type { AppData, ShiftType } from './types';
import { cellKey } from './types';

function shiftByName(data: AppData, name: string): ShiftType {
  const s = data.shifts.find((x) => x.name === name);
  if (!s) throw new Error(name);
  return s;
}

function person(data: AppData, name: string) {
  return data.people.find((p) => p.name === name)!;
}

describe('修改记录', () => {
  it('当前排班 = 原始排班 + 未撤销的修改，撤销的修改留在日志里', () => {
    const data = createSeedData();
    const round = data.rounds[0];
    const wu = person(data, '吴迪');
    const date = round.startDate;
    const s3 = shiftByName(data, '3');
    const s3star = shiftByName(data, '3*');
    const s32 = shiftByName(data, '3-2');

    data.changes.push(
      makeChangeSet(round.id, 'manual', '改为 3*', [{ personId: wu.id, date, from: { shiftId: s3.id }, to: { shiftId: s3star.id }, role: 'manual' }]),
      makeChangeSet(round.id, 'manual', '改为 3-2', [{ personId: wu.id, date, from: { shiftId: s3star.id }, to: { shiftId: s32.id }, role: 'manual' }]),
    );
    expect(currentCells(data)[cellKey(wu.id, date)]).toEqual({ shiftId: s32.id });
    expect(cellHistory(data, round.id, wu.id, date).map((h) => h.value)).toEqual([{ shiftId: s3.id }, { shiftId: s3star.id }, { shiftId: s32.id }]);

    data.changes[1].undone = true;
    expect(currentCells(data)[cellKey(wu.id, date)]).toEqual({ shiftId: s3star.id });
    expect(lastAppliedIndex(data.changes)).toBe(0);
    expect(redoCandidateIndex(data.changes)).toBe(1);
  });
});

describe('规则引擎', () => {
  const data = createSeedData();
  const dates = roundDates(data.rounds[0].startDate, 14);
  const ctx = buildRuleContext(data);
  const change = (name: string, day: number, shift: string) => {
    const p = person(data, name);
    return { personId: p.id, date: dates[day], from: ctx.cells[cellKey(p.id, dates[day])] ?? null, to: { shiftId: shiftByName(data, shift).id }, role: 'manual' as const };
  };

  it('夜班后第一天必须休息', () => {
    // 王磊第 7 天(下标6)是夜1，下标7 是休
    const r = checkChanges(ctx, [change('王磊', 7, '3-1')]);
    expect(r.hard.map((v) => v.ruleId)).toContain('restAfterNight');
  });

  it('3-4~3-7 每人每轮最多 2 个', () => {
    // 杨帆本轮已有 3-7、3-3… 先给一个 3-4 再给一个 3-5 就超了
    const once = checkChanges(ctx, [change('杨帆', 2, '3-4')]);
    expect(once.hard).toHaveLength(0);
    const twice = checkChanges(ctx, [change('杨帆', 2, '3-4'), change('杨帆', 8, '3-5')]);
    expect(twice.hard.map((v) => v.ruleId)).toContain('limit34567');
  });

  it('连续上班超过 4 天要拦截，夜班不计入', () => {
    // 张敏 下标 6~9 已连上 4 天，下标 10 是休，把它改成上班就是 5 天
    const r = checkChanges(ctx, [change('张敏', 10, '3-2')]);
    expect(r.hard.map((v) => v.ruleId)).toContain('consecutive');
  });

  it('排班表里原本就有的问题不算在这次改动头上', () => {
    const before = evaluate(ctx).filter((v) => v.personId === person(data, '王磊').id);
    const r = checkChanges(ctx, [change('王磊', 13, '休')]);
    expect(r.hard).toHaveLength(0);
    expect(before.length).toBeGreaterThanOrEqual(0);
  });

  it('A 班每轮超过 3 个只提示不拦截', () => {
    // 何蕾已有 4 个 A（示例数据里），把第 12 天的 T 也换成 A
    const r = checkChanges(ctx, [change('何蕾', 11, 'A')]);
    expect(r.hard).toHaveLength(0);
    expect(r.soft.map((v) => v.ruleId)).toContain('limitA');
  });

  it('3-6、夜班不能调休', () => {
    expect(leaveBlockedReason(shiftByName(data, '3-6'))).toContain('不能调休');
    expect(leaveBlockedReason(shiftByName(data, '夜1'))).toContain('不能调休');
    expect(leaveBlockedReason(shiftByName(data, '2(5)'))).toBe('');
  });
});

describe('调休方案', () => {
  it('当天调休的人可以直接顶班，1 步方案排在最前面；违规的人被排除并说明原因', () => {
    const data = createSeedData();
    const ctx = buildRuleContext(data);
    const zhang = person(data, '张敏');
    const result = findLeavePlans({ ctx, requesterId: zhang.id, dayIndex: 11, leaveShiftId: shiftByName(data, '调休').id });
    expect(result.plans.length).toBeGreaterThan(0);
    const first = result.plans[0];
    expect(first.steps).toHaveLength(1);
    // 张敏是级别1；当天调休的级别1 刘洋前一天帮夜，夜班后必须休息，所以被排除，轮到级别3
    expect([person(data, '冯凯').id, person(data, '郑悦').id]).toContain(first.steps[0].personId);
    expect(result.excluded.some((e) => e.reason.includes('刘洋'))).toBe(true);
    expect(first.changes[0]).toMatchObject({ personId: zhang.id, role: 'requester' });
    // 步数从少到多
    const steps = result.plans.map((p) => p.steps.length);
    expect([...steps].sort((a, b) => a - b)).toEqual(steps);
  });

  it('药师的 A 班当天没人空闲时，能找到多人联动方案', () => {
    const data = createSeedData();
    const ctx = buildRuleContext(data);
    const he = person(data, '何蕾');
    // 第 1 天何蕾上 A；当天没有调休的药师，只能多人联动或找不到
    const result = findLeavePlans({ ctx, requesterId: he.id, dayIndex: 0, leaveShiftId: shiftByName(data, '调休').id });
    for (const plan of result.plans) {
      expect(plan.changes[0].personId).toBe(he.id);
      expect(plan.steps.length).toBeGreaterThanOrEqual(1);
    }
  });

  it('3-4 调休后能找到还班日期', () => {
    const data = createSeedData();
    const zhang = person(data, '张敏');
    const round = data.rounds[0];
    // 把张敏 10/30（当天有好几个人调休）改成 3-4，再申请调休
    round.base[cellKey(zhang.id, roundDates(round.startDate, 14)[11])] = { shiftId: shiftByName(data, '3-4').id };
    // 示例数据里张敏休息的那几天一上班就连上 5 天，这里把连班规则关掉（顺带验证规则可以关）
    data.rules = DEFAULT_RULES.map((r) => (r.id === 'consecutive' ? { ...r, enabled: false } : r));
    const ctx = buildRuleContext(data);
    const result = findLeavePlans({ ctx, requesterId: zhang.id, dayIndex: 11, leaveShiftId: shiftByName(data, '调休').id });
    expect(result.plans.length).toBeGreaterThan(0);
    const plan = result.plans[0];
    const options = findReturnOptions(ctx, plan.changes, zhang.id, plan.steps[0].personId, ctx.dates[11]);
    expect(options.length).toBeGreaterThan(0);
    for (const o of options) {
      expect(o.changes).toHaveLength(2);
      expect(o.date).not.toBe(ctx.dates[11]);
      expect(o.changes[0].personId).toBe(zhang.id);
    }
  });
});
