import type { Person, ShiftType } from './types';

// 级别权限和人员特例：能不能上某个班。其余排班规则见 rulesEngine.ts

export function canPersonWork(person: Person, shift: ShiftType): string {
  if (!shift.allowedLevels.includes(person.levelId)) return '级别不能上这个班';
  if (person.onlyShifts.length > 0 && !person.onlyShifts.includes(shift.id)) return '人员特例：只允许上指定班次';
  if (person.bannedShifts.includes(shift.id)) return '人员特例：禁止上这个班';
  return '';
}

export function leaveBlockedReason(current: ShiftType | null): string {
  if (!current) return '这一天没有班次';
  if (current.category === 'off' || current.category === 'leave') return '这一天本来就不上班';
  if (!current.canTakeLeave) return `${current.name} 不能调休（可以先删除班次再手工安排）`;
  return '';
}
