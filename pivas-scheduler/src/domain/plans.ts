import { isWorkShift, checkShiftForCell } from './rules';
import type { AppData, CellChange, CellValue, DateStr, Person, PersonId, ShiftId, ShiftType } from './types';
import { cellKey } from './types';

// 初版调休方案查找：只做 1 步（当天调休/空白格的人直接顶班）和 2 步（中间换一个人）。
// 正式版会换成按"步数 → 级别 → 班次类型"打分的通用搜索，并接入完整规则引擎。

export interface PlanStep {
  personId: PersonId;
  fromLabel: string; // 这个人原来当天的情况：当天调休 / 3-1 ...
  takesShiftId: ShiftId;
  takesFromPersonId: PersonId;
}

export interface LeavePlan {
  steps: PlanStep[];
  changes: CellChange[];
}

export interface ExcludedCandidate {
  personId: PersonId;
  reason: string;
}

export interface PlanSearchResult {
  plans: LeavePlan[];
  excluded: ExcludedCandidate[];
}

// 找人顶班时优先找哪些级别（需求：级别3 > 级别1 > 级别2；级别4 > 级别2）
const LEVEL_PRIORITY: Record<string, string[]> = {
  L1: ['L1', 'L3', 'L2', 'L4'],
  L2: ['L2', 'L4', 'L3', 'L1'],
  L3: ['L3', 'L1', 'L2', 'L4'],
  L4: ['L4', 'L2', 'L3', 'L1'],
};

const MAX_PLANS = 20;
const MAX_EXCLUDED = 8;

export function findLeavePlans(
  data: AppData,
  cells: Record<string, CellValue>,
  dates: DateStr[],
  requesterId: PersonId,
  dayIndex: number,
  leaveShiftId: ShiftId,
): PlanSearchResult {
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const date = dates[dayIndex];
  const shiftOf = (personId: PersonId, d: DateStr): ShiftType | null => {
    const v = cells[cellKey(personId, d)];
    return v && 'shiftId' in v ? shiftsById.get(v.shiftId) ?? null : null;
  };
  const rowOf = (personId: PersonId) => dates.map((d) => shiftOf(personId, d));

  const requester = data.people.find((p) => p.id === requesterId)!;
  const target = shiftOf(requesterId, date);
  if (!target) return { plans: [], excluded: [] };

  const priority = LEVEL_PRIORITY[requester.levelId] ?? [];
  const levelRank = (p: Person) => {
    const i = priority.indexOf(p.levelId);
    return i < 0 ? priority.length : i;
  };

  const others = data.people.filter((p) => p.id !== requesterId && p.status === 'active');
  // 当天调休/补休，或者是删除班次留下的空白格，这些人可以被拉出来顶班
  const freePeople = others.filter((p) => {
    const v = cells[cellKey(p.id, date)];
    if (v === null || v === undefined) return true;
    const s = 'shiftId' in v ? shiftsById.get(v.shiftId) : null;
    return s?.category === 'leave';
  });
  // 当天在上班、而且班次允许被换走的人，可以做中间环节
  const workingPeople = others.filter((p) => {
    const s = shiftOf(p.id, date);
    return isWorkShift(s) && s!.canTakeLeave && !s!.needReturn && s!.id !== target.id;
  });

  const freeLabel = (p: Person) => {
    const v = cells[cellKey(p.id, date)];
    if (!v) return '空白格';
    return '当天' + (shiftOf(p.id, date)?.name ?? '调休');
  };
  const check = (p: Person, s: ShiftType) => {
    const row = rowOf(p.id);
    row[dayIndex] = s;
    return checkShiftForCell(p, row, dayIndex, s);
  };

  const plans: (LeavePlan & { rank: number[] })[] = [];
  const excluded: ExcludedCandidate[] = [];
  const exclude = (personId: PersonId, reason: string) => {
    if (excluded.length < MAX_EXCLUDED && !excluded.some((e) => e.personId === personId)) {
      excluded.push({ personId, reason });
    }
  };
  const requesterChange: CellChange = {
    personId: requesterId,
    date,
    from: { shiftId: target.id },
    to: { shiftId: leaveShiftId },
    role: 'requester',
  };

  for (const f of freePeople) {
    const why = check(f, target);
    if (why) {
      exclude(f.id, `${f.name} 接 ${target.name}：${why}`);
      continue;
    }
    plans.push({
      rank: [1, levelRank(f), 0],
      steps: [{ personId: f.id, fromLabel: freeLabel(f), takesShiftId: target.id, takesFromPersonId: requesterId }],
      changes: [
        requesterChange,
        { personId: f.id, date, from: cells[cellKey(f.id, date)] ?? null, to: { shiftId: target.id }, role: 'cover' },
      ],
    });
  }

  for (const w of workingPeople) {
    const own = shiftOf(w.id, date)!;
    const whyW = check(w, target);
    if (whyW) {
      exclude(w.id, `${w.name} 接 ${target.name}：${whyW}`);
      continue;
    }
    for (const f of freePeople) {
      if (check(f, own)) continue;
      plans.push({
        rank: [2, levelRank(f), own.category === 'normal' ? 0 : 1, levelRank(w)],
        steps: [
          { personId: w.id, fromLabel: own.name, takesShiftId: target.id, takesFromPersonId: requesterId },
          { personId: f.id, fromLabel: freeLabel(f), takesShiftId: own.id, takesFromPersonId: w.id },
        ],
        changes: [
          requesterChange,
          { personId: w.id, date, from: { shiftId: own.id }, to: { shiftId: target.id }, role: 'cover' },
          { personId: f.id, date, from: cells[cellKey(f.id, date)] ?? null, to: { shiftId: own.id }, role: 'cover' },
        ],
      });
    }
  }

  plans.sort((a, b) => {
    for (let i = 0; i < Math.max(a.rank.length, b.rank.length); i++) {
      const d = (a.rank[i] ?? 0) - (b.rank[i] ?? 0);
      if (d !== 0) return d;
    }
    return 0;
  });

  return { plans: plans.slice(0, MAX_PLANS).map(({ steps, changes }) => ({ steps, changes })), excluded };
}
