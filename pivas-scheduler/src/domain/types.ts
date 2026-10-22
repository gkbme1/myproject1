// 排班系统的核心数据结构。
// 原则：人员、班次都用 id 引用，不用表格行号；日期用真实日期(YYYY-MM-DD)，不用"第几天"。
// 所有对排班的改动都记成 ChangeSet，当前排班 = 基准排班 + 依次应用未撤销的 ChangeSet。

export type LevelId = string;
export type PersonId = string;
export type ShiftId = string;
export type DateStr = string; // YYYY-MM-DD

export interface Level {
  id: LevelId;
  name: string; // 技术员 / 高级技术员 / 第三方 / 药师
  shortName: string; // 级别1 ...
  order: number;
  nameColor?: string; // 排班表里姓名的颜色，和原来 Excel 里的习惯一致
}

// 班次分类，决定底色和联动时的优先级
export type ShiftCategory =
  | 'normal' // 普通班
  | 'special' // 特殊班
  | 'exclusive' // 特定班（只有一个级别能上）
  | 'pharmacist' // 药师班 A-T
  | 'fixedGroup' // 3(10) 这类固定小组轮换
  | 'night' // 夜班 / 帮夜
  | 'off' // 休
  | 'leave' // 调休 / 补休 / 补元旦 等
  | 'custom'; // 自定义班次（QA、Z菌…）

export type TimeSlot = 'morning' | 'noon' | 'night' | 'none';

export interface ShiftType {
  id: ShiftId;
  name: string;
  category: ShiftCategory;
  timeSlot: TimeSlot;
  canTakeLeave: boolean; // 这个班能不能申请调休
  needReturn: boolean; // 调休后需要还班（3-4/3-5/3-7）
  allowedLevels: LevelId[]; // 哪些级别能上
  order: number;
  builtIn: boolean; // 系统自带的班次不能删除，只能改属性
}

export type PersonStatus = 'active' | 'noRotate' | 'frozen';

export interface Person {
  id: PersonId;
  name: string;
  levelId: LevelId;
  order: number; // 在排班表里的顺序
  status: PersonStatus;
  onlyShifts: ShiftId[]; // 只允许上的班次，空 = 按级别权限
  bannedShifts: ShiftId[]; // 禁止上的班次
  note: string;
}

// 一个格子的内容：某个班次，或者自定义文字，或者空白（删除班次后留下的可接班空位）。
// text：导入时表格里的原文和班次名不一样时保留原文（如"调休张"按调休算，界面仍显示"调休张"）
export type CellValue = { shiftId: ShiftId; text?: string } | { custom: string } | null;

export interface Round {
  id: string;
  index: number; // 第几轮
  startDate: DateStr;
  days: number; // 一轮 14 天
  // 导入或生成时的原始排班，key = cellKey(personId, date)
  base: Record<string, CellValue>;
  // 这一轮排班表里的人员和顺序（导入时按 Excel 的行序）；没有时按人员列表显示
  personIds?: PersonId[];
  source?: RoundSource;
}

export interface RoundSource {
  fileName: string;
  title: string;
  importedAt: string;
}

export type ChangeKind =
  | 'manual' // 手工改班
  | 'leave' // 调休联动
  | 'return' // 还班
  | 'delete' // 删除班次
  | 'custom' // 填写自定义内容
  | 'import'; // 导入

// 联动里每个人的角色，用来决定显示颜色：申请人蓝字，被联动改班的人红字
export type CellRole = 'requester' | 'cover' | 'manual';

export interface CellChange {
  personId: PersonId;
  date: DateStr;
  from: CellValue;
  to: CellValue;
  role: CellRole;
}

export interface ChangeSet {
  id: string;
  roundId: string;
  kind: ChangeKind;
  title: string; // 例：张敏 10/30 调休（方案 1，1 步）
  note: string; // 用户填写的备注
  at: string; // ISO 时间
  operator: string;
  changes: CellChange[];
  undone: boolean; // 被撤销后保留在日志里，界面上显示为"已撤销"
  undoneAt?: string;
  leave?: LeaveInfo; // 调休类修改的附加信息，供"调休申请记录"汇总
  warnings?: string[]; // 执行时被确认放行的软规则提示
}

export interface LeaveInfo {
  personId: PersonId;
  date: DateStr;
  leaveShiftId: ShiftId;
  fromShiftId: ShiftId;
  coverPersonId: PersonId | null; // 直接接手这个班的人
  // 还班：3-4/3-5/3-7 调休后要还的班。done=已在本条修改里安排；pending=还没安排
  returnStatus: 'none' | 'done' | 'pending';
  returnDate?: DateStr;
}

// ---------- 排班规则（可在“规则”页里开关、改参数、设拦截或只提示） ----------
export type RuleSeverity = 'hard' | 'soft'; // hard=违规就拦截；soft=只提示

export type RuleConfig =
  | { id: string; type: 'maxConsecutiveWork'; enabled: boolean; severity: RuleSeverity; days: number; exemptNight: boolean }
  | { id: string; type: 'restAfterNight'; enabled: boolean; severity: RuleSeverity }
  | { id: string; type: 'nightWholeWeek'; enabled: boolean; severity: RuleSeverity; shifts: string[] }
  | { id: string; type: 'noShiftOnWeekday'; enabled: boolean; severity: RuleSeverity; shift: string; weekdays: number[] }
  | { id: string; type: 'roundGroupLimit'; enabled: boolean; severity: RuleSeverity; shifts: string[]; max: number }
  | { id: string; type: 'minRestDays'; enabled: boolean; severity: RuleSeverity; normal: number; withShift: string; withShiftDays: number }
  | { id: string; type: 'dailyHeadcount'; enabled: boolean; severity: RuleSeverity; shift: string; min: number; max: number }
  | { id: string; type: 'noConsecutiveShift'; enabled: boolean; severity: RuleSeverity; shift: string };

export interface AppSettings {
  operator: string; // 记在修改记录里的操作人
}

export interface AppData {
  schemaVersion: 1;
  levels: Level[];
  shifts: ShiftType[];
  people: Person[];
  rounds: Round[];
  currentRoundId: string;
  changes: ChangeSet[];
  lastActiveAt: string | null; // 上次使用时间，用于"欢迎回来"
  rules?: RuleConfig[]; // 没有时用默认规则
  settings?: AppSettings;
}

export function cellKey(personId: PersonId, date: DateStr): string {
  return `${personId}|${date}`;
}

export function sameCell(a: CellValue, b: CellValue): boolean {
  if (a === null || b === null) return a === b;
  if ('shiftId' in a) return 'shiftId' in b && a.shiftId === b.shiftId && (a.text ?? '') === (b.text ?? '');
  return 'custom' in b && a.custom === b.custom;
}
