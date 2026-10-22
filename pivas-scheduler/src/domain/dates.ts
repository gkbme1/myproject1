import type { DateStr } from './types';

const WEEKDAY_NAMES = ['日', '一', '二', '三', '四', '五', '六'];

function pad2(n: number): string {
  return String(n).padStart(2, '0');
}

// 只按本地日期计算，不经过 UTC，避免时区把日期算偏一天
export function parseDate(date: DateStr): Date {
  const [y, m, d] = date.split('-').map(Number);
  return new Date(y, m - 1, d);
}

export function formatDate(d: Date): DateStr {
  return `${d.getFullYear()}-${pad2(d.getMonth() + 1)}-${pad2(d.getDate())}`;
}

export function addDays(date: DateStr, days: number): DateStr {
  const d = parseDate(date);
  d.setDate(d.getDate() + days);
  return formatDate(d);
}

export function roundDates(startDate: DateStr, days: number): DateStr[] {
  return Array.from({ length: days }, (_, i) => addDays(startDate, i));
}

export function weekdayName(date: DateStr): string {
  return '周' + WEEKDAY_NAMES[parseDate(date).getDay()];
}

export function isWeekend(date: DateStr): boolean {
  const day = parseDate(date).getDay();
  return day === 0 || day === 6;
}

export function shortDate(date: DateStr): string {
  const d = parseDate(date);
  return `${d.getMonth() + 1}/${d.getDate()}`;
}

export function formatDateTime(iso: string): string {
  const d = new Date(iso);
  return `${pad2(d.getMonth() + 1)}-${pad2(d.getDate())} ${pad2(d.getHours())}:${pad2(d.getMinutes())}`;
}

export function formatTime(iso: string): string {
  const d = new Date(iso);
  return `${pad2(d.getHours())}:${pad2(d.getMinutes())}:${pad2(d.getSeconds())}`;
}
