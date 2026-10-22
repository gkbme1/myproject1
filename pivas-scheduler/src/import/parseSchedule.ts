import { addDays, formatDate, parseDate } from '../domain/dates';
import type { DateStr } from '../domain/types';
import type { RawSheet } from './readXlsx';

// 把工作表解析成"谁、哪天、写的什么"。现有模板的样子：
//   第 1 行  标题：PIVAS技术员排班表(编号:8.17)
//   第 2 行  人员\日期 | 8.17 | 8.18 | …
//   第 3 行           | 一（14）| 二（15）| …   ← 星期，括号里是当天人数
//   第 4 行起 姓名 | 班次 | 班次 | …           ← 姓名的字体颜色区分级别

export interface DraftPerson {
  row: number; // Excel 行号，从 1 开始
  name: string;
  color: string;
  texts: string[]; // 每天格子里的原文
}

export interface ImportDraft {
  title: string;
  startDate: DateStr;
  days: number;
  people: DraftPerson[];
  warnings: string[];
}

const WEEKDAY_CHARS = ['日', '一', '二', '三', '四', '五', '六'];

interface MonthDay {
  month: number;
  day: number;
}

// "8.17" "8月17日" "2026/8/17" "2026-08-17" 都认
function parseMonthDay(text: string): MonthDay | null {
  const t = text.trim();
  let m = t.match(/^(\d{4})[./-](\d{1,2})[./-](\d{1,2})/);
  if (m) return { month: Number(m[2]), day: Number(m[3]) };
  m = t.match(/^(\d{1,2})\s*[.月/-]\s*(\d{1,2})\s*日?$/);
  if (m) return { month: Number(m[1]), day: Number(m[2]) };
  return null;
}

// 表头只写了月.日，年份按星期行去对：取星期对得上、离今天最近的那一年
function inferStartDate(first: MonthDay, weekdayText: string, today: Date): DateStr {
  const weekdayIndex = WEEKDAY_CHARS.indexOf(weekdayText.trim().charAt(0));
  const year = today.getFullYear();
  const candidates = [year - 1, year, year + 1]
    .map((y) => new Date(y, first.month - 1, first.day))
    .filter((d) => d.getMonth() === first.month - 1 && d.getDate() === first.day);
  const matching = weekdayIndex >= 0 ? candidates.filter((d) => d.getDay() === weekdayIndex) : [];
  const pool = matching.length > 0 ? matching : candidates;
  pool.sort((a, b) => Math.abs(a.getTime() - today.getTime()) - Math.abs(b.getTime() - today.getTime()));
  return formatDate(pool[0] ?? new Date(year, first.month - 1, first.day));
}

export function parseScheduleSheet(sheet: RawSheet, today: Date = new Date()): ImportDraft {
  const rows = sheet.rows;
  const warnings: string[] = [];

  const headerIndex = rows.findIndex((r) => /人员|姓名|日期/.test(r[0]?.text ?? '') && r.slice(1).some((c) => parseMonthDay(c.text)));
  if (headerIndex < 0) {
    throw new Error('没有找到表头：第一列应该有“人员\\日期”，后面是 8.17 这样的日期');
  }
  const header = rows[headerIndex];

  // 日期列：从第 2 列开始，连续能认出日期的列
  const dateCols: number[] = [];
  for (let c = 1; c < header.length; c++) {
    if (parseMonthDay(header[c].text)) dateCols.push(c);
    else if (dateCols.length > 0) break;
  }
  const first = parseMonthDay(header[dateCols[0]].text)!;

  const weekdayRow = rows[headerIndex + 1];
  const hasWeekdayRow = !!weekdayRow && WEEKDAY_CHARS.includes((weekdayRow[dateCols[0]]?.text ?? '').trim().charAt(0));
  const startDate = inferStartDate(first, hasWeekdayRow ? weekdayRow[dateCols[0]].text : '', today);

  // 表头日期应该是一天接一天的，不连续就提示一下
  dateCols.forEach((c, i) => {
    const md = parseMonthDay(header[c].text);
    const expected = parseDate(addDays(startDate, i));
    if (md && (md.month !== expected.getMonth() + 1 || md.day !== expected.getDate())) {
      warnings.push(`表头第 ${i + 1} 天写的是 ${header[c].text}，按连续日期应该是 ${expected.getMonth() + 1}.${expected.getDate()}，已按连续日期导入`);
    }
  });

  const people: DraftPerson[] = [];
  const seen = new Set<string>();
  for (let r = headerIndex + (hasWeekdayRow ? 2 : 1); r < rows.length; r++) {
    const name = (rows[r][0]?.text ?? '').trim();
    if (!name) continue;
    if (seen.has(name)) {
      warnings.push(`第 ${r + 1} 行的 ${name} 重名，只导入了第一行`);
      continue;
    }
    seen.add(name);
    people.push({
      row: r + 1,
      name,
      color: rows[r][0].color,
      texts: dateCols.map((c) => (rows[r][c]?.text ?? '').trim()),
    });
  }
  if (people.length === 0) throw new Error('表头下面没有人员');

  const title = rows.slice(0, headerIndex).map((r) => r.find((c) => c.text.trim())?.text.trim() ?? '').find(Boolean) ?? sheet.name;

  return { title, startDate, days: dateCols.length, people, warnings };
}
