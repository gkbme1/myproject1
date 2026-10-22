import { parseDate, roundDates } from './dates';
import { currentCells, getRound } from './schedule';
import type { AppData, CellChange, CellValue, DateStr, Person, PersonId, RuleConfig, RuleSeverity, Round, ShiftType } from './types';
import { cellKey } from './types';

// 排班规则引擎。所有改班操作（手工改班、调休联动、还班）都用 checkChanges 判断
// "这次改动有没有新增违规"：排班表里本来就存在的问题不算在这次改动头上。

export const DEFAULT_RULES: RuleConfig[] = [
  { id: 'consecutive', type: 'maxConsecutiveWork', enabled: true, severity: 'hard', days: 4, exemptNight: true },
  { id: 'restAfterNight', type: 'restAfterNight', enabled: true, severity: 'hard' },
  { id: 'bangyeSunday', type: 'noShiftOnWeekday', enabled: true, severity: 'hard', shift: '帮夜', weekdays: [0] },
  { id: 'limit34567', type: 'roundGroupLimit', enabled: true, severity: 'hard', shifts: ['3-4', '3-5', '3-6', '3-7'], max: 2 },
  { id: 'limitA', type: 'roundGroupLimit', enabled: true, severity: 'soft', shifts: ['A'], max: 3 },
  { id: 'limitT', type: 'roundGroupLimit', enabled: true, severity: 'soft', shifts: ['T'], max: 3 },
  { id: 'nightWeek', type: 'nightWholeWeek', enabled: true, severity: 'soft', shifts: ['夜1', '夜2'] },
  { id: 'restDays', type: 'minRestDays', enabled: true, severity: 'soft', normal: 4, withShift: '帮夜', withShiftDays: 5 },
  { id: 'count3-1', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '3-1', min: 2, max: 2 },
  { id: 'count3-3', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '3-3', min: 2, max: 2 },
  { id: 'count3(10)', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '3(10)', min: 2, max: 2 },
  { id: 'count3', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '3', min: 1, max: 3 },
  { id: 'count2(3)', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '2(3)', min: 0, max: 1 },
  { id: 'count2(4)', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '2(4)', min: 0, max: 1 },
  { id: 'count2(5)', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '2(5)', min: 0, max: 1 },
  { id: 'count2(6)', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '2(6)', min: 0, max: 1 },
  { id: 'count3-2', type: 'dailyHeadcount', enabled: true, severity: 'soft', shift: '3-2', min: 0, max: 1 },
  { id: '3(10)alternate', type: 'noConsecutiveShift', enabled: true, severity: 'soft', shift: '3(10)' },
];

const WEEKDAY_NAMES = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];

export function getRules(data: AppData): RuleConfig[] {
  return data.rules ?? DEFAULT_RULES;
}

export function describeRule(rule: RuleConfig): string {
  switch (rule.type) {
    case 'maxConsecutiveWork':
      return `不能连续上班超过 ${rule.days} 天${rule.exemptNight ? '（夜班、帮夜不计入）' : ''}`;
    case 'restAfterNight':
      return '夜班后第一天必须休息（不能夜倒早，也不能接其他白班）';
    case 'nightWholeWeek':
      return `${rule.shifts.join('、')} 要周一到周日整周连上`;
    case 'noShiftOnWeekday':
      return `${rule.weekdays.map((w) => WEEKDAY_NAMES[w]).join('、')}不能上 ${rule.shift}`;
    case 'roundGroupLimit':
      return `${rule.shifts.join('、')} 每人每轮合计最多 ${rule.max} 个`;
    case 'minRestDays':
      return `每人每轮至少休 ${rule.normal} 天；上 ${rule.withShift} 的人至少休 ${rule.withShiftDays} 天`;
    case 'dailyHeadcount':
      return rule.min === rule.max ? `${rule.shift} 每天 ${rule.min} 人` : `${rule.shift} 每天 ${rule.min}~${rule.max} 人`;
    case 'noConsecutiveShift':
      return `${rule.shift} 上一休一，不能连着两天上`;
  }
}

export interface Violation {
  ruleId: string;
  severity: RuleSeverity;
  personId: PersonId | null; // 每日人数这类规则不针对某个人
  dates: DateStr[]; // 涉及的日期（只含本轮）
  message: string;
  group: string; // 同一个人同一条规则（或同一天同一条规则）算一组，用来比较改动前后
  amount: number; // 超出多少，用来判断改动后是变好了还是变差了
}

export interface RuleContext {
  data: AppData;
  round: Round;
  dates: DateStr[];
  cells: Record<string, CellValue>;
  // 上一轮最后几天的排班，用来判断跨轮的连班、夜班后休息
  prevDates: DateStr[];
  prevCells: Record<string, CellValue>;
  shiftsById: Map<string, ShiftType>;
  people: Person[];
}

const LOOK_BACK_DAYS = 7;

function previousRound(data: AppData, round: Round): Round | null {
  const start = parseDate(round.startDate).getTime();
  const earlier = data.rounds.filter((r) => r.id !== round.id && parseDate(r.startDate).getTime() < start);
  earlier.sort((a, b) => parseDate(b.startDate).getTime() - parseDate(a.startDate).getTime());
  return earlier[0] ?? null;
}

export function buildRuleContext(data: AppData, cells?: Record<string, CellValue>, roundId: string = data.currentRoundId): RuleContext {
  const round = getRound(data, roundId);
  const prev = previousRound(data, round);
  const prevAll = prev ? roundDates(prev.startDate, prev.days) : [];
  const roundPeopleIds = round.personIds ?? data.people.map((p) => p.id);
  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  return {
    data,
    round,
    dates: roundDates(round.startDate, round.days),
    cells: cells ?? currentCells(data, round.id),
    prevDates: prevAll.slice(-LOOK_BACK_DAYS),
    prevCells: prev ? currentCells(data, prev.id) : {},
    shiftsById: new Map(data.shifts.map((s) => [s.id, s])),
    people: roundPeopleIds.map((id) => peopleById.get(id)).filter((p): p is Person => !!p && p.status !== 'frozen'),
  };
}

function shiftOf(ctx: RuleContext, cells: Record<string, CellValue>, personId: string, date: DateStr): ShiftType | null {
  const v = cells[cellKey(personId, date)];
  return v && 'shiftId' in v ? ctx.shiftsById.get(v.shiftId) ?? null : null;
}

export function isWork(s: ShiftType | null): boolean {
  return !!s && s.category !== 'off' && s.category !== 'leave';
}

function isRest(s: ShiftType | null): boolean {
  return !!s && (s.category === 'off' || s.category === 'leave');
}

const norm = (name: string) => name.replace(/\s+/g, '');

function evaluatePerson(ctx: RuleContext, rules: RuleConfig[], person: Person, cells: Record<string, CellValue>, out: Violation[]) {
  const pid = person.id;
  // 上一轮最后几天 + 本轮，统一成一串，方便跨轮判断
  const allDates = [...ctx.prevDates, ...ctx.dates];
  const offset = ctx.prevDates.length;
  const row = allDates.map((d, i) => shiftOf(ctx, i < offset ? ctx.prevCells : cells, pid, d));
  const roundRow = row.slice(offset);
  const inRound = (i: number) => i >= offset;

  for (const rule of rules) {
    if (!rule.enabled) continue;
    const base = { ruleId: rule.id, severity: rule.severity, personId: pid };
    switch (rule.type) {
      case 'maxConsecutiveWork': {
        let start = -1;
        for (let i = 0; i <= row.length; i++) {
          const s = row[i];
          const counts = i < row.length && isWork(s) && !(rule.exemptNight && s!.timeSlot === 'night');
          if (counts) {
            if (start < 0) start = i;
            continue;
          }
          if (start >= 0) {
            const len = i - start;
            if (len > rule.days && inRound(i - 1)) {
              const dates = allDates.slice(Math.max(start, offset), i);
              out.push({ ...base, dates, message: `连续上班 ${len} 天（最多 ${rule.days} 天）`, group: `${rule.id}|${pid}`, amount: len - rule.days });
            }
            start = -1;
          }
        }
        break;
      }
      case 'restAfterNight': {
        for (let i = 0; i < row.length - 1; i++) {
          const s = row[i];
          const next = row[i + 1];
          if (s?.timeSlot === 'night' && isWork(next) && next!.timeSlot !== 'night' && inRound(i + 1)) {
            out.push({ ...base, dates: [allDates[i + 1]], message: `${s.name} 后第一天上了 ${next!.name}，夜班后必须休息`, group: `${rule.id}|${pid}`, amount: 1 });
          }
        }
        break;
      }
      case 'nightWholeWeek': {
        const names = rule.shifts.map(norm);
        // 只看本轮里完整的周一到周日
        for (let i = 0; i + 7 <= ctx.dates.length; i++) {
          if (parseDate(ctx.dates[i]).getDay() !== 1) continue;
          const week = roundRow.slice(i, i + 7);
          const nightDays = week.filter((s) => s && names.includes(norm(s.name))).length;
          if (nightDays > 0 && nightDays < 7) {
            out.push({ ...base, dates: ctx.dates.slice(i, i + 7), message: `这一周夜班只上了 ${nightDays} 天，应周一到周日连上`, group: `${rule.id}|${pid}`, amount: 7 - nightDays });
          }
        }
        break;
      }
      case 'noShiftOnWeekday': {
        roundRow.forEach((s, i) => {
          if (s && norm(s.name) === norm(rule.shift) && rule.weekdays.includes(parseDate(ctx.dates[i]).getDay())) {
            out.push({ ...base, dates: [ctx.dates[i]], message: `${WEEKDAY_NAMES[parseDate(ctx.dates[i]).getDay()]}不能上 ${s.name}`, group: `${rule.id}|${pid}`, amount: 1 });
          }
        });
        break;
      }
      case 'roundGroupLimit': {
        const names = rule.shifts.map(norm);
        const hits = roundRow.map((s, i) => (s && names.includes(norm(s.name)) ? ctx.dates[i] : null)).filter((d): d is string => !!d);
        if (hits.length > rule.max) {
          out.push({ ...base, dates: hits, message: `${rule.shifts.join('、')} 本轮 ${hits.length} 个（最多 ${rule.max} 个）`, group: `${rule.id}|${pid}`, amount: hits.length - rule.max });
        }
        break;
      }
      case 'minRestDays': {
        if (person.status === 'noRotate') break;
        const rest = roundRow.filter(isRest).length;
        const hasShift = roundRow.some((s) => s && norm(s.name) === norm(rule.withShift));
        const need = hasShift ? rule.withShiftDays : rule.normal;
        if (rest < need) {
          out.push({ ...base, dates: [], message: `本轮只休 ${rest} 天（应休 ${need} 天）`, group: `${rule.id}|${pid}`, amount: need - rest });
        }
        break;
      }
      case 'noConsecutiveShift': {
        for (let i = 0; i < row.length - 1; i++) {
          if (row[i] && row[i + 1] && norm(row[i]!.name) === norm(rule.shift) && norm(row[i + 1]!.name) === norm(rule.shift) && inRound(i + 1)) {
            out.push({ ...base, dates: [allDates[i + 1]], message: `${rule.shift} 连着两天上`, group: `${rule.id}|${pid}`, amount: 1 });
          }
        }
        break;
      }
      case 'dailyHeadcount':
        break; // 按天算，见 evaluateDaily
    }
  }
}

function evaluateDaily(ctx: RuleContext, rules: RuleConfig[], dates: DateStr[], cells: Record<string, CellValue>, out: Violation[]) {
  for (const rule of rules) {
    if (!rule.enabled || rule.type !== 'dailyHeadcount') continue;
    for (const d of dates) {
      const count = ctx.people.filter((p) => {
        const s = shiftOf(ctx, cells, p.id, d);
        return s && norm(s.name) === norm(rule.shift);
      }).length;
      if (count < rule.min || count > rule.max) {
        out.push({
          ruleId: rule.id,
          severity: rule.severity,
          personId: null,
          dates: [d],
          message: `${rule.shift} 当天 ${count} 人（应 ${rule.min === rule.max ? rule.min : `${rule.min}~${rule.max}`} 人）`,
          group: `${rule.id}|${d}`,
          amount: count < rule.min ? rule.min - count : count - rule.max,
        });
      }
    }
  }
}

export interface EvaluateScope {
  personIds?: Set<string>;
  dates?: Set<DateStr>;
}

export function evaluate(ctx: RuleContext, cells: Record<string, CellValue> = ctx.cells, scope: EvaluateScope = {}): Violation[] {
  const rules = getRules(ctx.data);
  const out: Violation[] = [];
  for (const person of ctx.people) {
    if (scope.personIds && !scope.personIds.has(person.id)) continue;
    evaluatePerson(ctx, rules, person, cells, out);
  }
  const dates = scope.dates ? ctx.dates.filter((d) => scope.dates!.has(d)) : ctx.dates;
  if (!scope.personIds || scope.dates) evaluateDaily(ctx, rules, dates, cells, out);
  return out;
}

export interface ChangeCheck {
  hard: Violation[]; // 这次改动新增的、要拦截的违规
  soft: Violation[]; // 这次改动新增的、只提示的
}

export function applyChangesToCells(cells: Record<string, CellValue>, changes: CellChange[]): Record<string, CellValue> {
  const next = { ...cells };
  for (const ch of changes) next[cellKey(ch.personId, ch.date)] = ch.to;
  return next;
}

// 只比较受影响的人和日期，改动前后哪一组变差了，就把那一组改动后的违规报出来
export function checkChanges(ctx: RuleContext, changes: CellChange[]): ChangeCheck {
  const scope: EvaluateScope = {
    personIds: new Set(changes.map((c) => c.personId)),
    dates: new Set(changes.map((c) => c.date)),
  };
  const before = evaluate(ctx, ctx.cells, scope);
  const after = evaluate(ctx, applyChangesToCells(ctx.cells, changes), scope);

  const total = (list: Violation[]) => {
    const m = new Map<string, number>();
    for (const v of list) m.set(v.group, (m.get(v.group) ?? 0) + v.amount);
    return m;
  };
  const beforeTotals = total(before);
  const afterTotals = total(after);
  const worse = new Set(Array.from(afterTotals.entries()).filter(([g, amt]) => amt > (beforeTotals.get(g) ?? 0)).map(([g]) => g));
  const added = after.filter((v) => worse.has(v.group));
  return { hard: added.filter((v) => v.severity === 'hard'), soft: added.filter((v) => v.severity === 'soft') };
}

export function violationText(v: Violation, people: Map<string, Person>): string {
  return v.personId ? `${people.get(v.personId)?.name ?? ''}：${v.message}` : v.message;
}
