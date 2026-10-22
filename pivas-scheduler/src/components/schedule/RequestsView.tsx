import { formatDateTime, shortDate, weekdayName } from '../../domain/dates';
import type { ChangeSet } from '../../domain/types';
import { useAppStore } from '../../store/appStore';
import type { ScheduleView } from './useScheduleView';

const RETURN_TEXT = { none: '不需要还班', done: '已安排还班', pending: '待还班' } as const;
const RETURN_BADGE = { none: 'gray', done: 'ok', pending: 'warn' } as const;

// 调休申请记录：本轮每一次调休 / 补休，谁接的班、要不要还班
export function RequestsView({ view }: { view: ScheduleView }) {
  const openCellHistory = useAppStore((s) => s.openCellHistory);
  const leaves = view.changes.filter((c): c is ChangeSet & { leave: NonNullable<ChangeSet['leave']> } => !c.undone && !!c.leave);
  const name = (id: string | null) => (id ? view.peopleById.get(id)?.name ?? '?' : '—');
  const shift = (id: string) => view.shiftsById.get(id)?.name ?? '?';
  const pending = leaves.filter((c) => c.leave.returnStatus === 'pending');

  const perPerson = new Map<string, number>();
  leaves.forEach((c) => perPerson.set(c.leave.personId, (perPerson.get(c.leave.personId) ?? 0) + 1));

  return (
    <>
      <section className="card" style={{ padding: '14px 16px', display: 'flex', gap: 24, alignItems: 'center', flexWrap: 'wrap' }}>
        <span style={{ fontSize: 14 }}>
          本轮调休 / 补休 <b>{leaves.length}</b> 次，涉及 <b>{perPerson.size}</b> 人
        </span>
        <span style={{ fontSize: 14, color: pending.length > 0 ? 'var(--warn)' : undefined }}>
          待还班 <b>{pending.length}</b> 次
        </span>
        {pending.length > 0 && <span className="muted">待还班的可以在排班表里给申请人手工安排，安排好后在这里核对。</span>}
      </section>

      <section className="card" style={{ overflowX: 'auto' }}>
        <div style={{ minWidth: 900 }}>
          <div className="table-row table-head" style={{ gridTemplateColumns: '90px 110px 80px 80px 90px 130px 1fr 110px' }}>
            <span>申请人</span>
            <span>日期</span>
            <span>类型</span>
            <span>原班次</span>
            <span>接班的人</span>
            <span>还班</span>
            <span>备注</span>
            <span>操作时间</span>
          </div>
          {leaves.length === 0 && <div className="muted" style={{ padding: 24, textAlign: 'center' }}>本轮还没有调休。在排班表里点格子选“调休…”即可。</div>}
          {[...leaves].reverse().map((c) => (
            <div key={c.id} className="table-row" style={{ gridTemplateColumns: '90px 110px 80px 80px 90px 130px 1fr 110px', fontSize: 13 }}>
              <span style={{ fontWeight: 500 }}>{name(c.leave.personId)}</span>
              <span>
                <button className="menu-link" style={{ padding: 0, color: 'var(--primary)' }} onClick={() => openCellHistory(c.leave.personId, c.leave.date)}>
                  {shortDate(c.leave.date)} {weekdayName(c.leave.date)}
                </button>
              </span>
              <span>{shift(c.leave.leaveShiftId)}</span>
              <span>{shift(c.leave.fromShiftId)}</span>
              <span>{name(c.leave.coverPersonId)}</span>
              <span>
                <span className={'badge ' + RETURN_BADGE[c.leave.returnStatus]}>
                  {RETURN_TEXT[c.leave.returnStatus]}
                  {c.leave.returnDate ? ` · ${shortDate(c.leave.returnDate)}` : ''}
                </span>
              </span>
              <span className="muted">{c.note || '—'}</span>
              <span className="muted">{formatDateTime(c.at)}</span>
            </div>
          ))}
        </div>
      </section>
    </>
  );
}
