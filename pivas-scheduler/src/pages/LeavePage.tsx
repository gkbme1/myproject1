import { Fragment, useMemo, useState } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import { formatDateTime, shortDate, weekdayName } from '../domain/dates';
import { computeLeaveBalances } from '../domain/leaveBalance';
import { getRound } from '../domain/schedule';
import type { AppData, LevelId } from '../domain/types';
import { levelLabel } from '../domain/types';
import { useAppStore } from '../store/appStore';

const COLUMNS = '48px 100px 140px 110px 90px 90px 1fr';

// 剩余假期：初始天数手工维护；不需要还班的调休每次扣 1 天（3-4/3-5/3-7 调休要还班，不扣）
export function LeavePage({ data }: { data: AppData }) {
  const setLeaveInitial = useAppStore((s) => s.setLeaveInitial);
  const balances = useMemo(() => computeLeaveBalances(data), [data]);
  const [levelFilter, setLevelFilter] = useState<LevelId | 'all'>('all');
  const [search, setSearch] = useState('');
  const [expanded, setExpanded] = useState<string | null>(null);
  const [batch, setBatch] = useState('');

  const levels = [...data.levels].sort((a, b) => a.order - b.order);
  const round = getRound(data);
  const roundOrder = round.personIds ?? data.people.map((p) => p.id);
  const ordered = [...roundOrder.map((id) => data.people.find((p) => p.id === id)!).filter(Boolean), ...data.people.filter((p) => !roundOrder.includes(p.id))];
  const people = ordered.filter((p) => (levelFilter === 'all' || p.levelId === levelFilter) && (!search.trim() || p.name.includes(search.trim())));
  const totalDeducted = people.reduce((n, p) => n + (balances.get(p.id)?.deductions.length ?? 0), 0);

  return (
    <>
      <header className="page-header compact">
        <h1>假期</h1>
        <span className="muted">初始假期手工填写；每次不需要还班的调休扣 1 天，3-4 / 3-5 / 3-7 调休要还班，不扣</span>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <div className="grid-toolbar">
          <button className={'chip' + (levelFilter === 'all' ? ' on' : '')} onClick={() => setLevelFilter('all')}>
            全部
          </button>
          {levels.map((l) => (
            <button key={l.id} className={'chip' + (levelFilter === l.id ? ' on' : '')} onClick={() => setLevelFilter(l.id)}>
              {levelLabel(l)}
            </button>
          ))}
          <input className="input" type="search" placeholder="搜索姓名" aria-label="搜索姓名" value={search} onChange={(e) => setSearch(e.target.value)} style={{ width: 130, height: 28 }} />
          <div className="spacer" />
          <span className="muted" style={{ fontSize: 13 }}>
            共 {people.length} 人，已扣减 {totalDeducted} 天
          </span>
          <form
            className="menu-row"
            onSubmit={(e) => {
              e.preventDefault();
              const n = Number(batch);
              if (batch === '' || Number.isNaN(n)) return;
              if (!window.confirm(`把下面列出的 ${people.length} 人的初始假期都设为 ${n} 天？`)) return;
              setLeaveInitial(Object.fromEntries(people.map((p) => [p.id, n])));
              setBatch('');
            }}
          >
            <input className="input" type="number" placeholder="天数" aria-label="批量设置初始假期" value={batch} onChange={(e) => setBatch(e.target.value)} style={{ width: 70, height: 28 }} />
            <button className="btn" type="submit" style={{ height: 28 }} disabled={batch === ''}>
              批量设置初始假期
            </button>
          </form>
        </div>

        <div className="card" style={{ overflowX: 'auto' }}>
          <div style={{ minWidth: 820 }}>
            <div className="table-row table-head" style={{ gridTemplateColumns: COLUMNS }}>
              <span>序号</span>
              <span>姓名</span>
              <span>级别</span>
              <span className="center">初始假期（天）</span>
              <span className="center">已扣减</span>
              <span className="center">当前剩余</span>
              <span>扣减明细</span>
            </div>
            {people.map((p, i) => {
              const b = balances.get(p.id)!;
              const level = data.levels.find((l) => l.id === p.levelId);
              const open = expanded === p.id;
              return (
                <Fragment key={p.id}>
                  <div className="table-row" style={{ gridTemplateColumns: COLUMNS, fontSize: 13, minHeight: 34 }}>
                    <span className="muted">{i + 1}</span>
                    <span style={{ fontWeight: 500, color: level?.nameColor }}>{p.name}</span>
                    <span className="muted">{level ? levelLabel(level) : ''}</span>
                    <span className="center">
                      <InitialInput key={p.id + ':' + b.initial} value={b.initial} onCommit={(n) => setLeaveInitial({ [p.id]: n })} label={`${p.name} 初始假期`} />
                    </span>
                    <span className="center">{b.deductions.length || ''}</span>
                    <span className="center" style={{ fontWeight: 700, color: b.current < 0 ? 'var(--danger)' : b.current === 0 ? 'var(--warn)' : '#15803d' }}>
                      {b.current}
                    </span>
                    <span>
                      {b.deductions.length > 0 ? (
                        <button className="menu-link" style={{ padding: 0, color: 'var(--primary)', fontSize: 12 }} onClick={() => setExpanded(open ? null : p.id)}>
                          {open ? '收起' : `查看 ${b.deductions.length} 条`}
                        </button>
                      ) : (
                        <span className="muted">—</span>
                      )}
                    </span>
                  </div>
                  {open && (
                    <div style={{ background: '#f7f9fb', borderBottom: '1px solid var(--line-soft)', padding: '6px 16px 8px 160px' }}>
                      {b.deductions.map((d) => (
                        <div key={d.changeSetId} style={{ fontSize: 12, lineHeight: 2, color: 'var(--ink-2)' }}>
                          第 {d.roundIndex} 轮 · {shortDate(d.date)} {weekdayName(d.date)} · {d.typeName}（让出 {d.fromShiftName || '—'}）· 扣 1 天 ·{' '}
                          {d.source === 'auto' ? '调休申请表自动安排' : '手工调休'} · 操作于 {formatDateTime(d.at)}
                        </div>
                      ))}
                    </div>
                  )}
                </Fragment>
              );
            })}
          </div>
        </div>
      </div>
    </>
  );
}

// 数字输入：改完离开输入框或按回车才保存
function InitialInput({ value, onCommit, label }: { value: number; onCommit(n: number): void; label: string }) {
  const [text, setText] = useState(String(value));
  const commit = () => {
    const n = Number(text);
    if (text.trim() === '' || Number.isNaN(n)) {
      setText(String(value));
      return;
    }
    if (n !== value) onCommit(n);
  };
  return (
    <input
      className="input"
      type="number"
      aria-label={label}
      value={text}
      onChange={(e) => setText(e.target.value)}
      onBlur={commit}
      onKeyDown={(e) => e.key === 'Enter' && commit()}
      style={{ width: 70, height: 28, textAlign: 'center' }}
    />
  );
}
