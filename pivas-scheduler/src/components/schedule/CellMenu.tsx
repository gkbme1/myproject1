import { useLayoutEffect, useRef, useState } from 'react';
import { shortDate, weekdayName } from '../../domain/dates';
import { canPersonWork, leaveBlockedReason } from '../../domain/rules';
import { checkChanges, violationText } from '../../domain/rulesEngine';
import { cellHistory, describeCell } from '../../domain/schedule';
import type { AppData, Person, ShiftType } from '../../domain/types';
import { cellKey } from '../../domain/types';
import type { ScheduleView } from './useScheduleView';

// 被点的格子在窗口里的位置；菜单贴着它弹出，并保证整个菜单都在窗口里
export interface MenuPlacement {
  top: number;
  bottom: number;
  left: number;
  right: number;
}

const MENU_WIDTH = 320;
const MARGIN = 8;

interface Props {
  data: AppData;
  view: ScheduleView;
  person: Person;
  dayIndex: number;
  placement: MenuPlacement;
  onStartLeave(leaveShift: ShiftType): void;
  // violations：选了违反拦截规则的班次（用户打开了“仍要排”）时，违反了哪些规则
  onPickShift(shift: ShiftType, warnings: string[], violations: string[]): void;
  onSetCustom(text: string): void;
  onDelete(): void;
  onOpenHistory(): void;
}

export function CellMenu({ data, view, person, dayIndex, placement, onStartLeave, onPickShift, onSetCustom, onDelete, onOpenHistory }: Props) {
  const [customText, setCustomText] = useState('');
  const [editingCustom, setEditingCustom] = useState(false);
  const [force, setForce] = useState(false); // 仍要排：允许选违反拦截规则的班次
  const ref = useRef<HTMLDivElement>(null);
  const [pos, setPos] = useState({ top: placement.bottom + 4, left: placement.left, visible: false });

  // 先渲染出来量高度，再决定往下还是往上弹，避免菜单被窗口边缘截掉
  useLayoutEffect(() => {
    const h = ref.current?.offsetHeight ?? 0;
    let top = placement.bottom + 4;
    if (top + h > window.innerHeight - MARGIN) top = Math.max(MARGIN, Math.min(placement.top - h - 4, window.innerHeight - MARGIN - h));
    let left = placement.left;
    if (left + MENU_WIDTH > window.innerWidth - MARGIN) left = Math.max(MARGIN, placement.right - MENU_WIDTH);
    setPos({ top, left, visible: true });
  }, [placement, editingCustom]);

  const date = view.dates[dayIndex];
  const value = view.cells[cellKey(person.id, date)] ?? null;
  const current = view.shiftOfCell(value);

  const history = cellHistory(data, view.round.id, person.id, date);
  const changedTimes = history.length - 1;

  const leaveShifts = data.shifts.filter((s) => s.category === 'leave').sort((a, b) => a.order - b.order);
  const leaveWhy = leaveBlockedReason(current);

  // 只列出这个人按级别能上的班次；违反拦截规则的置灰并说明原因，违反提示规则的可以选但标出来
  const choices = data.shifts
    .filter((s) => s.category !== 'leave' && s.allowedLevels.includes(person.levelId))
    .sort((a, b) => a.order - b.order)
    .map((s) => {
      if (s.id === current?.id) return { shift: s, why: '', warns: [] as string[], forcible: false, hard: [] as string[] };
      const special = canPersonWork(person, s);
      if (special) return { shift: s, why: special, warns: [] as string[], forcible: false, hard: [] as string[] };
      const result = checkChanges(view.ruleCtx, [{ personId: person.id, date, from: value, to: { shiftId: s.id }, role: 'manual' }]);
      return {
        shift: s,
        why: result.hard[0]?.message ?? '',
        warns: result.soft.map((v) => violationText(v, view.peopleById)),
        forcible: result.hard.length > 0, // 规则拦截的可以“仍要排”；级别、人员特例不行
        hard: result.hard.map((v) => violationText(v, view.peopleById)),
      };
    });
  const existing = view.cellViolations.get(cellKey(person.id, date)) ?? [];
  const blockedReasons = Array.from(new Set(choices.map((c) => c.why).filter(Boolean)));

  return (
    <div ref={ref} className="cell-menu" role="dialog" aria-label="格子操作" style={{ top: pos.top, left: pos.left, visibility: pos.visible ? 'visible' : 'hidden' }}>
      <div className="menu-title">
        {person.name} · {shortDate(date)} {weekdayName(date)} · 当前：{describeCell(value, view.shiftsById) || '空白'}
      </div>

      {changedTimes > 0 ? (
        <div className="menu-history">
          <div style={{ display: 'flex', alignItems: 'center', gap: 6, fontSize: 12, color: '#9a3412', fontWeight: 700 }}>
            <span className="mark-triangle" />
            本轮改过 {changedTimes} 次
          </div>
          <div className="menu-row">
            {history.map((h, i) => (
              <span key={i} style={{ display: 'contents' }}>
                <span className={'history-value' + (i === history.length - 1 ? ' last' : '')}>{describeCell(h.value, view.shiftsById) || '空白'}</span>
                {i < history.length - 1 && <span style={{ color: '#475467', fontSize: 12 }}>→</span>}
              </span>
            ))}
          </div>
          <div className="muted" style={{ fontSize: 11 }}>
            最近一次：{history[history.length - 1].label}
          </div>
          <button className="menu-link" style={{ padding: 0, color: 'var(--primary)', fontSize: 12 }} onClick={onOpenHistory}>
            查看完整修改记录 →
          </button>
        </div>
      ) : (
        <div className="muted">原始排班，本轮没有改过</div>
      )}

      {existing.length > 0 && (
        <div className="menu-note" style={{ color: existing.some((v) => v.severity === 'hard') ? 'var(--danger)' : 'var(--warn)', background: '#fff7ed' }}>
          这个格子目前的问题：{existing.map((v) => v.message).join('；')}
        </div>
      )}

      <div className="menu-section">申请休息</div>
      <div className="menu-row">
        {leaveShifts.map((s, i) => (
          <button key={s.id} className={'menu-action' + (i === 0 ? ' primary' : '')} disabled={!!leaveWhy} onClick={() => onStartLeave(s)}>
            {s.name}…
          </button>
        ))}
      </div>
      {leaveWhy && <div className="menu-note">{leaveWhy}</div>}

      <div className="menu-section">换成其他班次（只列出这个级别能上的）</div>
      <div className="menu-row">
        {choices.map(({ shift, why, warns, forcible, hard }) => (
          <button
            key={shift.id}
            className={
              'shift-choice' + (shift.id === current?.id ? ' current' : '') + (warns.length > 0 ? ' warn' : '') + (force && forcible ? ' forced' : '')
            }
            disabled={!!why && !(force && forcible)}
            title={why ? (force && forcible ? `会违反：${hard.join('；')}` : why) : warns.length > 0 ? `可以换，但有提示：${warns.join('；')}` : `换成 ${shift.name}`}
            onClick={() => shift.id !== current?.id && onPickShift(shift, warns, force && forcible ? hard : [])}
          >
            {shift.name}
          </button>
        ))}
      </div>
      {blockedReasons.length > 0 && !force && <div className="menu-note">划线的班次不能选：{blockedReasons.join('；')}</div>}
      {choices.some((c) => c.forcible) && (
        <label style={{ display: 'flex', alignItems: 'flex-start', gap: 6, fontSize: 12, lineHeight: 1.5, color: force ? 'var(--danger)' : 'var(--ink-2)', cursor: 'pointer' }}>
          <input type="checkbox" checked={force} onChange={(e) => setForce(e.target.checked)} style={{ marginTop: 2, flexShrink: 0 }} />
          仍要排（违反规则的班次也能选，选的时候会再确认一次，并记入修改记录）
        </label>
      )}
      {choices.some((c) => c.warns.length > 0) && <div className="menu-note">带橙点的班次可以选，但会有规则提示（鼠标停在上面能看到）</div>}

      <div style={{ borderTop: '1px solid var(--line-soft)', paddingTop: 6, display: 'flex', flexDirection: 'column' }}>
        {editingCustom ? (
          <form
            className="menu-row"
            style={{ padding: '4px 0' }}
            onSubmit={(e) => {
              e.preventDefault();
              if (customText.trim()) onSetCustom(customText.trim());
            }}
          >
            <input className="input" style={{ flex: 1, height: 30 }} autoFocus placeholder="如：外出培训" value={customText} onChange={(e) => setCustomText(e.target.value)} aria-label="自定义内容" />
            <button className="btn primary" type="submit" disabled={!customText.trim()}>
              确定
            </button>
          </form>
        ) : (
          <button className="menu-link" onClick={() => setEditingCustom(true)}>
            ✎　填写自定义内容（如 外出培训）
          </button>
        )}
        <button className="menu-link danger" onClick={onDelete} disabled={value === null}>
          ✕　删除班次（变成空白格，可以接任何班）
        </button>
      </div>
    </div>
  );
}
