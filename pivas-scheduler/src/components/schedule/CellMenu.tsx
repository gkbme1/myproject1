import { useState } from 'react';
import { shortDate, weekdayName } from '../../domain/dates';
import { checkShiftForCell, leaveBlockedReason } from '../../domain/rules';
import { cellHistory, describeCell } from '../../domain/schedule';
import type { AppData, Person, ShiftType } from '../../domain/types';
import { cellKey } from '../../domain/types';
import type { ScheduleView } from './useScheduleView';

export interface MenuPlacement {
  alignRight: boolean;
  openUp: boolean;
}

interface Props {
  data: AppData;
  view: ScheduleView;
  person: Person;
  dayIndex: number;
  placement: MenuPlacement;
  onStartLeave(leaveShift: ShiftType): void;
  onPickShift(shift: ShiftType): void;
  onSetCustom(text: string): void;
  onDelete(): void;
  onOpenHistory(): void;
}

export function CellMenu({ data, view, person, dayIndex, placement, onStartLeave, onPickShift, onSetCustom, onDelete, onOpenHistory }: Props) {
  const [customText, setCustomText] = useState('');
  const [editingCustom, setEditingCustom] = useState(false);

  const date = view.dates[dayIndex];
  const value = view.cells[cellKey(person.id, date)] ?? null;
  const current = view.shiftOfCell(value);
  const row = view.rowShifts(person.id);

  const history = cellHistory(data, view.round.id, person.id, date);
  const changedTimes = history.length - 1;

  const leaveShifts = data.shifts.filter((s) => s.category === 'leave').sort((a, b) => a.order - b.order);
  const leaveWhy = leaveBlockedReason(current);

  // 只列出这个人按级别和特例能上的班次；规则不允许的照样列出来但置灰，并说明原因
  const choices = data.shifts
    .filter((s) => s.category !== 'leave' && s.allowedLevels.includes(person.levelId))
    .sort((a, b) => a.order - b.order)
    .map((s) => {
      const testRow = [...row];
      testRow[dayIndex] = s;
      return { shift: s, why: s.id === current?.id ? '' : checkShiftForCell(person, testRow, dayIndex, s) };
    });
  const blockedReasons = Array.from(new Set(choices.map((c) => c.why).filter(Boolean)));

  const cls = 'cell-menu ' + (placement.alignRight ? 'right' : 'left') + (placement.openUp ? ' up' : '');

  return (
    <div className={cls} role="dialog" aria-label="格子操作">
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
                {i < history.length - 1 && <span style={{ color: '#98a2b3', fontSize: 12 }}>→</span>}
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
        {choices.map(({ shift, why }) => (
          <button
            key={shift.id}
            className={'shift-choice' + (shift.id === current?.id ? ' current' : '')}
            disabled={!!why}
            title={why || `换成 ${shift.name}`}
            onClick={() => shift.id !== current?.id && onPickShift(shift)}
          >
            {shift.name}
          </button>
        ))}
      </div>
      {blockedReasons.length > 0 && <div className="menu-note">划线的班次不能选：{blockedReasons.join('；')}</div>}

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
