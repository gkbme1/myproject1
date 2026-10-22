import { roundDates } from '../domain/dates';
import { newId } from '../domain/schedule';
import { createDefaultLevels, createDefaultShifts, DEFAULT_PERSON_RULES } from '../domain/seed';
import type { AppData, CellValue, DateStr, LevelId, Person, Round, ShiftType } from '../domain/types';
import { cellKey } from '../domain/types';
import type { DraftPerson, ImportDraft } from './parseSchedule';

// 人名颜色 -> 级别 的默认对应（现有模板：黑色技术员、红色高级技术员、紫色第三方）。
// 药师的名字也是黑色，用"只上 A/B/C/D/T"来区分。
export const DEFAULT_COLOR_LEVELS: Record<string, LevelId> = {
  '#000000': 'L1',
  '#FF0000': 'L2',
  '#A0327A': 'L3',
};

export const COLOR_NAMES: Record<string, string> = {
  '#000000': '黑色',
  '#FF0000': '红色',
  '#A0327A': '紫色',
};

export function colorName(color: string): string {
  return COLOR_NAMES[color] ?? color;
}

// 节日补休：表格里常写成"补五一""补端午"，也有"补五孙""补端毛"这种带人名的写法
const HOLIDAY_LEAVE: Record<string, string> = {
  五: '补五一',
  端: '补端午',
  元: '补元旦',
  国: '补国庆',
  中: '补中秋',
  清: '补清明',
  春: '补春节',
};

export function normalizeText(text: string): string {
  return text
    .trim()
    .replace(/[—–－]/g, '-')
    .replace(/（/g, '(')
    .replace(/）/g, ')')
    .replace(/＊/g, '*')
    .replace(/\s+/g, '');
}

export type Classified = { kind: 'shift'; shiftName: string; keepText: boolean } | { kind: 'custom' } | { kind: 'empty' };

// 判断格子里写的是哪个班次。认不出来的原样保留成自定义内容
export function classifyText(text: string, shiftNames: Set<string>): Classified {
  const norm = normalizeText(text);
  if (!norm) return { kind: 'empty' };
  if (shiftNames.has(norm)) return { kind: 'shift', shiftName: norm, keepText: false };
  if (norm.startsWith('调休')) return { kind: 'shift', shiftName: '调休', keepText: true };
  if (norm.startsWith('补休')) return { kind: 'shift', shiftName: '补休', keepText: true };
  if (norm.startsWith('补') && HOLIDAY_LEAVE[norm.charAt(1)]) {
    return { kind: 'shift', shiftName: HOLIDAY_LEAVE[norm.charAt(1)], keepText: true };
  }
  return { kind: 'custom' };
}

export interface LevelChoice {
  colorLevels: Record<string, LevelId>; // 每种颜色归到哪个级别
  overrides: Record<string, LevelId>; // 按姓名单独指定
}

const PHARMACIST_SHIFTS = new Set(['A', 'B', 'C', 'D', 'T']);

// 黑色名字里，所有上班的格子都是 A/B/C/D/T 的人，按药师处理
export function looksLikePharmacist(person: DraftPerson, shiftNames: Set<string>): boolean {
  let workDays = 0;
  for (const t of person.texts) {
    const c = classifyText(t, shiftNames);
    if (c.kind !== 'shift') continue;
    if (c.shiftName === '休' || c.shiftName === '调休' || c.shiftName === '补休' || c.shiftName.startsWith('补')) continue;
    if (!PHARMACIST_SHIFTS.has(c.shiftName)) return false;
    workDays++;
  }
  return workDays > 0;
}

export function suggestLevelChoice(draft: ImportDraft): LevelChoice {
  const colorLevels: Record<string, LevelId> = {};
  for (const p of draft.people) {
    if (!(p.color in colorLevels)) colorLevels[p.color] = DEFAULT_COLOR_LEVELS[p.color] ?? 'L1';
  }
  return { colorLevels, overrides: {} };
}

export function resolveLevel(person: DraftPerson, choice: LevelChoice, shiftNames: Set<string>): LevelId {
  if (choice.overrides[person.name]) return choice.overrides[person.name];
  const byColor = choice.colorLevels[person.color] ?? 'L1';
  if (byColor === 'L1' && looksLikePharmacist(person, shiftNames)) return 'L4';
  return byColor;
}

export function shiftNameSet(shifts: ShiftType[]): Set<string> {
  return new Set(shifts.map((s) => normalizeText(s.name)));
}

export interface ImportSummary {
  unknownTexts: { text: string; count: number }[];
  newLeaveShifts: string[];
}

export function summarizeDraft(draft: ImportDraft, shifts: ShiftType[]): ImportSummary {
  const names = shiftNameSet(shifts);
  const unknown = new Map<string, number>();
  const newLeave = new Set<string>();
  for (const p of draft.people) {
    for (const t of p.texts) {
      const c = classifyText(t, names);
      if (c.kind === 'custom') unknown.set(t, (unknown.get(t) ?? 0) + 1);
      if (c.kind === 'shift' && !names.has(c.shiftName)) newLeave.add(c.shiftName);
    }
  }
  return {
    unknownTexts: Array.from(unknown.entries())
      .map(([text, count]) => ({ text, count }))
      .sort((a, b) => b.count - a.count),
    newLeaveShifts: Array.from(newLeave),
  };
}

export interface ImportOptions {
  fileName: string;
  startDate: DateStr;
  choice: LevelChoice;
}

// 用导入的表生成新的一轮排班。已有的人员按姓名对上，保留他们的特例设置；
// 以前各轮的修改记录都留着，只是当前轮换成导入的这一轮。
export function applyImport(existing: AppData | null, draft: ImportDraft, options: ImportOptions): AppData {
  const levels = existing?.levels ?? createDefaultLevels();
  const shifts = [...(existing?.shifts ?? createDefaultShifts())];
  const names = shiftNameSet(shifts);

  // 先补上表里用到、但还没有的节日补休班次
  for (const name of summarizeDraft(draft, shifts).newLeaveShifts) {
    shifts.push({
      id: newId('s'),
      name,
      category: 'leave',
      timeSlot: 'none',
      canTakeLeave: false,
      needReturn: false,
      allowedLevels: levels.map((l) => l.id),
      order: Math.max(0, ...shifts.map((s) => s.order)) + 1,
      builtIn: false,
    });
    names.add(name);
  }
  const shiftIdByName = new Map(shifts.map((s) => [normalizeText(s.name), s.id]));

  const people: Person[] = existing ? existing.people.map((p) => ({ ...p })) : [];
  const byName = new Map(people.map((p) => [p.name, p]));
  const personIds: string[] = [];

  draft.people.forEach((dp, i) => {
    const levelId = resolveLevel(dp, options.choice, names);
    let person = byName.get(dp.name);
    if (person) {
      person.levelId = levelId;
      person.order = i + 1;
    } else {
      const rule = DEFAULT_PERSON_RULES[dp.name];
      const idsOf = (list?: string[]) => (list ?? []).map((n) => shiftIdByName.get(n)).filter((x): x is string => !!x);
      person = {
        id: newId('p'),
        name: dp.name,
        levelId,
        order: i + 1,
        status: rule?.status ?? 'active',
        onlyShifts: idsOf(rule?.only),
        bannedShifts: idsOf(rule?.banned),
        note: rule?.note ?? '',
      };
      people.push(person);
      byName.set(dp.name, person);
    }
    personIds.push(person.id);
  });

  const dates = roundDates(options.startDate, draft.days);
  const base: Record<string, CellValue> = {};
  draft.people.forEach((dp) => {
    const personId = byName.get(dp.name)!.id;
    dp.texts.forEach((text, d) => {
      const c = classifyText(text, names);
      let value: CellValue = null;
      if (c.kind === 'shift') {
        value = { shiftId: shiftIdByName.get(c.shiftName)! };
        if (c.keepText && normalizeText(text) !== c.shiftName) value.text = text.trim();
      } else if (c.kind === 'custom') {
        value = { custom: text.trim() };
      }
      base[cellKey(personId, dates[d])] = value;
    });
  });

  const round: Round = {
    id: newId('r'),
    index: Math.max(0, ...(existing?.rounds ?? []).map((r) => r.index)) + 1,
    startDate: options.startDate,
    days: draft.days,
    base,
    personIds,
    source: { fileName: options.fileName, title: draft.title, importedAt: new Date().toISOString() },
  };

  return {
    schemaVersion: 1,
    levels,
    shifts,
    people,
    rounds: [...(existing?.rounds ?? []), round],
    currentRoundId: round.id,
    changes: existing?.changes ?? [],
    lastActiveAt: existing?.lastActiveAt ?? null,
  };
}
