import type { AppData, CellChange, CellRole, CellValue, ChangeSet, DateStr, PersonId, Round, ShiftType } from './types';
import { cellKey } from './types';

export function getRound(data: AppData, roundId: string = data.currentRoundId): Round {
  const round = data.rounds.find((r) => r.id === roundId);
  if (!round) throw new Error(`找不到排班轮次 ${roundId}`);
  return round;
}

export function roundChanges(data: AppData, roundId: string): ChangeSet[] {
  return data.changes.filter((c) => c.roundId === roundId);
}

// 当前排班 = 基准排班 + 依次应用本轮所有未撤销的修改
export function currentCells(data: AppData, roundId: string = data.currentRoundId): Record<string, CellValue> {
  const cells: Record<string, CellValue> = { ...getRound(data, roundId).base };
  for (const cs of roundChanges(data, roundId)) {
    if (cs.undone) continue;
    for (const ch of cs.changes) {
      cells[cellKey(ch.personId, ch.date)] = ch.to;
    }
  }
  return cells;
}

export interface CellHistoryEntry {
  value: CellValue;
  label: string; // 原始排班 / 手工改班 / 联动：xxx
  at: string | null;
  changeSetId: string | null;
}

// 某个格子在本轮的完整修改链（只算未撤销的修改）
export function cellHistory(data: AppData, roundId: string, personId: PersonId, date: DateStr): CellHistoryEntry[] {
  const key = cellKey(personId, date);
  const entries: CellHistoryEntry[] = [
    { value: getRound(data, roundId).base[key] ?? null, label: '原始排班', at: null, changeSetId: null },
  ];
  for (const cs of roundChanges(data, roundId)) {
    if (cs.undone) continue;
    for (const ch of cs.changes) {
      if (ch.personId === personId && ch.date === date) {
        entries.push({ value: ch.to, label: cs.title, at: cs.at, changeSetId: cs.id });
      }
    }
  }
  return entries;
}

// 格子最近一次被改时的角色，决定显示蓝字(调休申请人)还是红字(被联动改班)
export function cellLastRole(data: AppData, roundId: string): Record<string, CellRole> {
  const roles: Record<string, CellRole> = {};
  for (const cs of roundChanges(data, roundId)) {
    if (cs.undone) continue;
    for (const ch of cs.changes) {
      roles[cellKey(ch.personId, ch.date)] = ch.role;
    }
  }
  return roles;
}

export function lastAppliedIndex(changes: ChangeSet[]): number {
  for (let i = changes.length - 1; i >= 0; i--) {
    if (!changes[i].undone) return i;
  }
  return -1;
}

// 可重做的：最后一条生效修改之后、最早被撤销的那一条
export function redoCandidateIndex(changes: ChangeSet[]): number {
  const last = lastAppliedIndex(changes);
  for (let i = last + 1; i < changes.length; i++) {
    if (changes[i].undone) return i;
  }
  return -1;
}

export function describeCell(value: CellValue, shiftsById: Map<string, ShiftType>): string {
  if (value === null) return '';
  if ('custom' in value) return value.custom;
  return value.text || (shiftsById.get(value.shiftId)?.name ?? '?');
}

let idCounter = 0;
export function newId(prefix: string): string {
  idCounter++;
  return `${prefix}${Date.now().toString(36)}${idCounter.toString(36)}`;
}

export interface ChangeExtra {
  note?: string;
  operator?: string;
  leave?: ChangeSet['leave'];
  warnings?: string[];
}

export function makeChangeSet(roundId: string, kind: ChangeSet['kind'], title: string, changes: CellChange[], extra: ChangeExtra = {}): ChangeSet {
  return {
    id: newId('c'),
    roundId,
    kind,
    title,
    note: extra.note ?? '',
    at: new Date().toISOString(),
    operator: extra.operator || '管理员',
    changes,
    undone: false,
    ...(extra.leave ? { leave: extra.leave } : {}),
    ...(extra.warnings && extra.warnings.length > 0 ? { warnings: extra.warnings } : {}),
  };
}
