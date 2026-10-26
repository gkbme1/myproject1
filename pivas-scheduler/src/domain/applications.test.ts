import { describe, expect, it } from 'vitest';
import { newApplication, nextRoundDates, processApplications } from './applications';
import { computeLeaveBalances } from './leaveBalance';
import { generateNextRound } from './rounds';
import { currentCells } from './schedule';
import { createSeedData } from './seed';
import type { AppData } from './types';
import { cellKey } from './types';

function setup() {
  const data: AppData = createSeedData();
  const dates = nextRoundDates(data);
  const { round } = generateNextRound(data, { useModified: true });
  const person = (n: string) => data.people.find((p) => p.name === n)!;
  return { data, dates, round, person };
}

// 按顺序登记，登记时间依次往后，保证“先来后到”
function register(data: AppData, list: [string, string][]) {
  data.leaveApplications = list.map(([pid, date], i) => ({ ...newApplication(pid, date, ''), registeredAt: `2026-10-26T08:00:0${i}.000Z` }));
}

describe('下一轮调休申请表', () => {
  it('按登记先后处理：能安排的写入新一轮并记成修改记录；本来就休息的拒绝并写明原因', () => {
    const { data, dates, round, person } = setup();
    const next = { ...data, rounds: [...data.rounds, round], currentRoundId: round.id };
    const cells = currentCells(next, round.id);
    const shiftName = (pid: string, d: string) => {
      const v = cells[cellKey(pid, d)];
      return v && 'shiftId' in v ? data.shifts.find((s) => s.id === v.shiftId)!.name : '';
    };
    // 找一个当天上普通班的人、和一个当天休息的人
    const day = dates[11];
    const worker = round.personIds!.find((id) => ['2(3)', '2(4)', '2(5)', '3-1', '3-2', '3-3', '3'].includes(shiftName(id, day)))!;
    const rester = round.personIds!.find((id) => shiftName(id, day) === '休')!;
    register(next, [
      [worker, day],
      [rester, day],
    ]);

    const result = processApplications(next, round.id);
    const [a1, a2] = result.data.leaveApplications!;
    expect(a1.status).toBe('approved');
    expect(a1.summary).toBeTruthy();
    expect(a2.status).toBe('rejected');
    expect(a2.reason).toContain('本来就不上班');

    const cs = result.data.changes.find((c) => c.id === a1.changeSetId)!;
    expect(cs.applicationId).toBe(a1.id);
    expect(cs.leave?.personId).toBe(worker);
    // 申请人当天变成调休
    const after = currentCells(result.data, round.id)[cellKey(worker, day)];
    expect(after && 'shiftId' in after && data.shifts.find((s) => s.id === after.shiftId)!.name).toBe('调休');
    expect(person('张敏')).toBeTruthy();
  });

  it('同一天先登记的人用掉的顶班人，后登记的人不能再用', () => {
    const { data, dates, round } = setup();
    const next = { ...data, rounds: [...data.rounds, round], currentRoundId: round.id };
    const cells = currentCells(next, round.id);
    const day = dates[11];
    const workers = round.personIds!.filter((id) => {
      const v = cells[cellKey(id, day)];
      const s = v && 'shiftId' in v ? data.shifts.find((x) => x.id === v.shiftId) : null;
      return s && s.category === 'normal' && s.canTakeLeave && !s.needReturn;
    });
    register(next, [
      [workers[0], day],
      [workers[1], day],
    ]);
    const result = processApplications(next, round.id);
    const [a1, a2] = result.data.leaveApplications!;
    expect(a1.status).toBe('approved');
    const first = result.data.changes.find((c) => c.id === a1.changeSetId)!;
    const used = new Set(first.changes.map((c) => c.personId));
    if (a2.status === 'approved') {
      const second = result.data.changes.find((c) => c.id === a2.changeSetId)!;
      for (const c of second.changes.slice(1)) expect(used.has(c.personId)).toBe(false);
    }
  });

  it('剩余假期：不还班的调休扣 1 天，撤销后不扣', () => {
    const { data, dates, round } = setup();
    const next = { ...data, rounds: [...data.rounds, round], currentRoundId: round.id };
    const cells = currentCells(next, round.id);
    const day = dates[11];
    const worker = round.personIds!.find((id) => {
      const v = cells[cellKey(id, day)];
      const s = v && 'shiftId' in v ? data.shifts.find((x) => x.id === v.shiftId) : null;
      return s && s.category === 'normal' && s.canTakeLeave && !s.needReturn;
    })!;
    register(next, [[worker, day]]);
    next.leaveInitial = { [worker]: 5 };
    const result = processApplications(next, round.id);
    expect(result.approved).toBe(1);
    let b = computeLeaveBalances(result.data).get(worker)!;
    expect(b.initial).toBe(5);
    expect(b.deductions).toHaveLength(1);
    expect(b.deductions[0].source).toBe('auto');
    expect(b.current).toBe(4);

    const undone = { ...result.data, changes: result.data.changes.map((c) => ({ ...c, undone: true })) };
    b = computeLeaveBalances(undone).get(worker)!;
    expect(b.current).toBe(5);
  });

  it('需要还班的调休（3-4/3-5/3-7）不扣假期', () => {
    const data = createSeedData();
    data.changes.push({
      id: 'c1',
      roundId: data.currentRoundId,
      kind: 'leave',
      title: '',
      note: '',
      at: '',
      operator: '',
      undone: false,
      changes: [],
      leave: { personId: 'p1', date: '2026-10-21', leaveShiftId: 's35', fromShiftId: 's14', coverPersonId: 'p2', returnStatus: 'done', returnDate: '2026-10-25' },
    });
    expect(computeLeaveBalances(data).get('p1')!.deductions).toHaveLength(0);
  });
});
