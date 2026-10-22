import { useState } from 'react';
import { sortedRounds } from '../../domain/rounds';
import { NextRoundDialog } from './NextRoundDialog';
import { parseDate, roundDates } from '../../domain/dates';
import { lastAppliedIndex, redoCandidateIndex } from '../../domain/schedule';
import type { AppData } from '../../domain/types';
import { useAppStore } from '../../store/appStore';
import type { ScheduleTab } from '../../store/appStore';
import { SaveIndicator } from '../SaveIndicator';
import { HistoryView } from './HistoryView';
import { ScheduleGrid } from './ScheduleGrid';
import { useScheduleView } from './useScheduleView';

const WEEKDAY = ['日', '一', '二', '三', '四', '五', '六'];

function dateWithWeekday(date: string) {
  return `${date}（${WEEKDAY[parseDate(date).getDay()]}）`;
}

export function SchedulePage({ data }: { data: AppData }) {
  const view = useScheduleView(data);
  const tab = useAppStore((s) => s.scheduleTab);
  const setTab = useAppStore((s) => s.setScheduleTab);
  const undo = useAppStore((s) => s.undo);
  const redo = useAppStore((s) => s.redo);
  const switchRound = useAppStore((s) => s.switchRound);
  const [showNext, setShowNext] = useState(false);
  const rounds = sortedRounds(data);
  const roundPos = rounds.findIndex((r) => r.id === view.round.id);

  const dates = roundDates(view.round.startDate, view.round.days);
  const canUndo = lastAppliedIndex(view.changes) >= 0;
  const canRedo = redoCandidateIndex(view.changes) >= 0;

  const tabButton = (id: ScheduleTab, label: string, count?: number) => (
    <button className={'tab' + (tab === id ? ' on' : '')} onClick={() => setTab(id)} aria-current={tab === id ? 'page' : undefined}>
      {label}
      {count !== undefined && <span className="count">{count}</span>}
    </button>
  );

  return (
    <>
      <header className="page-header compact">
        <h1>排班</h1>
        <div className="menu-row" style={{ gap: 4 }}>
          <button className="btn icon" aria-label="上一轮" disabled={roundPos <= 0} title="上一轮" onClick={() => switchRound(rounds[roundPos - 1].id)}>
            ‹
          </button>
          <span style={{ fontWeight: 500 }}>
            第 {view.round.index} 轮 · {dateWithWeekday(dates[0])} ~ {dateWithWeekday(dates[dates.length - 1]).slice(5)}
          </span>
          <button
            className="btn icon"
            aria-label="下一轮"
            title={roundPos < rounds.length - 1 ? '下一轮' : '生成下一轮'}
            onClick={() => (roundPos < rounds.length - 1 ? switchRound(rounds[roundPos + 1].id) : setShowNext(true))}
          >
            ›
          </button>
          {roundPos === rounds.length - 1 && (
            <button className="btn" style={{ height: 28 }} onClick={() => setShowNext(true)}>
              生成下一轮
            </button>
          )}
          {roundPos < rounds.length - 1 && <span className="badge warn">不是最新一轮</span>}
        </div>
        <nav className="header-tabs" aria-label="排班页面">
          {tabButton('grid', '排班表')}
          {tabButton('history', '修改历史', view.changes.length)}
          {tabButton('requests', '调休申请记录')}
        </nav>
        <div className="spacer" />
        <span className="stat">
          本轮修改 <b style={{ color: '#9a3412' }}>{view.appliedCount}</b>
        </span>
        <SaveIndicator />
        <button className="btn" onClick={undo} disabled={!canUndo}>
          撤销
        </button>
        <button className="btn" onClick={redo} disabled={!canRedo}>
          重做
        </button>
        <button className="btn" disabled title="导出 Excel 将在后续版本提供">
          导出 Excel
        </button>
      </header>

      {showNext && <NextRoundDialog data={data} onClose={() => setShowNext(false)} />}
      {tab === 'grid' && <ScheduleGrid key={view.round.id} data={data} view={view} />}
      {tab !== 'grid' && (
        <div className="page-body">
          {tab === 'history' && <HistoryView data={data} view={view} />}
          {tab === 'requests' && (
            <div className="card" style={{ padding: 32, textAlign: 'center' }}>
              <div style={{ fontSize: 15, fontWeight: 700, marginBottom: 6 }}>调休申请记录</div>
              <div className="muted">按人员汇总本轮的调休、补休申请和还班情况，将在后续版本提供。</div>
            </div>
          )}
        </div>
      )}
    </>
  );
}
