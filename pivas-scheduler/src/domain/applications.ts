import { addDays, roundDates, shortDate } from './dates';
import { findLeavePlans, findReturnOptions } from './plans';
import { leaveBlockedReason } from './rules';
import { buildRuleContext } from './rulesEngine';
import { makeChangeSet, newId } from './schedule';
import { sortedRounds } from './rounds';
import type { AppData, DateStr, LeaveApplication, PersonId } from './types';
import { cellKey } from './types';

// 下一轮调休申请表：先登记，生成下一轮时按登记时间先后逐条自动安排。
// 能安排的自动完成顶班联动并写入新一轮（记成修改记录，可撤销）；安排不了的拒绝并写明原因，不影响其他申请。

export const AUTO_OPERATOR = '自动安排（调休申请表）';

// 下一轮的日期范围：最新一轮之后的 14 天
export function nextRoundDates(data: AppData): DateStr[] {
  const rounds = sortedRounds(data);
  const last = rounds[rounds.length - 1];
  return roundDates(addDays(last.startDate, last.days), last.days);
}

export function newApplication(personId: PersonId, date: DateStr, note: string): LeaveApplication {
  return { id: newId('a'), personId, date, registeredAt: new Date().toISOString(), note, status: 'pending' };
}

export interface ProcessResult {
  data: AppData;
  approved: number;
  rejected: number;
}

export function processApplications(data: AppData, roundId: string): ProcessResult {
  const round = data.rounds.find((r) => r.id === roundId);
  if (!round) return { data, approved: 0, rejected: 0 };
  const dates = roundDates(round.startDate, round.days);
  const leaveDefault = data.shifts.find((s) => s.category === 'leave' && s.name === '调休') ?? data.shifts.find((s) => s.category === 'leave');
  const nameOf = (id: string) => data.people.find((p) => p.id === id)?.name ?? '?';
  const inRound = new Set(round.personIds ?? data.people.map((p) => p.id));

  let cur = data;
  let approved = 0;
  let rejected = 0;
  const results = new Map<string, Partial<LeaveApplication>>();
  const queue = (data.leaveApplications ?? [])
    .filter((a) => a.status === 'pending' && dates.includes(a.date))
    .sort((a, b) => a.registeredAt.localeCompare(b.registeredAt));

  const reject = (a: LeaveApplication, reason: string) => {
    results.set(a.id, { status: 'rejected', reason, processedAt: new Date().toISOString() });
    rejected++;
  };

  for (const app of queue) {
    const person = cur.people.find((p) => p.id === app.personId);
    if (!person || !inRound.has(app.personId)) {
      reject(app, '这个人不在这一轮的排班表里');
      continue;
    }
    if (person.status === 'frozen') {
      reject(app, '这个人已冻结，暂时不排班');
      continue;
    }
    const leaveShiftId = app.leaveShiftId ?? leaveDefault?.id;
    if (!leaveShiftId) {
      reject(app, '没有“调休”类班次，请先在班次页添加');
      continue;
    }

    const ctx = buildRuleContext(cur, undefined, roundId);
    const dayIndex = ctx.dates.indexOf(app.date);
    const value = ctx.cells[cellKey(app.personId, app.date)];
    const target = value && 'shiftId' in value ? ctx.shiftsById.get(value.shiftId) ?? null : null;
    const blocked = leaveBlockedReason(target);
    if (blocked) {
      reject(app, `${shortDate(app.date)} ${target?.name ?? '空白'}：${blocked}`);
      continue;
    }

    const result = findLeavePlans({ ctx, requesterId: app.personId, dayIndex, leaveShiftId });
    if (result.plans.length === 0) {
      const why = result.excluded.slice(0, 2).map((e) => e.reason).join('；');
      reject(app, `当天没有人能顶 ${target!.name}${why ? '（' + why + '）' : ''}`);
      continue;
    }

    // 3-4/3-5/3-7 需还班：选第一个能找到还班日期的方案（方案已按步数从少到多排好）
    let plan = result.plans[0];
    let returnOption = null as ReturnType<typeof findReturnOptions>[number] | null;
    if (target!.needReturn) {
      const found = result.plans
        .map((p) => ({ p, options: findReturnOptions(ctx, p.changes, app.personId, p.steps[0].personId, app.date) }))
        .find((x) => x.options.length > 0);
      if (!found) {
        reject(app, `${target!.name} 调休后需要还班，本轮找不到可行的还班日期`);
        continue;
      }
      plan = found.p;
      returnOption = found.options[0];
    }

    const steps = plan.steps
      .map((s) => `${nameOf(s.personId)}（${s.fromLabel}）接${nameOf(s.takesFromPersonId)}的 ${ctx.shiftsById.get(s.takesShiftId)?.name}`)
      .join('，');
    const summary = steps + (returnOption ? `；${shortDate(returnOption.date)} 还班` : '');
    let title = `${person.name} ${shortDate(app.date)} 调休（申请表自动安排，${plan.steps.length} 步）`;
    if (returnOption) title += `，${shortDate(returnOption.date)} 还班`;

    const cs = makeChangeSet(roundId, 'leave', title, [...plan.changes, ...(returnOption?.changes ?? [])], {
      note: app.note,
      operator: AUTO_OPERATOR,
      warnings: [...plan.warnings, ...(returnOption?.warnings ?? [])],
      leave: {
        personId: app.personId,
        date: app.date,
        leaveShiftId,
        fromShiftId: target!.id,
        coverPersonId: plan.steps[0].personId,
        returnStatus: target!.needReturn ? 'done' : 'none',
        ...(returnOption ? { returnDate: returnOption.date } : {}),
      },
    });
    cs.applicationId = app.id;
    cur = { ...cur, changes: [...cur.changes, cs] };
    results.set(app.id, { status: 'approved', summary, changeSetId: cs.id, leaveShiftId, processedAt: new Date().toISOString() });
    approved++;
  }

  const leaveApplications = (cur.leaveApplications ?? []).map((a) => (results.has(a.id) ? { ...a, ...results.get(a.id) } : a));
  return { data: { ...cur, leaveApplications }, approved, rejected };
}
