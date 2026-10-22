import { formatDateTime, roundDates, shortDate } from '../domain/dates';
import { getRound, roundChanges } from '../domain/schedule';
import type { AppData } from '../domain/types';
import { useAppStore } from '../store/appStore';

// 再次打开软件时的"欢迎回来"：告诉用户上次的进度都在，点一下继续
export function ResumeDialog({ data }: { data: AppData }) {
  const previousActiveAt = useAppStore((s) => s.previousActiveAt);
  const dismiss = useAppStore((s) => s.dismissResume);
  const setScheduleTab = useAppStore((s) => s.setScheduleTab);

  const round = getRound(data);
  const dates = roundDates(round.startDate, round.days);
  const applied = roundChanges(data, round.id).filter((c) => !c.undone);
  const last = applied[applied.length - 1];

  return (
    <div className="overlay">
      <div className="dialog" role="dialog" aria-modal="true" aria-labelledby="resume-title">
        <div className="menu-row" style={{ gap: 12 }}>
          <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="#1F5FA8" strokeWidth="2" aria-hidden="true">
            <rect x="3" y="4" width="18" height="17" rx="2" />
            <path d="M3 9h18M8 2v4M16 2v4" />
          </svg>
          <div>
            <h1 id="resume-title" style={{ margin: 0, fontSize: 22 }}>
              欢迎回来
            </h1>
            <div className="muted" style={{ fontSize: 13 }}>
              已找到上次的排班进度，所有修改都还在
            </div>
          </div>
        </div>

        <div>
          <div className="kv">
            <span>上次使用</span>
            <span>{previousActiveAt ? formatDateTime(previousActiveAt) : '—'}</span>
          </div>
          <div className="kv">
            <span>排班轮次</span>
            <span>
              第 {round.index} 轮 · {shortDate(dates[0])} ~ {shortDate(dates[dates.length - 1])}
            </span>
          </div>
          <div className="kv">
            <span>本轮已修改</span>
            <span>{applied.length} 处</span>
          </div>
          <div className="kv" style={{ borderBottom: 0 }}>
            <span>最后一步</span>
            <span>{last ? last.title : '还没有修改'}</span>
          </div>
        </div>

        <button className="btn primary big-button" autoFocus onClick={dismiss}>
          继续上次排班
        </button>
        <button
          className="btn"
          style={{ height: 44 }}
          onClick={() => {
            setScheduleTab('history');
            dismiss();
          }}
        >
          先看看改了什么
        </button>

        <div className="menu-note" style={{ fontSize: 12, lineHeight: 1.7 }}>
          每次修改都会马上保存到本机，不需要手动点“保存”。即使直接关机或断电，再次打开也会回到最后一步。每天自动备份一次，保留最近 30 天。
        </div>
      </div>
    </div>
  );
}
