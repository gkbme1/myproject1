import { useEffect, useMemo, useState } from 'react';
import type { CSSProperties, MouseEvent } from 'react';
import { isWeekend, shortDate, weekdayName } from '../../domain/dates';
import { findLeavePlans, findReturnOptions } from '../../domain/plans';
import { violationText } from '../../domain/rulesEngine';
import { describeCell, lastAppliedIndex } from '../../domain/schedule';
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
  openedAt: number;
}

interface LeaveSession {
  personId: string;
  dayIndex: number;
  leaveShiftId: string;
  picked: number;
  note: string;
  returnChoice: number | 'pending'; // 需还班时：选第几个还班日期，或者先记为待还班
}

export function ScheduleGrid({ data, view }: { data: AppData; view: ScheduleView }) {
  const commitChange = useAppStore((s) => s.commitChange);
  const undo = useAppStore((s) => s.undo);
  const openCellHistory = useAppStore((s) => s.openCellHistory);

  const [levelFilter, setLevelFilter] = useState<LevelId | 'all'>('all');
  const [onlyModified, setOnlyModified] = useState(false);
  const [onlyModifiedPeople, setOnlyModifiedPeople] = useState(false);
  const [openCell, setOpenCell] = useState<OpenCell | null>(null);
  const [leave, setLeave] = useState<LeaveSession | null>(null);
  const [toast, setToast] = useState('');
  const [showViolations, setShowViolations] = useState(false);
  const [focusKey, setFocusKey] = useState('');

  // 提示条几秒后自动消失，不一直占着地方
  useEffect(() => {
    if (!toast) return;
    const timer = setTimeout(() => setToast(''), 6000);
    return () => clearTimeout(timer);
  }, [toast]);

  const leaveShifts = useMemo(() => data.shifts.filter((s) => s.category === 'leave').sort((a, b) => a.order - b.order), [data.shifts]);

  const planResult = useMemo(() => {
    if (!leave) return null;
    return findLeavePlans({ ctx: view.ruleCtx, requesterId: leave.personId, dayIndex: leave.dayIndex, leaveShiftId: leave.leaveShiftId });
    // 换方案、改备注不需要重新查找
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [view, leave?.personId, leave?.dayIndex, leave?.leaveShiftId]);

  const pickedPlan = planResult?.plans[leave?.picked ?? 0] ?? null;
  const leaveTargetShift = leave ? view.shiftOfCell(view.cells[cellKey(leave.personId, view.dates[leave.dayIndex])]) : null;
  // 3-4 / 3-5 / 3-7 这类需还班的班次：申请人在另一天替接班人上一个班
  const returnOptions = useMemo(() => {
    if (!leave || !pickedPlan || !leaveTargetShift?.needReturn) return [];
    return findReturnOptions(view.ruleCtx, pickedPlan.changes, leave.personId, pickedPlan.steps[0].personId, view.dates[leave.dayIndex]);
  }, [view, leave, pickedPlan, leaveTargetShift]);
  const pickedReturn = leave && typeof leave.returnChoice === 'number' ? returnOptions[leave.returnChoice] ?? null : null;

  // 方案预览：选中方案会变动的格子，key -> "原→新"
  const preview = useMemo(() => {
    const map = new Map<string, string>();
    if (!pickedPlan) return map;
    for (const ch of [...pickedPlan.changes, ...(pickedReturn?.changes ?? [])]) {
      map.set(cellKey(ch.personId, ch.date), `${describeCell(ch.from, view.shiftsById) || '空'}→${describeCell(ch.to, view.shiftsById)}`);
    }
    return map;
  }, [pickedPlan, pickedReturn, view.shiftsById]);

  const label = (person: Person, dayIndex: number) => `${person.name} ${shortDate(view.dates[dayIndex])}`;

  const commitCell = (kind: 'manual' | 'delete' | 'custom', person: Person, dayIndex: number, to: CellValue, title: string, warnings: string[] = []) => {
    const date = view.dates[dayIndex];
    commitChange(kind, title, [{ personId: person.id, date, from: view.cells[cellKey(person.id, date)] ?? null, to, role: 'manual' }], { warnings });
    setOpenCell(null);
    setToast(warnings.length > 0 ? `已保存：${title}。提示：${warnings.join('；')}` : `已保存：${title}`);
  };

  const locate = (key: string) => {
    setFocusKey(key);
    document.querySelector(`[data-cell="${key}"]`)?.scrollIntoView({ block: 'center', inline: 'center', behavior: 'smooth' });
  };

  const onCellClick = (e: MouseEvent<HTMLButtonElement>, personId: string, dayIndex: number) => {
    const rect = e.currentTarget.getBoundingClientRect();
    setLeave(null);
    setOpenCell({ personId, dayIndex, placement: { top: rect.top, bottom: rect.bottom, left: rect.left, right: rect.right }, openedAt: Date.now() });
  };

  const startLeave = (person: Person, dayIndex: number, leaveShift: ShiftType) => {
    setOpenCell(null);
    setLeave({ personId: person.id, dayIndex, leaveShiftId: leaveShift.id, picked: 0, note: '', returnChoice: 0 });
  };

  const confirmLeave = () => {
    if (!leave || !pickedPlan || !leaveTargetShift) return;
    const person = view.peopleById.get(leave.personId)!;
    const leaveName = view.shiftsById.get(leave.leaveShiftId)?.name ?? '调休';
    const needReturn = leaveTargetShift.needReturn;
    const coverId = pickedPlan.steps[0].personId;
    let title = `${label(person, leave.dayIndex)} ${leaveName}（方案 ${leave.picked + 1}，${pickedPlan.steps.length} 步）`;
    if (needReturn) title += pickedReturn ? `，${shortDate(pickedReturn.date)} 还班` : '，待还班';
    commitChange('leave', title, [...pickedPlan.changes, ...(pickedReturn?.changes ?? [])], {
      note: leave.note,
      warnings: [...pickedPlan.warnings, ...(pickedReturn?.warnings ?? [])],
      leave: {
        personId: leave.personId,
        date: view.dates[leave.dayIndex],
        leaveShiftId: leave.leaveShiftId,
        fromShiftId: leaveTargetShift.id,
        coverPersonId: coverId,
        returnStatus: !needReturn ? 'none' : pickedReturn ? 'done' : 'pending',
        ...(pickedReturn ? { returnDate: pickedReturn.date } : {}),
      },
    });
    setLeave(null);
    setToast(`已执行：${title}，记为 1 条修改`);
  };

  const groups = view.groups.filter((g) => levelFilter === 'all' || g.level.id === levelFilter);
  const gridStyle = { '--days': view.dates.length } as CSSProperties;
  const leavePerson = leave ? view.peopleById.get(leave.personId) : null;
  // 撤销等操作可能让申请人当天已经没有班了，这时就不再显示方案浮窗
  const leaveTarget = leaveTargetShift;

  // 有改动的人（整行标出来）和最近一次操作动到的格子（闪几下并加深色框）
  const modifiedPeople = new Set(Array.from(view.modified).map((k) => k.split('|')[0]));
  const lastIdx = lastAppliedIndex(view.changes);
  const latestKeys = new Set(lastIdx >= 0 ? view.changes[lastIdx].changes.map((c) => cellKey(c.personId, c.date)) : []);
  const visibleGroups = groups
    .map((g) => ({ ...g, people: onlyModifiedPeople ? g.people.filter((p) => modifiedPeople.has(p.id)) : g.people }))
    .filter((g) => g.people.length > 0);

  return (
    <div className="grid-page">
      {openCell && <button className="scrim" aria-label="关闭菜单" onClick={() => setOpenCell(null)} />}

      <div className="grid-toolbar">
        <button className={'chip' + (levelFilter === 'all' ? ' on' : '')} onClick={() => setLevelFilter('all')}>
          全部
        </button>
        {view.groups.map((g) => (
          <button key={g.level.id} className={'chip' + (levelFilter === g.level.id ? ' on' : '')} onClick={() => setLevelFilter(g.level.id)}>
            {g.level.name}
          </button>
        ))}
        <span className="toolbar-sep" />
        <button className={'chip mark' + (onlyModifiedPeople ? ' on' : '')} onClick={() => setOnlyModifiedPeople(!onlyModifiedPeople)} aria-pressed={onlyModifiedPeople}>
          只看改过的人（{modifiedPeople.size}）
        </button>
        <button className={'chip mark' + (onlyModified ? ' on' : '')} onClick={() => setOnlyModified(!onlyModified)} aria-pressed={onlyModified}>
          淡化没改的格子
        </button>
        <span className="toolbar-sep" />
        <button className="stat-button" onClick={() => setShowViolations(!showViolations)} title="点开看具体是哪些人、哪几天">
          违规 <b style={{ color: view.hardCount > 0 ? 'var(--danger)' : '#15803d' }}>{view.hardCount}</b>　提示 <b style={{ color: '#b45309' }}>{view.softCount}</b>
        </button>
        <div className="spacer" />
        <span className="legend-item"><span className="swatch s-mod-manual" />手工改过</span>
        <span className="legend-item"><span className="swatch s-requester" />调休</span>
        <span className="legend-item"><span className="swatch s-cover" />被联动改班</span>
        <span className="legend-item"><span className="swatch" style={{ outline: '2px solid #1b2430', outlineOffset: -2 }} />最近一次</span>
        <span className="legend-item"><span className="swatch k-empty" />空白格</span>
      </div>

      <div
        className="grid"
        style={gridStyle}
        onScroll={() => {
          // 表格滚动后菜单就对不上格子了，关掉；刚点开时浏览器把格子滚进视野的那一下不算
          if (openCell && Date.now() - openCell.openedAt > 300) setOpenCell(null);
        }}
      >
        <div className="grid-inner">
          <div className="grid-row grid-head">
            <div className="hc">序号</div>
            <div className="hc">姓名</div>
            {view.dates.map((d) => (
              <div key={d} className={'hc' + (isWeekend(d) ? ' weekend' : '')}>
                {shortDate(d)} {weekdayName(d).slice(1)}
              </div>
            ))}
          </div>

          {visibleGroups.map((g) => (
            <div key={g.level.id}>
              <div className="group-row">
                {g.level.name}（{g.level.shortName}）· {g.people.length} 人
              </div>
              {g.people.map((person) => (
                <div key={person.id} className={'grid-row' + (modifiedPeople.has(person.id) ? ' row-modified' : '')}>
                  <div className="fx">{view.rowNumber.get(person.id)}</div>
                  <div className="fx name" style={g.level.nameColor ? { color: g.level.nameColor } : undefined} title={g.level.name}>
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
                    } else if (isModified) {
                      // 改过的格子整格上色，一眼能看出谁被调整了
                      cls += role === 'requester' ? 's-requester' : role === 'cover' ? 's-cover' : 's-mod-manual';
                    } else {
                      cls += value === null ? 'k-empty' : shift ? 'k-' + shift.category : 'k-custom';
                      if (onlyModified) cls += ' s-dim';
                    }
                    const cellVs = view.cellViolations.get(key);
                    if (cellVs && !previewText) cls += cellVs.some((v) => v.severity === 'hard') ? ' v-hard' : ' v-soft';
                    if ((latestKeys.has(key) || focusKey === key) && !previewText) cls += ' s-latest';
                    if (isOpen || (isLeaveSource && !previewText)) cls += ' s-selected';

                    return (
                      <div key={date} className="cell-wrap">
                        <button
                          className={cls}
                          data-cell={key}
                          title={[isModified ? `${text} · 本轮改过，点开看记录` : text || '空白格', ...(cellVs ?? []).map((v) => v.message)].join('\n')}
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
                            onPickShift={(s, warns) => commitCell('manual', person, dayIndex, { shiftId: s.id }, `${label(person, dayIndex)} 改为 ${s.name}`, warns)}
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
          {visibleGroups.length === 0 && <div className="muted" style={{ padding: 24, textAlign: 'center' }}>本轮还没有改过的人</div>}
        </div>
      </div>

      {showViolations && (
        <section className="violation-panel" aria-label="规则检查结果">
          <div className="plan-panel-head" style={{ cursor: 'default' }}>
            <strong style={{ fontSize: 14 }}>规则检查：违规 {view.hardCount}，提示 {view.softCount}</strong>
            <div className="spacer" />
            <button className="btn icon" aria-label="关闭" onClick={() => setShowViolations(false)}>
              ✕
            </button>
          </div>
          <div style={{ overflowY: 'auto', flex: 1 }}>
            {view.violations.length === 0 && <div className="muted" style={{ padding: 16 }}>没有发现问题</div>}
            {[...view.violations]
              .sort((a, b) => (a.severity === b.severity ? 0 : a.severity === 'hard' ? -1 : 1))
              .map((v, i) => (
                <button
                  key={i}
                  className="violation-item"
                  onClick={() => v.personId && v.dates[0] && locate(cellKey(v.personId, v.dates[0]))}
                  title={v.personId ? '点击定位到表格里的格子' : undefined}
                >
                  <span className={'badge ' + (v.severity === 'hard' ? 'red' : 'warn')} style={{ marginRight: 6 }}>
                    {v.severity === 'hard' ? '违规' : '提示'}
                  </span>
                  {violationText(v, view.peopleById)}
                  {v.dates.length > 0 && <span className="muted"> · {v.dates.slice(0, 4).map(shortDate).join('、')}{v.dates.length > 4 ? '…' : ''}</span>}
                </button>
              ))}
          </div>
          <div className="muted" style={{ padding: '8px 12px', borderTop: '1px solid var(--line-soft)' }}>
            “违规”的规则在改班时会拦截；“提示”只提醒。可以在左侧“规则”里调整。
          </div>
        </section>
      )}

      {toast && (
        <div className="toast">
          <span style={{ width: 8, height: 8, borderRadius: 4, background: '#34d399' }} />
          <span>{toast}</span>
          <button
            className="btn"
            style={{ height: 26, background: 'transparent', color: '#fff', borderColor: '#475467' }}
            onClick={() => {
              undo();
              setToast('已撤销上一步');
            }}
          >
            撤销
          </button>
          <button className="btn icon" style={{ height: 26, width: 26, background: 'transparent', color: '#fff', borderColor: '#475467' }} aria-label="关闭提示" onClick={() => setToast('')}>
            ✕
          </button>
        </div>
      )}

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
          returnOptions={returnOptions}
          needReturn={!!leaveTarget.needReturn}
          returnChoice={leave.returnChoice}
          onPickReturn={(c) => setLeave({ ...leave, returnChoice: c })}
          onPick={(i) => setLeave({ ...leave, picked: i, returnChoice: 0 })}
          onChangeLeave={(s) => setLeave({ ...leave, leaveShiftId: s.id })}
          onChangeNote={(note) => setLeave({ ...leave, note })}
          onConfirm={confirmLeave}
          onClose={() => setLeave(null)}
        />
      )}
    </div>
  );
}
