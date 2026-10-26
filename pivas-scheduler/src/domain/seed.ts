import { roundDates } from './dates';
import type { AppData, CellValue, Level, Person, Round, ShiftCategory, ShiftType, TimeSlot } from './types';
import { cellKey } from './types';

// 第一次打开软件、还没有数据文件时用的示例数据。
// 班次权限按现有 MFC 版本的权限表来（需求文档里有矛盾的地方，先按代码逻辑）。

const LEVELS: Level[] = [
  { id: 'L1', name: '技术员', shortName: '级别1', order: 1, nameColor: '#1B2430' },
  { id: 'L2', name: '高级技术员', shortName: '级别2', order: 2, nameColor: '#D92D20' },
  { id: 'L3', name: '第三方', shortName: '级别3', order: 3, nameColor: '#A0327A' },
  { id: 'L4', name: '药师', shortName: '级别4', order: 4, nameColor: '#1B2430' },
];

// [名称, 分类, 时段, 可调休, 需还班, 可上的级别]
type ShiftRow = [string, ShiftCategory, TimeSlot, boolean, boolean, string];

const SHIFT_ROWS: ShiftRow[] = [
  ['2(1)', 'special', 'noon', true, false, 'L1 L2'],
  ['2(2)', 'special', 'noon', true, false, 'L1 L2'],
  ['2(3)', 'normal', 'noon', true, false, 'L1 L2 L3'],
  ['2(4)', 'normal', 'noon', true, false, 'L1 L2 L3'],
  ['2(5)', 'normal', 'noon', true, false, 'L1 L2 L3'],
  ['2(6)', 'normal', 'noon', true, false, 'L1 L2 L3'],
  ['3', 'normal', 'morning', true, false, 'L1 L2 L3'],
  ['3(10)', 'fixedGroup', 'morning', false, false, 'L1 L2'],
  ['3*', 'special', 'morning', true, false, 'L1 L2 L3'],
  ['3*-1', 'special', 'morning', true, false, 'L1 L2 L3'],
  ['3-1', 'normal', 'morning', true, false, 'L1 L2 L3'],
  ['3-2', 'normal', 'morning', true, false, 'L1 L2 L3'],
  ['3-3', 'normal', 'morning', true, false, 'L1 L2 L3'],
  ['3-4', 'normal', 'morning', true, true, 'L1 L2 L3'],
  ['3-5', 'normal', 'morning', true, true, 'L1 L2 L3'],
  ['3-6', 'normal', 'noon', false, false, 'L1 L2 L3'],
  ['3-7', 'normal', 'morning', true, true, 'L1 L2 L3'],
  ['4-1', 'normal', 'noon', true, false, 'L1 L2'],
  ['4-2', 'normal', 'noon', true, false, 'L1 L2'],
  ['4-3', 'normal', 'noon', true, false, 'L1 L2'],
  ['4-4', 'exclusive', 'noon', true, false, 'L2'],
  ['Z', 'exclusive', 'noon', true, false, 'L2'],
  ['A', 'pharmacist', 'noon', true, false, 'L4'],
  ['B', 'pharmacist', 'noon', true, false, 'L2 L4'],
  ['C', 'pharmacist', 'noon', true, false, 'L2 L4'],
  ['D', 'pharmacist', 'noon', true, false, 'L2 L4'],
  ['T', 'pharmacist', 'noon', true, false, 'L4'],
  ['帮夜', 'night', 'night', false, false, 'L1 L3'],
  ['夜1', 'night', 'night', false, false, 'L1 L2'],
  ['夜2', 'night', 'night', false, false, 'L1'],
  ['QA', 'custom', 'noon', true, false, 'L2'],
  ['Z菌', 'custom', 'noon', true, false, 'L2'],
  ['Z破', 'custom', 'noon', true, false, 'L2'],
  ['休', 'off', 'none', false, false, 'L1 L2 L3 L4'],
  ['调休', 'leave', 'none', false, false, 'L1 L2 L3 L4'],
  ['补休', 'leave', 'none', false, false, 'L1 L2 L3 L4'],
  ['补元旦', 'leave', 'none', false, false, 'L1 L2 L3 L4'],
  ['补五一', 'leave', 'none', false, false, 'L1 L2 L3 L4'],
  ['补端午', 'leave', 'none', false, false, 'L1 L2 L3 L4'],
];

export function createDefaultLevels(): Level[] {
  return LEVELS.map((l) => ({ ...l }));
}

export function createDefaultShifts(): ShiftType[] {
  return buildShifts();
}

// 需求里点名的人员特例；导入时第一次出现这些人就按这里设置，之后在人员页里改
export const DEFAULT_PERSON_RULES: Record<string, { status: Person['status']; only?: string[]; banned?: string[]; note: string }> = {
  方海利: { status: 'noRotate', note: '锁行' },
  代安然: { status: 'noRotate', banned: ['T'], note: '锁行，不上 T' },
};

function buildShifts(): ShiftType[] {
  return SHIFT_ROWS.map(([name, category, timeSlot, canTakeLeave, needReturn, levels], i) => ({
    id: 's' + (i + 1),
    name,
    category,
    timeSlot,
    canTakeLeave,
    needReturn,
    allowedLevels: levels.split(' '),
    order: i + 1,
    builtIn: category !== 'custom' && !/^补(?!休)/.test(name), // 补元旦、补五一这类节日补休可以删
  }));
}

// [姓名, 级别, 状态, 14 天排班(空格分隔)]
const PEOPLE_ROWS: [string, string, Person['status'], string, string?][] = [
  ['张敏', 'L1', 'active', '2(3) 3-1 3-4 3 休 休 2(4) 3-1 3-3 2(5) 休 2(5) 3 3-2'],
  ['王磊', 'L1', 'active', '夜1 夜1 夜1 夜1 夜1 夜1 夜1 休 3-3 3 2(6) 3-1 休 休'],
  ['刘洋', 'L1', 'active', '3-3 3 休 2(4) 3-5 3-1 休 帮夜 休 帮夜 帮夜 调休 3-2 3'],
  ['陈静', 'L1', 'active', '3(10) 休 3(10) 休 3(10) 休 3(10) 休 3(10) 休 3(10) 休 3(10) 休'],
  ['杨帆', 'L1', 'active', '2(5) 3-7 3 3-2 休 休 3-1 2(3) 3 休 3-3 3-3 2(4) 休'],
  ['赵琳', 'L2', 'active', 'Z 4-4 2(1) 休 休 Z 2(2) 3* 4-4 休 Z 2(1) 休 2(1)'],
  ['孙浩', 'L2', 'active', '4-4 Z 休 2(1) 2(2) 休 Z 4-4 休 2(1) 4-4 休 Z 2(2)'],
  ['方海利', 'L2', 'noRotate', 'QA QA QA QA QA 休 休 QA QA QA QA QA 休 休', '锁行'],
  ['吴迪', 'L3', 'active', '3 2(6) 3-2 休 3-3 3 休 3*-1 3 2(3) 休 3-1 3-5 休'],
  ['郑悦', 'L3', 'active', '帮夜 休 帮夜 帮夜 休 休 3 3-2 3*-1 休 2(4) 调休 3-3 休'],
  ['冯凯', 'L3', 'active', '3-1 3*-1 休 3-3 2(6) 3-2 休 3 3* 3-1 休 调休 3 休'],
  ['何蕾', 'L4', 'active', 'A B 休 C A 休 休 A D 休 B T 休 A'],
  ['许诺', 'L4', 'active', 'B A T 休 D C 休 C A B 休 D T 休'],
  ['代安然', 'L4', 'noRotate', 'A 休 休 A 休 休 休 休 A 休 休 休 休 休', '锁行，不上 T'],
];

export function createSeedData(): AppData {
  const shifts = buildShifts();
  const shiftByName = new Map(shifts.map((s) => [s.name, s.id]));

  const people: Person[] = PEOPLE_ROWS.map(([name, levelId, status, , note], i) => ({
    id: 'p' + (i + 1),
    name,
    levelId,
    order: i + 1,
    status,
    onlyShifts: name === '代安然' ? [shiftByName.get('A')!] : [],
    bannedShifts: name === '代安然' ? [shiftByName.get('T')!] : [],
    note: note ?? '',
  }));

  const startDate = '2026-10-19';
  const dates = roundDates(startDate, 14);
  const base: Record<string, CellValue> = {};
  PEOPLE_ROWS.forEach(([, , , schedule], i) => {
    schedule.split(' ').forEach((name, d) => {
      const shiftId = shiftByName.get(name);
      base[cellKey(people[i].id, dates[d])] = shiftId ? { shiftId } : { custom: name };
    });
  });

  const round: Round = { id: 'r43', index: 43, startDate, days: 14, base };

  return {
    schemaVersion: 1,
    levels: LEVELS,
    shifts,
    people,
    rounds: [round],
    currentRoundId: round.id,
    changes: [],
    lastActiveAt: null,
  };
}
