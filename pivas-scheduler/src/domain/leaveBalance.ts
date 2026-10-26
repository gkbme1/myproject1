import { currentCells } from './schedule';
import type { AppData, CellValue, DateStr, PersonId } from './types';
import { cellKey } from './types';

// 剩余假期：初始天数手工维护；每次“不需要还班”的调休扣 1 天。
// 3-4 / 3-5 / 3-7 这类调休后要还班的，不扣假期。被撤销的调休不算。

export interface Deduction {
  changeSetId: string;
  date: DateStr;
  roundIndex: number;
  typeName: string; // 调休 / 补休 / 补国庆…
  fromShiftName: string; // 让出来的班
  source: 'auto' | 'manual'; // 申请表自动安排 / 手工调休
  at: string;
}

export interface LeaveBalance {
  initial: number;
  deductions: Deduction[];
  current: number;
}

export function computeLeaveBalances(data: AppData): Map<PersonId, LeaveBalance> {
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const roundIndex = new Map(data.rounds.map((r) => [r.id, r.index]));
  const cellsByRound = new Map<string, Record<string, CellValue>>();
  const cellsOf = (roundId: string) => {
    if (!cellsByRound.has(roundId)) cellsByRound.set(roundId, currentCells(data, roundId));
    return cellsByRound.get(roundId)!;
  };

  const result = new Map<PersonId, LeaveBalance>();
  const get = (pid: PersonId) => {
    if (!result.has(pid)) result.set(pid, { initial: data.leaveInitial?.[pid] ?? 0, deductions: [], current: 0 });
    return result.get(pid)!;
  };
  data.people.forEach((p) => get(p.id));

  for (const cs of data.changes) {
    if (cs.undone || !cs.leave || cs.leave.returnStatus !== 'none') continue;
    const { personId, date } = cs.leave;
    // 类型以格子现在的内容为准（排班人员可能后来把“调休”改成了“补国庆”）
    const now = data.rounds.some((r) => r.id === cs.roundId) ? cellsOf(cs.roundId)[cellKey(personId, date)] : null;
    const nowShift = now && 'shiftId' in now ? shiftsById.get(now.shiftId) : null;
    const typeName = nowShift?.category === 'leave' ? nowShift.name : shiftsById.get(cs.leave.leaveShiftId)?.name ?? '调休';
    get(personId).deductions.push({
      changeSetId: cs.id,
      date,
      roundIndex: roundIndex.get(cs.roundId) ?? 0,
      typeName,
      fromShiftName: shiftsById.get(cs.leave.fromShiftId)?.name ?? '',
      source: cs.applicationId ? 'auto' : 'manual',
      at: cs.at,
    });
  }
  for (const b of result.values()) {
    b.deductions.sort((x, y) => x.date.localeCompare(y.date));
    b.current = b.initial - b.deductions.length;
  }
  return result;
}
