import { describe, expect, it } from 'vitest';
import { roundDates } from './dates';
import { findLeavePlans } from './plans';
import { checkShiftForCell, leaveBlockedReason } from './rules';
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

describe('格子菜单里的规则', () => {
  const data = createSeedData();
  const dates = roundDates(data.rounds[0].startDate, 14);
  const cells = currentCells(data);
  const rowOf = (personId: string) =>
    dates.map((d) => {
      const v = cells[cellKey(personId, d)];
      return v && 'shiftId' in v ? data.shifts.find((s) => s.id === v.shiftId)! : null;
    });

  it('级别不能上的班次不能选', () => {
    const p = person(data, '张敏');
    const row = rowOf(p.id);
    expect(checkShiftForCell(p, row, 0, shiftByName(data, 'A'))).toBe('级别不能上这个班');
  });

  it('夜班后第一天必须休息', () => {
    const p = person(data, '王磊'); // 第 7 天(下标6)是夜1，下标7 是休
    const row = rowOf(p.id);
    expect(checkShiftForCell(p, row, 7, shiftByName(data, '3-1'))).toContain('夜班后第一天必须休息');
  });

  it('3-4~3-7 每人每轮最多 2 个', () => {
    const p = person(data, '杨帆'); // 已有 3-7
    const row = rowOf(p.id);
    row[2] = shiftByName(data, '3-4');
    expect(checkShiftForCell(p, row, 8, shiftByName(data, '3-5'))).toContain('3-4~3-7');
  });

  it('3-6、夜班不能调休', () => {
    expect(leaveBlockedReason(shiftByName(data, '3-6'))).toContain('不能调休');
    expect(leaveBlockedReason(shiftByName(data, '夜1'))).toContain('不能调休');
    expect(leaveBlockedReason(shiftByName(data, '2(5)'))).toBe('');
  });
});

describe('调休方案', () => {
  it('当天调休的人可以直接顶班，1 步方案排在最前面', () => {
    const data = createSeedData();
    const dates = roundDates(data.rounds[0].startDate, 14);
    const zhang = person(data, '张敏');
    const result = findLeavePlans(data, currentCells(data), dates, zhang.id, 11, shiftByName(data, '调休').id);
    expect(result.plans.length).toBeGreaterThan(0);
    const first = result.plans[0];
    expect(first.steps).toHaveLength(1);
    // 张敏是级别1，按"级别1 > 级别3 > 级别2"优先找；当天调休的级别1 是刘洋，但他前一天帮夜，所以排除，轮到级别3
    expect(person(data, '冯凯').id === first.steps[0].personId || person(data, '郑悦').id === first.steps[0].personId).toBe(true);
    expect(result.excluded.some((e) => e.reason.includes('刘洋'))).toBe(true);
    expect(first.changes[0]).toMatchObject({ personId: zhang.id, role: 'requester' });
  });
});
