import { useMemo, useState } from 'react';
import { nextRoundDates } from '../../domain/applications';
import { formatDateTime, shortDate, weekdayName } from '../../domain/dates';
import type { AppData, LeaveApplication } from '../../domain/types';
import { levelLabel } from '../../domain/types';
import { useAppStore } from '../../store/appStore';

const STATUS_TEXT = { pending: '待处理', approved: '已安排', rejected: '已拒绝' } as const;

// 下一轮调休申请表：按日期登记，生成下一轮时按登记先后自动安排
export function ApplicationsView({ data }: { data: AppData }) {
  const addApplication = useAppStore((s) => s.addApplication);
  const removeApplication = useAppStore((s) => s.removeApplication);
  const clearApplications = useAppStore((s) => s.clearApplications);
  const setApplicationType = useAppStore((s) => s.setApplicationType);

  const apps = useMemo(() => [...(data.leaveApplications ?? [])].sort((a, b) => a.registeredAt.localeCompare(b.registeredAt)), [data.leaveApplications]);
  const nextDates = nextRoundDates(data);
  const [name, setName] = useState('');
  const [date, setDate] = useState(nextDates[0]);
  const [note, setNote] = useState('');

  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const leaveShifts = data.shifts.filter((s) => s.category === 'leave').sort((a, b) => a.order - b.order);
  const levels = [...data.levels].sort((a, b) => a.order - b.order);
  const order = new Map(apps.map((a, i) => [a.id, i + 1]));
  const person = data.people.find((p) => p.name === name.trim());
  const duplicate = !!person && apps.some((a) => a.personId === person.id && a.date === date && a.status === 'pending');

  // 表格的行：下一轮的 14 天，加上以前登记过、还没清除的日期
  const rows = Array.from(new Set([...nextDates, ...apps.map((a) => a.date)])).sort();
  const changeById = new Map(data.changes.map((c) => [c.id, c]));
  const undone = (a: LeaveApplication) => a.status === 'approved' && !!a.changeSetId && !!changeById.get(a.changeSetId)?.undone;

  const chipStyle = (a: LeaveApplication) => {
    if (undone(a)) return { background: '#f2f4f7', borderColor: '#d0d5dd', color: 'var(--muted)', textDecoration: 'line-through' };
    if (a.status === 'approved') return { background: '#2f6feb', borderColor: '#2f6feb', color: '#fff' };
    if (a.status === 'rejected') return { background: '#e5484d', borderColor: '#e5484d', color: '#fff' };
    return { background: '#fff', borderColor: '#98a2b3', color: 'var(--ink)' };
  };

  const register = () => {
    if (!person || duplicate) return;
    addApplication(person.id, date, note.trim());
    setName('');
    setNote('');
  };

  const counts = { pending: 0, approved: 0, rejected: 0 };
  apps.forEach((a) => counts[a.status]++);

  return (
    <>
      <section className="card" style={{ padding: '14px 16px', display: 'flex', flexDirection: 'column', gap: 10 }}>
        <div className="menu-row" style={{ gap: 14 }}>
          <h2 style={{ margin: 0, fontSize: 16 }}>下一轮调休申请表</h2>
          <span className="muted" style={{ fontSize: 13 }}>
            下一轮 {shortDate(nextDates[0])} ~ {shortDate(nextDates[nextDates.length - 1])}。点“生成下一轮”时按登记先后自动安排：能安排的蓝底，安排不了的红底并写明原因。
          </span>
          <div className="spacer" />
          <span style={{ fontSize: 13 }}>
            待处理 <b>{counts.pending}</b>　已安排 <b style={{ color: '#2f6feb' }}>{counts.approved}</b>　已拒绝 <b style={{ color: '#e5484d' }}>{counts.rejected}</b>
          </span>
          <button
            className="btn"
            disabled={apps.length === 0}
            onClick={() => window.confirm('清除全部申请记录（包括已安排和已拒绝的）？\n已经安排进排班表的调休不受影响。请先确认剩余假期已统计完。') && clearApplications()}
          >
            清除全部记录
          </button>
        </div>
        <form
          className="menu-row"
          onSubmit={(e) => {
            e.preventDefault();
            register();
          }}
        >
          <input className="input" list="apply-people" placeholder="申请人姓名" aria-label="申请人" value={name} onChange={(e) => setName(e.target.value)} style={{ width: 130 }} />
          <datalist id="apply-people">
            {levels.flatMap((l) =>
              data.people
                .filter((p) => p.levelId === l.id && p.status !== 'frozen')
                .map((p) => (
                  <option key={p.id} value={p.name}>
                    {levelLabel(l)}
                  </option>
                )),
            )}
          </datalist>
          <select className="input" value={date} onChange={(e) => setDate(e.target.value)} aria-label="调休日期">
            {nextDates.map((d) => (
              <option key={d} value={d}>
                {shortDate(d)} {weekdayName(d)}
              </option>
            ))}
          </select>
          <input className="input" placeholder="备注（可选）" aria-label="备注" value={note} onChange={(e) => setNote(e.target.value)} style={{ width: 200 }} />
          <button className="btn primary" type="submit" disabled={!person || duplicate}>
            登记
          </button>
          {name.trim() && !person && <span className="muted" style={{ color: 'var(--danger)' }}>没有这个人</span>}
          {duplicate && <span className="muted" style={{ color: 'var(--danger)' }}>这个人这一天已经登记过</span>}
        </form>
      </section>

      <section className="card" style={{ overflowX: 'auto' }}>
        <div style={{ minWidth: 700 }}>
          <div className="table-row table-head" style={{ gridTemplateColumns: '110px 1fr' }}>
            <span>日期</span>
            <span>申请人（按登记先后，数字是登记序号）</span>
          </div>
          {rows.map((d) => {
            const list = apps.filter((a) => a.date === d);
            return (
              <div key={d} className="table-row" style={{ gridTemplateColumns: '110px 1fr', minHeight: 36, background: nextDates.includes(d) ? '#fff' : '#f7f9fb' }}>
                <span style={{ fontWeight: 500 }}>
                  {shortDate(d)} {weekdayName(d)}
                </span>
                <span className="menu-row" style={{ gap: 6, padding: '4px 8px' }}>
                  {list.length === 0 && <span className="muted">—</span>}
                  {list.map((a) => (
                    <span
                      key={a.id}
                      className="chip"
                      style={{ ...chipStyle(a), cursor: 'default' }}
                      title={a.status === 'approved' ? a.summary : a.status === 'rejected' ? a.reason : '待生成下一轮时处理'}
                    >
                      <b style={{ fontSize: 11, opacity: 0.85 }}>{order.get(a.id)}</b>
                      {peopleById.get(a.personId)?.name}
                    </span>
                  ))}
                </span>
              </div>
            );
          })}
        </div>
      </section>

      {apps.length > 0 && (
        <section className="card" style={{ overflowX: 'auto' }}>
          <div style={{ minWidth: 1000 }}>
            <div className="table-row table-head" style={{ gridTemplateColumns: '48px 80px 100px 80px 110px 1fr 110px 60px' }}>
              <span>序号</span>
              <span>申请人</span>
              <span>日期</span>
              <span>状态</span>
              <span>调休类型</span>
              <span>安排结果 / 拒绝原因</span>
              <span>登记时间</span>
              <span />
            </div>
            {apps.map((a) => (
              <div
                key={a.id}
                className="table-row"
                style={{
                  gridTemplateColumns: '48px 80px 100px 80px 110px 1fr 110px 60px',
                  fontSize: 13,
                  background: undone(a) ? '#f7f9fb' : a.status === 'approved' ? '#e8f0fe' : a.status === 'rejected' ? '#fdecec' : '#fff',
                }}
              >
                <span>{order.get(a.id)}</span>
                <span style={{ fontWeight: 500 }}>{peopleById.get(a.personId)?.name}</span>
                <span>
                  {shortDate(a.date)} {weekdayName(a.date)}
                </span>
                <span style={{ fontWeight: 700, color: a.status === 'approved' ? '#1d4ed8' : a.status === 'rejected' ? '#b42318' : 'var(--ink-2)' }}>
                  {undone(a) ? '已撤销' : STATUS_TEXT[a.status]}
                </span>
                <span>
                  {a.status === 'approved' && !undone(a) ? (
                    <select className="input" style={{ height: 28 }} value={a.leaveShiftId} onChange={(e) => setApplicationType(a.id, e.target.value)} aria-label="调休类型">
                      {leaveShifts.map((s) => (
                        <option key={s.id} value={s.id}>
                          {s.name}
                        </option>
                      ))}
                    </select>
                  ) : (
                    <span className="muted">{a.leaveShiftId ? shiftsById.get(a.leaveShiftId)?.name : '—'}</span>
                  )}
                </span>
                <span style={{ fontSize: 12, lineHeight: 1.6, padding: '4px 8px' }}>
                  {a.status === 'approved' ? a.summary : a.status === 'rejected' ? a.reason : a.note || <span className="muted">生成下一轮时处理</span>}
                </span>
                <span className="muted">{formatDateTime(a.registeredAt)}</span>
                <span>
                  {a.status === 'pending' && (
                    <button className="btn" style={{ height: 26 }} onClick={() => removeApplication(a.id)}>
                      删除
                    </button>
                  )}
                </span>
              </div>
            ))}
          </div>
        </section>
      )}
    </>
  );
}
