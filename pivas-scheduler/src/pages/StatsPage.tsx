import { useState } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import { useScheduleView } from '../components/schedule/useScheduleView';
import { roundDates, shortDate } from '../domain/dates';
import { isWork } from '../domain/rulesEngine';
import type { AppData, LevelId } from '../domain/types';
import { cellKey } from '../domain/types';

const COLUMNS = '40px 90px 90px repeat(9, minmax(56px, 1fr)) minmax(180px, 2fr)';
const LIMITED = ['3-4', '3-5', '3-6', '3-7'];

// 本轮每人的班次统计：上班、休息、夜班、A/T、3-4~3-7、被改动次数和规则问题
export function StatsPage({ data }: { data: AppData }) {
  const view = useScheduleView(data);
  const [levelFilter, setLevelFilter] = useState<LevelId | 'all'>('all');
  const dates = roundDates(view.round.startDate, view.round.days);

  const changedCount = new Map<string, number>();
  for (const cs of view.changes) {
    if (cs.undone) continue;
    for (const ch of cs.changes) changedCount.set(ch.personId, (changedCount.get(ch.personId) ?? 0) + 1);
  }

  const rows = view.roundPeople
    .filter((p) => levelFilter === 'all' || p.levelId === levelFilter)
    .map((p) => {
      const shifts = dates.map((d) => view.shiftOfCell(view.cells[cellKey(p.id, d)]));
      const count = (pred: (name: string) => boolean) => shifts.filter((s) => s && pred(s.name)).length;
      const problems = view.violations.filter((v) => v.personId === p.id);
      return {
        person: p,
        work: shifts.filter(isWork).length,
        off: shifts.filter((s) => s?.category === 'off').length,
        leave: shifts.filter((s) => s?.category === 'leave').length,
        night: shifts.filter((s) => s?.timeSlot === 'night' && s.name !== '帮夜').length,
        bangye: count((n) => n === '帮夜'),
        a: count((n) => n === 'A'),
        t: count((n) => n === 'T'),
        limited: count((n) => LIMITED.includes(n)),
        changed: changedCount.get(p.id) ?? 0,
        problems,
      };
    });

  const warnCell = (bad: boolean) => (bad ? { color: 'var(--danger)', fontWeight: 700 } : undefined);

  return (
    <>
      <header className="page-header compact">
        <h1>统计</h1>
        <span className="muted">
          第 {view.round.index} 轮 · {shortDate(dates[0])} ~ {shortDate(dates[dates.length - 1])}
        </span>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <div className="grid-toolbar">
          <button className={'chip' + (levelFilter === 'all' ? ' on' : '')} onClick={() => setLevelFilter('all')}>
            全部
          </button>
          {view.groups.map((g) => (
            <button key={g.level.id} className={'chip' + (levelFilter === g.level.id ? ' on' : '')} onClick={() => setLevelFilter(g.level.id)}>
              {g.level.name}
            </button>
          ))}
          <div className="spacer" />
          <span className="muted">红字 = 超出规则限制；“问题”一列是规则检查发现的违规和提示</span>
        </div>
        <div className="card" style={{ overflowX: 'auto' }}>
          <div style={{ minWidth: 1000 }}>
            <div className="table-row table-head" style={{ gridTemplateColumns: COLUMNS }}>
              <span>序号</span>
              <span>姓名</span>
              <span>级别</span>
              <span className="center">上班</span>
              <span className="center">休息</span>
              <span className="center">调休/补休</span>
              <span className="center">夜1/夜2</span>
              <span className="center">帮夜</span>
              <span className="center">A</span>
              <span className="center">T</span>
              <span className="center">3-4~3-7</span>
              <span className="center">被改动</span>
              <span>问题</span>
            </div>
            {rows.map((r) => (
              <div key={r.person.id} className="table-row" style={{ gridTemplateColumns: COLUMNS, fontSize: 13, minHeight: 30 }}>
                <span className="muted">{view.rowNumber.get(r.person.id)}</span>
                <span style={{ fontWeight: 500, color: view.levelsById.get(r.person.levelId)?.nameColor }}>{r.person.name}</span>
                <span className="muted">{view.levelsById.get(r.person.levelId)?.name}</span>
                <span className="center">{r.work}</span>
                <span className="center" style={warnCell(r.off + r.leave < 4)}>{r.off}</span>
                <span className="center">{r.leave || ''}</span>
                <span className="center">{r.night || ''}</span>
                <span className="center">{r.bangye || ''}</span>
                <span className="center" style={warnCell(r.a > 3)}>{r.a || ''}</span>
                <span className="center" style={warnCell(r.t > 3)}>{r.t || ''}</span>
                <span className="center" style={warnCell(r.limited > 2)}>{r.limited || ''}</span>
                <span className="center">{r.changed || ''}</span>
                <span style={{ fontSize: 12 }}>
                  {r.problems.map((v, i) => (
                    <span key={i} className={'badge ' + (v.severity === 'hard' ? 'red' : 'warn')} style={{ marginRight: 4, display: 'inline-block', marginBottom: 2 }}>
                      {v.message}
                    </span>
                  ))}
                </span>
              </div>
            ))}
          </div>
        </div>
      </div>
    </>
  );
}
