import { useRef, useState } from 'react';
import type { PointerEvent as ReactPointerEvent } from 'react';
import { shortDate, weekdayName } from '../../domain/dates';
import type { PlanSearchResult } from '../../domain/plans';
import type { Person, ShiftType } from '../../domain/types';
import type { ScheduleView } from './useScheduleView';

interface Props {
  view: ScheduleView;
  requester: Person;
  dayIndex: number;
  target: ShiftType;
  leaveShifts: ShiftType[];
  leaveShift: ShiftType;
  result: PlanSearchResult;
  picked: number;
  note: string;
  onPick(index: number): void;
  onChangeLeave(shift: ShiftType): void;
  onChangeNote(note: string): void;
  onConfirm(): void;
  onClose(): void;
}

// 调休方案浮窗：不遮暗背景，可以拖动，选中方案时表格里会用虚线框预览变动
export function PlanPanel(props: Props) {
  const { view, requester, dayIndex, target, leaveShifts, leaveShift, result, picked, note } = props;
  // 浮窗默认放在远离被调休那一列的一侧，避免一打开就挡住要看的格子
  const [pos, setPos] = useState(() => {
    const onRightHalf = dayIndex >= view.dates.length / 2;
    return { x: onRightHalf ? 96 : Math.max(96, window.innerWidth - 480), y: 120 };
  });
  const drag = useRef<{ dx: number; dy: number } | null>(null);

  const onPointerDown = (e: ReactPointerEvent<HTMLDivElement>) => {
    if ((e.target as HTMLElement).closest('button')) return;
    drag.current = { dx: e.clientX - pos.x, dy: e.clientY - pos.y };
    e.currentTarget.setPointerCapture(e.pointerId);
  };
  const onPointerMove = (e: ReactPointerEvent<HTMLDivElement>) => {
    if (!drag.current) return;
    const x = Math.min(Math.max(0, e.clientX - drag.current.dx), window.innerWidth - 200);
    const y = Math.min(Math.max(0, e.clientY - drag.current.dy), window.innerHeight - 80);
    setPos({ x, y });
  };
  const onPointerUp = () => {
    drag.current = null;
  };

  const date = view.dates[dayIndex];
  const nameOf = (id: string) => view.peopleById.get(id)?.name ?? '?';
  const levelOf = (id: string) => {
    const p = view.peopleById.get(id);
    return p ? view.levelsById.get(p.levelId)?.shortName ?? '' : '';
  };

  return (
    <section className="plan-panel" style={{ left: pos.x, top: pos.y }} role="dialog" aria-label="调休方案">
      <div className="plan-panel-head" onPointerDown={onPointerDown} onPointerMove={onPointerMove} onPointerUp={onPointerUp}>
        <div style={{ display: 'flex', flexDirection: 'column' }}>
          <strong style={{ fontSize: 14 }}>
            {leaveShift.name}方案 · {requester.name} {shortDate(date)} {weekdayName(date)} · {target.name}
          </strong>
          <span className="muted" style={{ fontSize: 11 }}>
            按住标题栏可拖动，不挡住要看的格子
          </span>
        </div>
        <div className="spacer" />
        <button className="btn icon" aria-label="关闭" onClick={props.onClose}>
          ✕
        </button>
      </div>

      <div className="plan-panel-body">
        <div className="menu-row">
          <span className="muted">类型</span>
          {leaveShifts.map((s) => (
            <button key={s.id} className={'chip' + (s.id === leaveShift.id ? ' on' : '')} onClick={() => props.onChangeLeave(s)}>
              {s.name}
            </button>
          ))}
          <input
            className="input"
            style={{ flex: 1, minWidth: 100, height: 28, fontSize: 12 }}
            placeholder="备注（可选，会记入修改历史）"
            aria-label="备注"
            value={note}
            onChange={(e) => props.onChangeNote(e.target.value)}
          />
        </div>

        {result.plans.length > 0 ? (
          <div className="muted">找到 {result.plans.length} 个可行方案（步数少的在前）· 选中后表格里会用虚线框预览变动</div>
        ) : (
          <div className="menu-note" style={{ fontSize: 13, color: 'var(--danger)', background: '#fdecec' }}>
            没有找到可行方案：当天没有能顶这个班的人。可以先把别人的班删成空白格，再回来调休。
          </div>
        )}

        {result.plans.map((plan, i) => (
          <button key={i} className={'plan-card' + (i === picked ? ' on' : '')} onClick={() => props.onPick(i)}>
            <span style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
              <strong style={{ fontSize: 14 }}>方案 {i + 1}</strong>
              <span className="badge gray">{plan.steps.length} 步</span>
              <span className="badge ok">规则检查通过</span>
              {i === 0 && <span style={{ marginLeft: 'auto', fontSize: 12, color: 'var(--primary)', fontWeight: 500 }}>推荐</span>}
            </span>
            {plan.steps.map((step, k) => (
              <span key={k} className="plan-step">
                <span>{nameOf(step.personId)}</span>
                <span className="badge gray">
                  {levelOf(step.personId)} · {step.fromLabel}
                </span>
                <span style={{ color: '#98a2b3' }}>→</span>
                <span>
                  接{nameOf(step.takesFromPersonId)}的 <b>{view.shiftsById.get(step.takesShiftId)?.name}</b>
                </span>
              </span>
            ))}
          </button>
        ))}

        {result.excluded.length > 0 && (
          <details className="muted">
            <summary style={{ cursor: 'pointer' }}>有 {result.excluded.length} 个人因为规则限制不能顶班，点开看原因</summary>
            <ul style={{ margin: '6px 0 0', paddingLeft: 18, lineHeight: 1.8 }}>
              {result.excluded.map((e) => (
                <li key={e.personId}>{e.reason}</li>
              ))}
            </ul>
          </details>
        )}
      </div>

      <div className="plan-panel-foot">
        <button className="btn" onClick={props.onClose}>
          取消
        </button>
        <button className="btn primary" disabled={result.plans.length === 0} onClick={props.onConfirm}>
          确认执行方案 {result.plans.length > 0 ? picked + 1 : ''}
        </button>
      </div>
    </section>
  );
}
