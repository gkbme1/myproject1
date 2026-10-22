import { levelLabel } from '../domain/types';
import { useState } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import { newId } from '../domain/schedule';
import type { AppData, ShiftCategory, ShiftType, TimeSlot } from '../domain/types';
import { useAppStore } from '../store/appStore';

export const CATEGORY_NAMES: Record<ShiftCategory, string> = {
  normal: '普通班',
  special: '特殊班',
  exclusive: '特定班',
  pharmacist: '药师班',
  fixedGroup: '固定小组',
  night: '夜班',
  off: '休息',
  leave: '调休类',
  custom: '自定义',
};

const CATEGORY_COLORS: Record<ShiftCategory, string> = {
  normal: '#ffffff',
  special: '#e7f4ec',
  exclusive: '#c9e8d4',
  pharmacist: '#e8eefa',
  fixedGroup: '#f0eafa',
  night: '#334155',
  off: '#ffffff',
  leave: '#d6e6fb',
  custom: '#f2f4f7',
};

const TIME_NAMES: Record<TimeSlot, string> = { morning: '早', noon: '中', night: '夜', none: '—' };

const COLUMNS = '28px 90px 110px 56px 64px 64px repeat(4, minmax(70px, 1fr))';

export function ShiftsPage({ data }: { data: AppData }) {
  const updateShift = useAppStore((s) => s.updateShift);
  const addShift = useAppStore((s) => s.addShift);
  const [newName, setNewName] = useState('');
  const levels = [...data.levels].sort((a, b) => a.order - b.order);
  const shifts = [...data.shifts].sort((a, b) => a.order - b.order);

  const toggleLevel = (shift: ShiftType, levelId: string) => {
    const has = shift.allowedLevels.includes(levelId);
    updateShift(shift.id, {
      allowedLevels: has ? shift.allowedLevels.filter((l) => l !== levelId) : [...shift.allowedLevels, levelId],
    });
  };

  const create = () => {
    const name = newName.trim();
    if (!name || data.shifts.some((s) => s.name === name)) return;
    addShift({
      id: newId('s'),
      name,
      category: 'custom',
      timeSlot: 'noon',
      canTakeLeave: true,
      needReturn: false,
      allowedLevels: [],
      order: Math.max(...data.shifts.map((s) => s.order)) + 1,
      builtIn: false,
    });
    setNewName('');
  };

  const duplicate = data.shifts.some((s) => s.name === newName.trim());

  return (
    <>
      <header className="page-header">
        <h1>班次</h1>
        <span className="muted">勾选 = 该级别可以上这个班次。改动立即生效并自动保存</span>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <form
          className="menu-row"
          onSubmit={(e) => {
            e.preventDefault();
            create();
          }}
        >
          <h2 style={{ margin: 0, fontSize: 16 }}>班次定义与级别权限</h2>
          <div className="spacer" />
          <input className="input" placeholder="新班次名称，如 Z菌" value={newName} onChange={(e) => setNewName(e.target.value)} aria-label="新班次名称" />
          <button className="btn primary" type="submit" disabled={!newName.trim() || duplicate}>
            ＋ 新增班次
          </button>
          {duplicate && <span className="muted" style={{ color: 'var(--danger)' }}>已有同名班次</span>}
        </form>

        <div className="card" style={{ overflowX: 'auto' }}>
          <div style={{ minWidth: 860 }}>
            <div className="table-row table-head" style={{ gridTemplateColumns: COLUMNS }}>
              <span />
              <span>班次</span>
              <span>分类</span>
              <span className="center">时段</span>
              <span className="center">可调休</span>
              <span className="center">需还班</span>
              {levels.map((l) => (
                <span key={l.id} className="center">
                  {levelLabel(l)}
                </span>
              ))}
            </div>
            {shifts.map((s) => (
              <div key={s.id} className="table-row" style={{ gridTemplateColumns: COLUMNS }}>
                <span>
                  <span className="swatch" style={{ display: 'inline-block', background: CATEGORY_COLORS[s.category] }} />
                </span>
                <span style={{ fontWeight: 500 }}>
                  {s.name}
                  {!s.builtIn && <span className="badge blue" style={{ marginLeft: 4, fontSize: 10 }}>自定义</span>}
                </span>
                <span>
                  <span className="badge gray">{CATEGORY_NAMES[s.category]}</span>
                </span>
                <span className="center">{TIME_NAMES[s.timeSlot]}</span>
                <span className="center">
                  <input type="checkbox" checked={s.canTakeLeave} onChange={() => updateShift(s.id, { canTakeLeave: !s.canTakeLeave })} aria-label={`${s.name} 可调休`} />
                </span>
                <span className="center">
                  <input type="checkbox" checked={s.needReturn} onChange={() => updateShift(s.id, { needReturn: !s.needReturn })} aria-label={`${s.name} 需还班`} />
                </span>
                {levels.map((l) => (
                  <span key={l.id} className="center">
                    <input type="checkbox" checked={s.allowedLevels.includes(l.id)} onChange={() => toggleLevel(s, l.id)} aria-label={`${levelLabel(l)}可上${s.name}`} />
                  </span>
                ))}
              </div>
            ))}
          </div>
        </div>
      </div>
    </>
  );
}
