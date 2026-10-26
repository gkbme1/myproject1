import { useMemo, useState } from 'react';
import { addDays, shortDate, weekdayName } from '../../domain/dates';
import { generateNextRound } from '../../domain/rounds';
import { getRound } from '../../domain/schedule';
import type { AppData } from '../../domain/types';
import { useAppStore } from '../../store/appStore';

// 生成下一轮：说明滚动规则，选模板，预览谁接谁的行，确认后切到新一轮
export function NextRoundDialog({ data, onClose }: { data: AppData; onClose(): void }) {
  const addRound = useAppStore((s) => s.addRound);
  const [useModified, setUseModified] = useState(true);
  const startDate = addDays(getRound(data).startDate, getRound(data).days);
  const endDate = addDays(startDate, getRound(data).days - 1);
  const pendingApps = (data.leaveApplications ?? []).filter((a) => a.status === 'pending' && a.date >= startDate && a.date <= endDate);
  const [processApps, setProcessApps] = useState(true);
  const current = getRound(data);
  const result = useMemo(() => generateNextRound(data, { useModified }), [data, useModified]);
  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  const start = addDays(current.startDate, current.days);
  const end = addDays(start, current.days - 1);
  const locked = (current.personIds ?? data.people.map((p) => p.id)).map((id) => peopleById.get(id)).filter((p) => p && p.status !== 'active');

  return (
    <div className="overlay" style={{ background: 'rgba(27, 36, 48, 0.45)' }}>
      <div className="dialog" role="dialog" aria-modal="true" aria-labelledby="next-round-title" style={{ maxWidth: 640, gap: 14 }}>
        <h1 id="next-round-title" style={{ margin: 0, fontSize: 20 }}>
          生成第 {current.index + 1} 轮排班
        </h1>
        <div style={{ fontSize: 13 }}>
          {shortDate(start)} {weekdayName(start)} ~ {shortDate(end)} {weekdayName(end)}，共 {current.days} 天
        </div>
        <div className="menu-note" style={{ fontSize: 12, lineHeight: 1.8 }}>
          按“班不动、人滚动”生成：每个级别里，排班表每一行的班次规律不变，人往下挪一行，最后一个人挪到第一行。
          {locked.length > 0 && <>锁行 / 冻结的人留在原来那一行：{locked.map((p) => p!.name).join('、')}。</>}
          调休、补休这些个人的休假不跟着行走，新一轮里会变成空白格，需要再安排。
        </div>

        <fieldset style={{ border: 0, padding: 0, margin: 0, display: 'flex', flexDirection: 'column', gap: 6 }}>
          <legend className="muted" style={{ marginBottom: 6 }}>用哪张表当模板</legend>
          <label className="menu-row" style={{ fontSize: 13 }}>
            <input type="radio" name="tpl" checked={useModified} onChange={() => setUseModified(true)} />
            本轮修改后的排班（调整过的班次会带到下一轮）
          </label>
          <label className="menu-row" style={{ fontSize: 13 }}>
            <input type="radio" name="tpl" checked={!useModified} onChange={() => setUseModified(false)} />
            本轮导入时的原始排班（本轮的修改不带过去）
          </label>
        </fieldset>

        <div style={{ fontSize: 13 }}>
          {result.moves.length} 人换了行，{result.clearedLeaveCells} 个调休 / 补休 / 自定义格子会变成空白格。
        </div>
        <div style={{ maxHeight: 160, overflowY: 'auto', fontSize: 12, color: 'var(--ink-2)', lineHeight: 1.8 }} className="card">
          <div style={{ padding: '6px 10px' }}>
            {result.moves.map((m) => (
              <span key={m.personId} style={{ display: 'inline-block', marginRight: 14 }}>
                {peopleById.get(m.personId)?.name} ← {peopleById.get(m.fromRowOf)?.name}的行
              </span>
            ))}
          </div>
        </div>

        {pendingApps.length > 0 && (
          <label className="menu-row" style={{ fontSize: 13, fontWeight: 500 }}>
            <input type="checkbox" checked={processApps} onChange={(e) => setProcessApps(e.target.checked)} />
            按登记先后自动安排“下一轮调休申请表”里的 {pendingApps.length} 条申请
          </label>
        )}
        <div className="menu-row" style={{ justifyContent: 'flex-end' }}>
          <button className="btn" onClick={onClose}>
            取消
          </button>
          <button
            className="btn primary"
            onClick={() => {
              addRound(result.round, processApps && pendingApps.length > 0);
              onClose();
            }}
          >
            生成并切换到第 {current.index + 1} 轮
          </button>
        </div>
      </div>
    </div>
  );
}
