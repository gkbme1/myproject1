import { useMemo, useState } from 'react';
import type { CSSProperties, MouseEvent } from 'react';
import { isWeekend, shortDate, weekdayName } from '../../domain/dates';
import { findLeavePlans } from '../../domain/plans';
import { describeCell } from '../../domain/schedule';
import type { AppData, CellValue, LevelId, Person, ShiftType } from '../../domain/types';
import { cellKey } from '../../domain/types';
import { useAppStore } from '../../store/appStore';
import { CellMenu } from './CellMenu';
import type { MenuPlacement } from './CellMenu';
import { PlanPanel } from './PlanPanel';
import type { ScheduleView } from './useScheduleView';

interface OpenCell {
  personId: string;
  dayIndex: number;
  placement: MenuPlacement;
}

interface LeaveSession {
  personId: string;
  dayIndex: number;
  leaveShiftId: string;
  picked: number;
  note: string;
}

const MENU_HEIGHT_GUESS = 440;

export function ScheduleGrid({ data, view }: { data: AppData; view: ScheduleView }) {
  const commitChange = useAppStore((s) => s.commitChange);
  const undo = useAppStore((s) => s.undo);
  const openCellHistory = useAppStore((s) => s.openCellHistory);

  const [levelFilter, setLevelFilter] = useState<LevelId | 'all'>('all');
  const [onlyModified, setOnlyModified] = useState(false);
  const [openCell, setOpenCell] = useState<OpenCell | null>(null);
  const [leave, setLeave] = useState<LeaveSession | null>(null);
  const [toast, setToast] = useState('');

  const leaveShifts = useMemo(() => data.shifts.filter((s) => s.category === 'leave').sort((a, b) => a.order - b.order), [data.shifts]);

  const planResult = useMemo(() => {
    if (!leave) return null;
    return findLeavePlans(data, view.cells, view.dates, leave.personId, leave.dayIndex, leave.leaveShiftId);
  }, [data, view, leave]);

  // 方案预览：选中方案会变动的格子，key -> "原→新"
  const preview = useMemo(() => {
    const map = new Map<string, string>();
    const plan = planResult?.plans[leave?.picked ?? 0];
    if (!plan) return map;
    for (const ch of plan.changes) {
      map.set(cellKey(ch.personId, ch.date), `${describeCell(ch.from, view.shiftsById) || '空'}→${describeCell(ch.to, view.shiftsById)}`);
    }
    return map;
  }, [planResult, leave, view.shiftsById]);

  const label = (person: Person, dayIndex: number) => `${person.name} ${shortDate(view.dates[dayIndex])}`;

  const commitCell = (kind: 'manual' | 'delete' | 'custom', person: Person, dayIndex: number, to: CellValue, title: string) => {
    const date = view.dates[dayIndex];
    commitChange(kind, title, [{ personId: person.id, date, from: view.cells[cellKey(person.id, date)] ?? null, to, role: 'manual' }]);
    setOpenCell(null);
    setToast(`已保存：${title}`);
  };

  const onCellClick = (e: MouseEvent<HTMLButtonElement>, personId: string, dayIndex: number) => {
    const rect = e.currentTarget.getBoundingClientRect();
    setLeave(null);
    setOpenCell({
      personId,
      dayIndex,
      placement: {
        alignRight: rect.left + 330 > window.innerWidth,
        openUp: rect.bottom + MENU_HEIGHT_GUESS > window.innerHeight && rect.top > MENU_HEIGHT_GUESS,
      },
    });
  };

  const startLeave = (person: Person, dayIndex: number, leaveShift: ShiftType) => {
    setOpenCell(null);
    setLeave({ personId: person.id, dayIndex, leaveShiftId: leaveShift.id, picked: 0, note: '' });
  };

  const confirmLeave = () => {
    if (!leave || !planResult) return;
    const plan = planResult.plans[leave.picked];
    if (!plan) return;
    const person = view.peopleById.get(leave.personId)!;
    const leaveName = view.shiftsById.get(leave.leaveShiftId)?.name ?? '调休';
    const title = `${label(person, leave.dayIndex)} ${leaveName}（方案 ${leave.picked + 1}，${plan.steps.length} 步）`;
    commitChange('leave', title, plan.changes, leave.note);
    setLeave(null);
    setToast(`已执行：${title}，记为 1 条修改`);
  };

  const groups = view.groups.filter((g) => levelFilter === 'all' || g.level.id === levelFilter);
  const gridStyle = { '--days': view.dates.length } as CSSProperties;
  const leavePerson = leave ? view.peopleById.get(leave.personId) : null;
  // 撤销等操作可能让申请人当天已经没有班了，这时就不再显示方案浮窗
  const leaveTarget = leave ? view.shiftOfCell(view.cells[cellKey(leave.personId, view.dates[leave.dayIndex])]) : null;

  return (
    <>
      {openCell && <button className="scrim" aria-label="关闭菜单" onClick={() => setOpenCell(null)} />}

      <div className="menu-row" style={{ gap: 8 }}>
        <button className={'chip' + (levelFilter === 'all' ? ' on' : '')} onClick={() => setLevelFilter('all')}>
          全部人员
        </button>
        {view.groups.map((g) => (
          <button key={g.level.id} className={'chip' + (levelFilter === g.level.id ? ' on' : '')} onClick={() => setLevelFilter(g.level.id)}>
            {g.level.name}
          </button>
        ))}
        <div className="spacer" />
        <button className={'chip mark' + (onlyModified ? ' on' : '')} onClick={() => setOnlyModified(!onlyModified)} aria-pressed={onlyModified}>
          <span className="mark-triangle" />
          只看改过的格子
        </button>
        <span className="muted">点任意格子：调休 / 换班次 / 删除 / 自定义 / 看记录</span>
      </div>

      {toast && (
        <div className="toast">
          <span style={{ width: 8, height: 8, borderRadius: 4, background: '#34d399' }} />
          <span>{toast}</span>
          <div className="spacer" />
          <button
            className="btn"
            style={{ height: 28, background: 'transparent', color: '#fff', borderColor: '#475467' }}
            onClick={() => {
              undo();
              setToast('已撤销上一步');
            }}
          >
            撤销
          </button>
          <button className="btn icon" style={{ background: 'transparent', color: '#fff', borderColor: '#475467' }} aria-label="关闭提示" onClick={() => setToast('')}>
            ✕
          </button>
        </div>
      )}

      <div className="grid" style={gridStyle}>
        <div style={{ minWidth: 960 }}>
          <div className="grid-row grid-head">
            <div className="hc">序号</div>
            <div className="hc">级别</div>
            <div className="hc">姓名</div>
            {view.dates.map((d) => (
              <div key={d} className={'hc' + (isWeekend(d) ? ' weekend' : '')}>
                <span>{shortDate(d)}</span>
                <span>{weekdayName(d)}</span>
              </div>
            ))}
          </div>

          {groups.map((g) => (
            <div key={g.level.id}>
              <div className="group-row">
                {g.level.name}（{g.level.shortName}）· {g.people.length} 人
              </div>
              {g.people.map((person) => (
                <div key={person.id} className="grid-row">
                  <div className="fx">{view.rowNumber.get(person.id)}</div>
                  <div className="fx">{g.level.shortName}</div>
                  <div className="fx name" style={g.level.nameColor ? { color: g.level.nameColor } : undefined}>
                    {person.name}
                    {person.status !== 'active' && <span className="person-tag">{person.status === 'noRotate' ? '不轮班' : '冻结'}</span>}
                  </div>
                  {view.dates.map((date, dayIndex) => {
                    const key = cellKey(person.id, date);
                    const value = view.cells[key] ?? null;
                    const shift = view.shiftOfCell(value);
                    const text = describeCell(value, view.shiftsById);
                    const isModified = view.modified.has(key);
                    const role = isModified ? view.roles[key] : undefined;
                    const previewText = preview.get(key);
                    const isOpen = openCell?.personId === person.id && openCell.dayIndex === dayIndex;
                    const isLeaveSource = leave?.personId === person.id && leave.dayIndex === dayIndex;

                    let cls = 'cell ';
                    if (previewText) {
                      cls += 's-preview';
                    } else {
                      cls += value === null ? 'k-empty' : shift ? 'k-' + shift.category : 'k-custom';
                      if (role === 'requester') cls += ' s-requester';
                      if (role === 'cover') cls += ' s-cover';
                    }
                    if (isModified) cls += ' s-modified';
                    else if (onlyModified) cls += ' s-dim';
                    if (isOpen || (isLeaveSource && !previewText)) cls += ' s-selected';

                    return (
                      <div key={date} className="cell-wrap">
                        <button
                          className={cls}
                          title={isModified ? `${text} · 本轮改过，点开看记录` : text || '空白格'}
                          aria-label={`${person.name} ${shortDate(date)} ${text || '空白'}${isModified ? '，有修改记录' : ''}`}
                          onClick={(e) => onCellClick(e, person.id, dayIndex)}
                        >
                          {previewText ?? text}
                        </button>
                        {isOpen && (
                          <CellMenu
                            data={data}
                            view={view}
                            person={person}
                            dayIndex={dayIndex}
                            placement={openCell.placement}
                            onStartLeave={(s) => startLeave(person, dayIndex, s)}
                            onPickShift={(s) => commitCell('manual', person, dayIndex, { shiftId: s.id }, `${label(person, dayIndex)} 改为 ${s.name}`)}
                            onSetCustom={(t) => commitCell('custom', person, dayIndex, { custom: t }, `${label(person, dayIndex)} 填写“${t}”`)}
                            onDelete={() => commitCell('delete', person, dayIndex, null, `${label(person, dayIndex)} 删除班次`)}
                            onOpenHistory={() => {
                              setOpenCell(null);
                              openCellHistory(person.id, date);
                            }}
                          />
                        )}
                      </div>
                    );
                  })}
                </div>
              ))}
            </div>
          ))}
        </div>
      </div>

      <div className="legend">
        <div className="legend-item"><span className="swatch" style={{ background: '#fff' }} />普通班</div>
        <div className="legend-item"><span className="swatch" style={{ background: '#e7f4ec' }} />特定 / 特殊班</div>
        <div className="legend-item"><span className="swatch" style={{ background: '#e8eefa' }} />药师班 A–T</div>
        <div className="legend-item"><span className="swatch" style={{ background: '#f0eafa' }} />3(10)</div>
        <div className="legend-item"><span className="swatch" style={{ background: '#334155' }} />夜班 / 帮夜</div>
        <div className="legend-item">
          <span className="swatch" style={{ background: '#fff' }}>
            <span className="mark-triangle" style={{ position: 'absolute', top: 0, right: 0 }} />
          </span>
          右上角橙色三角 = 本轮改过（点开可看记录）
        </div>
        <div className="legend-item"><span style={{ color: 'var(--relax)', fontWeight: 700 }}>蓝字</span>= 调休</div>
        <div className="legend-item"><span style={{ color: 'var(--danger)', fontWeight: 700 }}>红字</span>= 因联动被改</div>
        <div className="legend-item"><span className="swatch" style={{ background: '#fbfcfd' }} />空白格（可接任何班）</div>
      </div>

      {leave && leavePerson && leaveTarget && planResult && (
        <PlanPanel
          view={view}
          requester={leavePerson}
          dayIndex={leave.dayIndex}
          target={leaveTarget}
          leaveShifts={leaveShifts}
          leaveShift={view.shiftsById.get(leave.leaveShiftId)!}
          result={planResult}
          picked={leave.picked}
          note={leave.note}
          onPick={(i) => setLeave({ ...leave, picked: i })}
          onChangeLeave={(s) => setLeave({ ...leave, leaveShiftId: s.id })}
          onChangeNote={(note) => setLeave({ ...leave, note })}
          onConfirm={confirmLeave}
          onClose={() => setLeave(null)}
        />
      )}
    </>
  );
}
