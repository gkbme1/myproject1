import type { Person, ShiftType } from './types';

// 初版规则：只覆盖格子菜单里判断"哪些班次能选"要用的几条。
// 完整的规则引擎（可配置的硬/软规则、跨轮次回看、每日人数等）后面单独做。

const LIMITED_FOUR = ['3-4', '3-5', '3-6', '3-7'];
const MAX_CONSECUTIVE_WORK_DAYS = 4;

export function isWorkShift(shift: ShiftType | null): boolean {
  return !!shift && shift.category !== 'off' && shift.category !== 'leave';
}

export function canPersonWork(person: Person, shift: ShiftType): string {
  if (!shift.allowedLevels.includes(person.levelId)) return '级别不能上这个班';
  if (person.onlyShifts.length > 0 && !person.onlyShifts.includes(shift.id)) return '人员特例：只允许上指定班次';
  if (person.bannedShifts.includes(shift.id)) return '人员特例：禁止上这个班';
  return '';
}

// row：这个人本轮每一天的班次（没有班次的格子为 null），dayIndex：要改的那一天
// 返回不能选的原因，空字符串表示可以选
export function checkShiftForCell(person: Person, row: (ShiftType | null)[], dayIndex: number, target: ShiftType): string {
  const levelWhy = canPersonWork(person, target);
  if (levelWhy) return levelWhy;
  if (!isWorkShift(target)) return '';

  const prev = dayIndex > 0 ? row[dayIndex - 1] : null;
  const next = dayIndex < row.length - 1 ? row[dayIndex + 1] : null;

  if (target.timeSlot === 'night' && isWorkShift(next)) return '次日要上班，夜班后第一天必须休息';
  if (prev?.timeSlot === 'night' && isWorkShift(target)) return '前一天是夜班，夜班后第一天必须休息';

  if (LIMITED_FOUR.includes(target.name)) {
    const count = row.filter((s, i) => i !== dayIndex && s && LIMITED_FOUR.includes(s.name)).length;
    if (count >= 2) return `3-4~3-7 本轮已有 ${count} 个`;
  }

  // 夜班按"周一到周日连上"的规矩排，不计入连班限制
  if (target.timeSlot !== 'night') {
    let streak = 1;
    for (let i = dayIndex - 1; i >= 0 && isWorkShift(row[i]) && row[i]!.timeSlot !== 'night'; i--) streak++;
    for (let i = dayIndex + 1; i < row.length && isWorkShift(row[i]) && row[i]!.timeSlot !== 'night'; i++) streak++;
    if (streak > MAX_CONSECUTIVE_WORK_DAYS) return `会连续上班 ${streak} 天`;
  }

  return '';
}

export function leaveBlockedReason(current: ShiftType | null): string {
  if (!current) return '这一天没有班次';
  if (!isWorkShift(current)) return '这一天本来就不上班';
  if (!current.canTakeLeave) return `${current.name} 不能调休（可以先删除班次再手工安排）`;
  return '';
}
