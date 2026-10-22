import { addDays, parseDate, roundDates } from './dates';
import { currentCells, getRound, newId } from './schedule';
import type { AppData, CellValue, Person, PersonId, Round } from './types';
import { cellKey } from './types';

// 生成下一轮：“班不动，人滚动”。
// 每个级别内，排班表的每一行（班次规律）保持不动，人往下挪一行，最后一个人挪到第一行；
// 不轮班、冻结的人留在原来那一行不动（和原来软件的“锁定”一样）。
// 调休、补休、节日补休是个人的，不跟着行走：这些格子在新一轮变成空白格，由排班的人再安排。

export interface NextRoundOptions {
  useModified: boolean; // true=用本轮改过之后的排班当模板；false=用导入时的原始排班
}

export interface RotationMove {
  personId: PersonId;
  fromRowOf: PersonId; // 这个人新一轮用的是谁原来那一行的班
}

export interface NextRoundResult {
  round: Round;
  moves: RotationMove[];
  clearedLeaveCells: number; // 变成空白格的调休 / 补休格子数
}

export function sortedRounds(data: AppData): Round[] {
  return [...data.rounds].sort((a, b) => parseDate(a.startDate).getTime() - parseDate(b.startDate).getTime());
}

// 一个级别内的轮转：能轮转的人往下挪一位，锁定的人留在原位
export function rotateWithLocks<T>(list: T[], isLocked: (item: T) => boolean): T[] {
  const movable = list.filter((x) => !isLocked(x));
  if (movable.length <= 1) return [...list];
  const rotated = [movable[movable.length - 1], ...movable.slice(0, -1)];
  let k = 0;
  return list.map((x) => (isLocked(x) ? x : rotated[k++]));
}

export function generateNextRound(data: AppData, options: NextRoundOptions): NextRoundResult {
  const current = getRound(data);
  const template = options.useModified ? currentCells(data, current.id) : current.base;
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  const order: Person[] = (current.personIds ?? [...data.people].sort((a, b) => a.order - b.order).map((p) => p.id))
    .map((id) => peopleById.get(id))
    .filter((p): p is Person => !!p);

  const isLocked = (p: Person) => p.status !== 'active';
  const newOrder: Person[] = [...order];
  const moves: RotationMove[] = [];
  for (const level of data.levels) {
    const positions = order.map((p, i) => (p.levelId === level.id ? i : -1)).filter((i) => i >= 0);
    const people = positions.map((i) => order[i]);
    const rotated = rotateWithLocks(people, isLocked);
    positions.forEach((pos, k) => {
      newOrder[pos] = rotated[k];
      if (rotated[k].id !== people[k].id) moves.push({ personId: rotated[k].id, fromRowOf: people[k].id });
    });
  }

  const oldDates = roundDates(current.startDate, current.days);
  const startDate = addDays(current.startDate, current.days);
  const newDates = roundDates(startDate, current.days);
  const base: Record<string, CellValue> = {};
  let clearedLeaveCells = 0;

  order.forEach((rowOwner, pos) => {
    const newPerson = newOrder[pos];
    oldDates.forEach((d, i) => {
      let v: CellValue = template[cellKey(rowOwner.id, d)] ?? null;
      if (v && 'shiftId' in v) {
        const s = shiftsById.get(v.shiftId);
        if (s?.category === 'leave') {
          v = null;
          clearedLeaveCells++;
        } else {
          v = { shiftId: v.shiftId }; // “调休张”这类个人备注不带到下一轮
        }
      } else if (v && 'custom' in v) {
        v = null;
        clearedLeaveCells++;
      }
      base[cellKey(newPerson.id, newDates[i])] = v;
    });
  });

  const round: Round = {
    id: newId('r'),
    index: Math.max(...data.rounds.map((r) => r.index)) + 1,
    startDate,
    days: current.days,
    base,
    personIds: newOrder.map((p) => p.id),
    source: {
      fileName: '',
      title: `由第 ${current.index} 轮滚动生成（${options.useModified ? '用修改后的排班' : '用原始排班'}）`,
      importedAt: new Date().toISOString(),
    },
  };
  return { round, moves, clearedLeaveCells };
}
