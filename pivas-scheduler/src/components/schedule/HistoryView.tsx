import { useState } from 'react';
import { formatDateTime, shortDate, weekdayName } from '../../domain/dates';
import { cellHistory, describeCell, lastAppliedIndex } from '../../domain/schedule';
import type { AppData, ChangeKind } from '../../domain/types';
import { useAppStore } from '../../store/appStore';
import type { ScheduleView } from './useScheduleView';

const KIND_NAMES: Record<ChangeKind, string> = {
  leave: '调休联动',
  manual: '手工改班',
  return: '还班',
  delete: '删除班次',
  custom: '自定义内容',
  import: '导入',
};

const KIND_BADGE: Record<ChangeKind, string> = {
  leave: 'blue',
  manual: 'gray',
  return: 'ok',
  delete: 'red',
  custom: 'gray',
  import: 'gray',
};

export function HistoryView({ data, view }: { data: AppData; view: ScheduleView }) {
  const undo = useAppStore((s) => s.undo);
  const focus = useAppStore((s) => s.historyFocus);
  const clearFocus = useAppStore((s) => s.clearHistoryFocus);
  const [kindFilter, setKindFilter] = useState<ChangeKind | 'all'>('all');
  const [personFilter, setPersonFilter] = useState('all');

  const lastApplied = lastAppliedIndex(view.changes);
  const lastAppliedId = lastApplied >= 0 ? view.changes[lastApplied].id : null;

  const items = view.changes
    .filter((c) => kindFilter === 'all' || c.kind === kindFilter)
    .filter((c) => personFilter === 'all' || c.changes.some((ch) => ch.personId === personFilter))
    .slice()
    .reverse();

  const kindCounts = new Map<ChangeKind, number>();
  view.changes.forEach((c) => kindCounts.set(c.kind, (kindCounts.get(c.kind) ?? 0) + 1));

  const focusPerson = focus ? view.peopleById.get(focus.personId) : null;
  const focusHistory = focus ? cellHistory(data, view.round.id, focus.personId, focus.date) : [];

  return (
    <>
      {focus && focusPerson && (
        <section className="card" style={{ padding: '16px 18px', display: 'flex', flexDirection: 'column', gap: 12 }}>
          <div className="menu-row" style={{ gap: 10 }}>
            <h2 style={{ margin: 0, fontSize: 16 }}>单个格子的修改链</h2>
            <span className="muted" style={{ fontSize: 13 }}>
              {focusPerson.name} · {shortDate(focus.date)} {weekdayName(focus.date)}
            </span>
            <div className="spacer" />
            <button className="btn" onClick={clearFocus}>
              收起
            </button>
          </div>
          <div className="menu-row" style={{ gap: 10 }}>
            {focusHistory.map((h, i) => (
              <span key={i} style={{ display: 'contents' }}>
                <div
                  className="card"
                  style={{
                    padding: '10px 14px',
                    minWidth: 120,
                    display: 'flex',
                    flexDirection: 'column',
                    gap: 2,
                    ...(i === focusHistory.length - 1 ? { borderColor: 'var(--primary)', background: 'var(--primary-soft)' } : {}),
                  }}
                >
                  <span className="muted" style={{ fontSize: 11 }}>{h.label}</span>
                  <span style={{ fontSize: 20, fontWeight: 700 }}>{describeCell(h.value, view.shiftsById) || '空白'}</span>
                  <span className="muted" style={{ fontSize: 11 }}>{h.at ? formatDateTime(h.at) : '导入时'}{i === focusHistory.length - 1 ? ' · 当前值' : ''}</span>
                </div>
                {i < focusHistory.length - 1 && <span style={{ color: '#475467' }}>→</span>}
              </span>
            ))}
          </div>
        </section>
      )}

      <section className="card" style={{ padding: '16px 18px', display: 'flex', flexDirection: 'column', gap: 14 }}>
        <div className="menu-row" style={{ gap: 8 }}>
          <h2 style={{ margin: '0 8px 0 0', fontSize: 16 }}>本轮全部修改</h2>
          <button className={'chip' + (kindFilter === 'all' ? ' on' : '')} onClick={() => setKindFilter('all')}>
            全部 {view.changes.length}
          </button>
          {Array.from(kindCounts.entries()).map(([kind, count]) => (
            <button key={kind} className={'chip' + (kindFilter === kind ? ' on' : '')} onClick={() => setKindFilter(kind)}>
              {KIND_NAMES[kind]} {count}
            </button>
          ))}
          <div className="spacer" />
          <label className="muted" style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
            按人员
            <select className="input" style={{ height: 30 }} value={personFilter} onChange={(e) => setPersonFilter(e.target.value)}>
              <option value="all">全部人员</option>
              {view.roundPeople.map((p) => (
                <option key={p.id} value={p.id}>
                  {p.name}
                </option>
              ))}
            </select>
          </label>
        </div>

        {items.length === 0 && <div className="muted" style={{ padding: '24px 0', textAlign: 'center', fontSize: 13 }}>本轮还没有修改。在排班表里点格子就可以改班、调休。</div>}

        <div style={{ display: 'flex', flexDirection: 'column' }}>
          {items.map((cs, i) => (
            <div key={cs.id} className={'timeline-item' + (cs.undone ? ' undone' : '')}>
              <div className="timeline-time">{formatDateTime(cs.at).replace(' ', '\n')}</div>
              <div className="timeline-line">
                <i style={cs.undone ? { background: '#98a2b3' } : undefined} />
                <b style={i === items.length - 1 ? { background: 'transparent' } : undefined} />
              </div>
              <div style={{ paddingBottom: 16, display: 'flex', flexDirection: 'column', gap: 6 }}>
                <div className="menu-row" style={{ gap: 8 }}>
                  <span className={'badge ' + KIND_BADGE[cs.kind]}>{KIND_NAMES[cs.kind]}</span>
                  <b style={{ fontSize: 14 }}>{cs.title}</b>
                  {cs.undone && <span className="badge gray">已撤销{cs.undoneAt ? ' · ' + formatDateTime(cs.undoneAt) : ''}</span>}
                  <div className="spacer" />
                  {cs.id === lastAppliedId && (
                    <button className="btn" onClick={undo}>
                      撤销这一步
                    </button>
                  )}
                </div>
                <div style={{ fontSize: 13, color: 'var(--ink-2)', lineHeight: 1.8 }}>
                  {cs.changes.map((ch, k) => (
                    <span key={k}>
                      {k > 0 && '　·　'}
                      {view.peopleById.get(ch.personId)?.name} {shortDate(ch.date)}：<span className="old-value">{describeCell(ch.from, view.shiftsById) || '空白'}</span> →{' '}
                      <span className="new-value" style={ch.role === 'requester' ? { color: 'var(--relax)' } : undefined}>
                        {describeCell(ch.to, view.shiftsById) || '空白'}
                      </span>
                    </span>
                  ))}
                </div>
                <div className="muted">
                  {cs.operator}
                  {cs.note ? ` · 备注：${cs.note}` : ''}
                </div>
                {cs.warnings && cs.warnings.length > 0 && (
                  <div style={{ fontSize: 12, color: cs.warnings.some((w) => w.startsWith('违规放行')) ? 'var(--danger)' : 'var(--warn)' }}>
                    {cs.warnings.join('；')}
                  </div>
                )}
              </div>
            </div>
          ))}
        </div>
      </section>
    </>
  );
}
