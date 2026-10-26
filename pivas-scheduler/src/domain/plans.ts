import { canPersonWork } from './rules';
import type { RuleContext, Violation } from './rulesEngine';
import { applyChangesToCells, checkChanges, isWork } from './rulesEngine';
import type { CellChange, CellValue, DateStr, Person, PersonId, ShiftId, ShiftType } from './types';
import { cellKey } from './types';

// 调休联动方案查找。
// 申请人 A 让出当天的班 S → 有人接 S；接班的人如果本来在上班，他让出的班再找人接……
// 直到最后由当天调休 / 补休 / 空白格的人接上，链条闭合。
// 排序：步数少的优先 → 级别优先（级别3 > 级别1 > 级别2，级别4 > 级别2）→ 班次优先（调休的人 > 普通班 > 特殊班）。
// 违反“拦截”规则的方案不会出现；违反“只提示”规则的方案照常列出，并写明提示。

export interface PlanStep {
  personId: PersonId;
  fromLabel: string; // 这个人原来当天的情况：当天调休 / 3-1 / 空白格
  takesShiftId: ShiftId;
  takesFromPersonId: PersonId;
}

export interface LeavePlan {
  steps: PlanStep[];
  changes: CellChange[];
  warnings: string[]; // 只提示的规则
}

export interface ExcludedCandidate {
  personId: PersonId;
  reason: string;
}

export interface PlanSearchResult {
  plans: LeavePlan[];
  excluded: ExcludedCandidate[];
}

// 找人顶班时各级别的优先顺序（按申请人的级别）
export const LEVEL_PRIORITY: Record<string, string[]> = {
  L1: ['L1', 'L3', 'L2', 'L4'],
  L2: ['L2', 'L4', 'L3', 'L1'],
  L3: ['L3', 'L1', 'L2', 'L4'],
  L4: ['L4', 'L2', 'L3', 'L1'],
};

// 中间环节让出的班次：普通班优先，特殊 / 特定 / 药师班其次
const CATEGORY_RANK: Record<string, number> = { normal: 0, custom: 1, special: 2, exclusive: 3, pharmacist: 3 };

const MAX_STEPS = 3; // 最多几个人接班（不含申请人）
const MAX_PLANS = 30;
const MAX_CHECKS = 4000;
const MAX_EXCLUDED = 10;

interface SearchOptions {
  ctx: RuleContext;
  requesterId: PersonId;
  dayIndex: number;
  leaveShiftId: ShiftId;
  maxSteps?: number;
}

// 当前排班里这个人当天能不能被拉进链条：能接 target 吗（只看级别和人员特例）
function eligible(person: Person, target: ShiftType): string {
  if (person.status === 'frozen') return '已冻结，暂时不排班';
  return canPersonWork(person, target);
}

export function findLeavePlans(opts: SearchOptions): PlanSearchResult {
  const { ctx, requesterId, dayIndex, leaveShiftId } = opts;
  const maxSteps = opts.maxSteps ?? MAX_STEPS;
  const date: DateStr = ctx.dates[dayIndex];
  const cells = ctx.cells;
  const valueOf = (pid: string): CellValue => cells[cellKey(pid, date)] ?? null;
  const shiftOf = (pid: string): ShiftType | null => {
    const v = valueOf(pid);
    return v && 'shiftId' in v ? ctx.shiftsById.get(v.shiftId) ?? null : null;
  };

  const requester = ctx.people.find((p) => p.id === requesterId) ?? ctx.data.people.find((p) => p.id === requesterId)!;
  const target = shiftOf(requesterId);
  if (!target) return { plans: [], excluded: [] };

  const priority = LEVEL_PRIORITY[requester.levelId] ?? [];
  const levelRank = (p: Person) => {
    const i = priority.indexOf(p.levelId);
    return i < 0 ? priority.length : i;
  };

  // 同一天已经确认过的调休方案里的人（申请人和顶班链上的人）在这一天已排定，锁住不再参与新的方案，
  // 否则同一个人会被重复调度，两个方案互相覆盖
  const lockedBy = new Map<PersonId, string>();
  for (const cs of ctx.data.changes) {
    if (cs.undone || cs.kind !== 'leave' || cs.roundId !== ctx.round.id) continue;
    for (const ch of cs.changes) {
      if (ch.date === date && ch.personId !== requesterId) lockedBy.set(ch.personId, cs.title);
    }
  }
  const others = ctx.people.filter((p) => p.id !== requesterId && !lockedBy.has(p.id));
  // 链条末端（万能格）：删除班次留下的空白格、当天调休 / 补休、当天休息的人
  // 收尾优先级：空白格 > 调休 / 补休 > 休（拉休息的人来顶班会占掉他一个休息日，放最后）
  const endRank = (p: Person) => {
    const v = valueOf(p.id);
    if (v === null) return 0;
    return shiftOf(p.id)?.category === 'leave' ? 1 : 2;
  };
  const freePeople = others.filter((p) => {
    const v = valueOf(p.id);
    if (v === null) return true;
    const c = shiftOf(p.id)?.category;
    return c === 'leave' || c === 'off';
  });
  // 链条中间：当天在上班，而且自己的班允许被换走（夜班、3(10)、需还班的班不参与联动）
  const workingPeople = others.filter((p) => {
    const s = shiftOf(p.id);
    return isWork(s) && s!.canTakeLeave && !s!.needReturn && s!.category !== 'night' && s!.category !== 'fixedGroup';
  });

  const freeLabel = (p: Person) => {
    const v = valueOf(p.id);
    if (!v) return '空白格';
    if ('custom' in v) return v.custom;
    return '当天' + (v.text || shiftOf(p.id)?.name || '调休');
  };

  const requesterChange: CellChange = {
    personId: requesterId,
    date,
    from: valueOf(requesterId),
    to: { shiftId: leaveShiftId },
    role: 'requester',
  };

  type Found = LeavePlan & { rank: number[] };
  const found: Found[] = [];
  const excluded: ExcludedCandidate[] = [];
  const exclude = (personId: PersonId, reason: string) => {
    if (excluded.length < MAX_EXCLUDED && !excluded.some((e) => e.personId === personId)) excluded.push({ personId, reason });
  };
  let checks = 0;
  for (const [pid, title] of lockedBy) {
    const name = ctx.data.people.find((p) => p.id === pid)?.name ?? '';
    exclude(pid, `${name}：当天已在“${title}”里排定，不再参与其他调休方案`);
  }

  const firstReason = (vs: Violation[]) => vs[0]?.message ?? '';
  const peopleById = new Map(ctx.data.people.map((p) => [p.id, p]));
  const warningsOf = (vs: Violation[]) =>
    vs.map((v) => (v.personId ? `${peopleById.get(v.personId)?.name ?? ''}：${v.message}` : v.message));

  // 广度优先：先把所有 1 步方案找全，再找 2 步、3 步
  type Partial = { chain: Person[]; ownShifts: ShiftType[]; changes: CellChange[] };
  let frontier: Partial[] = [{ chain: [requester], ownShifts: [target], changes: [requesterChange] }];

  for (let step = 1; step <= maxSteps && frontier.length > 0; step++) {
    const next: Partial[] = [];
    for (const partial of frontier) {
      const need = partial.ownShifts[partial.ownShifts.length - 1]; // 现在空出来、要有人接的班
      const prevPerson = partial.chain[partial.chain.length - 1];
      const used = new Set(partial.chain.map((p) => p.id));

      // 收尾：当天空闲的人接这个班
      for (const f of freePeople) {
        if (used.has(f.id) || checks >= MAX_CHECKS || found.length >= MAX_PLANS * 3) continue;
        const why = eligible(f, need);
        if (why) {
          if (step === 1) exclude(f.id, `${f.name} 接 ${need.name}：${why}`);
          continue;
        }
        const changes: CellChange[] = [...partial.changes, { personId: f.id, date, from: valueOf(f.id), to: { shiftId: need.id }, role: 'cover' }];
        checks++;
        const result = checkChanges(ctx, changes);
        if (result.hard.length > 0) {
          if (step === 1) exclude(f.id, `${f.name} 接 ${need.name}：${firstReason(result.hard)}`);
          continue;
        }
        const movers = [...partial.chain.slice(1), f];
        found.push({
          rank: [step, result.soft.length > 0 ? 1 : 0, endRank(f), levelRank(f), ...partial.ownShifts.slice(1).map((s) => CATEGORY_RANK[s.category] ?? 2), ...movers.map(levelRank)],
          steps: [
            ...partial.chain.slice(1).map((p, i) => ({
              personId: p.id,
              fromLabel: partial.ownShifts[i + 1].name,
              takesShiftId: partial.ownShifts[i].id,
              takesFromPersonId: partial.chain[i].id,
            })),
            { personId: f.id, fromLabel: freeLabel(f), takesShiftId: need.id, takesFromPersonId: prevPerson.id },
          ],
          changes,
          warnings: warningsOf(result.soft),
        });
      }

      // 延伸：在上班的人接这个班，把自己的班空出来交给下一步
      if (step === maxSteps) continue;
      for (const w of workingPeople) {
        if (used.has(w.id) || checks >= MAX_CHECKS) continue;
        const own = shiftOf(w.id)!;
        if (own.id === need.id) continue;
        if (eligible(w, need)) continue;
        const changes: CellChange[] = [...partial.changes, { personId: w.id, date, from: valueOf(w.id), to: { shiftId: need.id }, role: 'cover' }];
        // 只检查这个人自己：他换班之后有没有违规。当天人数要等链条闭合才算数
        checks++;
        const result = checkChanges({ ...ctx, cells: applyChangesToCells(ctx.cells, partial.changes) }, [changes[changes.length - 1]]);
        if (result.hard.some((v) => v.personId === w.id)) {
          if (step === 1) exclude(w.id, `${w.name} 接 ${need.name}：${firstReason(result.hard.filter((v) => v.personId === w.id))}`);
          continue;
        }
        next.push({ chain: [...partial.chain, w], ownShifts: [...partial.ownShifts, own], changes });
      }
    }
    // 下一层按级别优先排一下，检查次数用完时优先保留好的方向
    next.sort((a, b) => levelRank(a.chain[a.chain.length - 1]) - levelRank(b.chain[b.chain.length - 1]));
    frontier = next;
  }

  found.sort((a, b) => {
    for (let i = 0; i < Math.max(a.rank.length, b.rank.length); i++) {
      const d = (a.rank[i] ?? 0) - (b.rank[i] ?? 0);
      if (d !== 0) return d;
    }
    return 0;
  });

  return { plans: found.slice(0, MAX_PLANS).map(({ steps, changes, warnings }) => ({ steps, changes, warnings })), excluded };
}

export interface ReturnOption {
  date: DateStr;
  shiftId: ShiftId; // 申请人那天去上的班（原本是接班人的）
  changes: CellChange[]; // 还班那天的两处变动
  warnings: string[];
}

// 还班：申请人 A 调休后，在本轮另一天替接班人 B 上一个班，B 那天休息。
// 只列 A 那天休息、B 那天上着 A 能上的班、而且换了以后不违反拦截规则的日期。
export function findReturnOptions(ctx: RuleContext, leaveChanges: CellChange[], requesterId: PersonId, coverId: PersonId, leaveDate: DateStr): ReturnOption[] {
  const requester = ctx.data.people.find((p) => p.id === requesterId)!;
  const afterLeave = applyChangesToCells(ctx.cells, leaveChanges);
  const leaveCtx: RuleContext = { ...ctx, cells: afterLeave };
  const options: ReturnOption[] = [];
  const peopleById = new Map(ctx.data.people.map((p) => [p.id, p]));
  const offShift = ctx.data.shifts.find((s) => s.category === 'off');
  if (!offShift) return options;

  for (const date of ctx.dates) {
    if (date === leaveDate) continue;
    const aVal = afterLeave[cellKey(requesterId, date)] ?? null;
    const bVal = afterLeave[cellKey(coverId, date)] ?? null;
    const aShift = aVal && 'shiftId' in aVal ? ctx.shiftsById.get(aVal.shiftId) : null;
    const bShift = bVal && 'shiftId' in bVal ? ctx.shiftsById.get(bVal.shiftId) : null;
    if (aShift?.category !== 'off' || !bShift || !isWork(bShift)) continue;
    if (canPersonWork(requester, bShift)) continue;
    const changes: CellChange[] = [
      { personId: requesterId, date, from: aVal, to: { shiftId: bShift.id }, role: 'cover' },
      { personId: coverId, date, from: bVal, to: { shiftId: offShift.id }, role: 'cover' },
    ];
    const result = checkChanges(leaveCtx, changes);
    if (result.hard.length > 0) continue;
    options.push({
      date,
      shiftId: bShift.id,
      changes,
      warnings: result.soft.map((v) => (v.personId ? `${peopleById.get(v.personId)?.name ?? ''}：${v.message}` : v.message)),
    });
  }
  // 离调休那天近的排前面
  const dist = (d: string) => Math.abs(new Date(d).getTime() - new Date(leaveDate).getTime());
  return options.sort((a, b) => a.warnings.length - b.warnings.length || dist(a.date) - dist(b.date));
}
