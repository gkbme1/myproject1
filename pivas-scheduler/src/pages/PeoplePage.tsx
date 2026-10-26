import { useState } from 'react';
import type { KeyboardEvent } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import { getRound, newId } from '../domain/schedule';
import type { AppData, Person, PersonStatus, ShiftId } from '../domain/types';
import { levelLabel } from '../domain/types';
import { useAppStore } from '../store/appStore';

const STATUS_NAMES: Record<PersonStatus, string> = {
  active: '在岗',
  noRotate: '锁行',
  frozen: '冻结',
};
const STATUS_BADGE: Record<PersonStatus, string> = {
  active: 'ok',
  noRotate: 'warn',
  frozen: 'gray',
};
const STATUS_LONG: Record<PersonStatus, string> = {
  active: '在岗，正常轮班和联动',
  noRotate: '在岗，锁行（生成下一轮时留在原行），但参与联动换班',
  frozen: '冻结（暂时不排，如休产假）',
};

export function PeoplePage({ data }: { data: AppData }) {
  const updatePerson = useAppStore((s) => s.updatePerson);
  const addPerson = useAppStore((s) => s.addPerson);
  const setRoundPersonOrder = useAppStore((s) => s.setRoundPersonOrder);
  const round = getRound(data);
  const roundIds = round.personIds ?? [...data.people].sort((a, b) => a.order - b.order).map((p) => p.id);

  const [selectedId, setSelectedId] = useState<string>(roundIds[0] ?? data.people[0]?.id ?? '');
  const [search, setSearch] = useState('');
  const [newName, setNewName] = useState('');
  const [newLevel, setNewLevel] = useState(data.levels[0]?.id ?? 'L1');

  const levels = [...data.levels].sort((a, b) => a.order - b.order);
  const shifts = [...data.shifts].filter((s) => s.category !== 'off' && s.category !== 'leave').sort((a, b) => a.order - b.order);
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  const selected = peopleById.get(selectedId) ?? null;
  const matches = (p: Person) => !search.trim() || p.name.includes(search.trim());
  const inRound = new Set(roundIds);
  const outOfRound = data.people.filter((p) => !inRound.has(p.id));

  const ruleText = (p: Person) => {
    const parts: string[] = [];
    if (p.onlyShifts.length) parts.push('只上 ' + p.onlyShifts.map((id) => shiftsById.get(id)?.name).join('、'));
    if (p.bannedShifts.length) parts.push('不上 ' + p.bannedShifts.map((id) => shiftsById.get(id)?.name).join('、'));
    return parts.join(' · ') || '—';
  };

  // 在本轮排班表里上移 / 下移（只在同级别的人之间挪）
  const move = (person: Person, dir: -1 | 1) => {
    const same = roundIds.filter((id) => peopleById.get(id)?.levelId === person.levelId);
    const k = same.indexOf(person.id);
    const other = same[k + dir];
    if (!other) return;
    const ids = [...roundIds];
    const a = ids.indexOf(person.id);
    const b = ids.indexOf(other);
    [ids[a], ids[b]] = [ids[b], ids[a]];
    setRoundPersonOrder(ids);
  };

  const create = () => {
    const name = newName.trim();
    if (!name || data.people.some((p) => p.name === name)) return;
    const person: Person = {
      id: newId('p'),
      name,
      levelId: newLevel,
      order: data.people.length + 1,
      status: 'active',
      onlyShifts: [],
      bannedShifts: [],
      note: '',
    };
    addPerson(person, true);
    setSelectedId(person.id);
    setNewName('');
  };

  const shiftPicker = (label: string, value: ShiftId[], onChange: (v: ShiftId[]) => void) => (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
      <span className="muted">{label}</span>
      <div className="menu-row">
        {value.map((id) => (
          <button key={id} className="chip on" onClick={() => onChange(value.filter((x) => x !== id))} title="点击去掉">
            {shiftsById.get(id)?.name} ✕
          </button>
        ))}
        <select
          className="input"
          style={{ height: 28, width: 110 }}
          value=""
          onChange={(e) => e.target.value && onChange([...value, e.target.value])}
          aria-label={label}
        >
          <option value="">＋ 添加班次</option>
          {shifts
            .filter((s) => !value.includes(s.id))
            .map((s) => (
              <option key={s.id} value={s.id}>
                {s.name}
              </option>
            ))}
        </select>
      </div>
    </div>
  );

  const row = (p: Person, movable: boolean) => (
    <div
      key={p.id}
      className="table-row"
      style={{
        gridTemplateColumns: '1fr 80px 1.4fr 76px',
        minHeight: 38,
        padding: '0 10px',
        cursor: 'pointer',
        background: p.id === selectedId ? 'var(--primary-soft)' : '#fff',
        boxShadow: p.id === selectedId ? 'inset 3px 0 0 var(--primary)' : undefined,
      }}
      onClick={() => setSelectedId(p.id)}
    >
      <button className="menu-link" style={{ padding: 0, fontWeight: 500, color: data.levels.find((l) => l.id === p.levelId)?.nameColor }}>
        {p.name}
      </button>
      <span>
        <span className={'badge ' + STATUS_BADGE[p.status]}>{STATUS_NAMES[p.status]}</span>
      </span>
      <span className="muted">{ruleText(p)}</span>
      <span className="menu-row" style={{ gap: 2, flexWrap: 'nowrap', padding: 0 }}>
        {movable && (
          <>
            <button className="btn icon" aria-label={`${p.name} 上移`} title="在排班表里上移一行" onClick={(e) => (e.stopPropagation(), move(p, -1))}>
              ↑
            </button>
            <button className="btn icon" aria-label={`${p.name} 下移`} title="在排班表里下移一行" onClick={(e) => (e.stopPropagation(), move(p, 1))}>
              ↓
            </button>
          </>
        )}
      </span>
    </div>
  );

  return (
    <>
      <header className="page-header compact">
        <h1>人员</h1>
        <span className="muted">
          本轮 {roundIds.length} 人 · 改动立即生效并自动保存
        </span>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body" style={{ flexDirection: 'row', alignItems: 'flex-start', gap: 16 }}>
        <section style={{ flex: '3 1 460px', minWidth: 0, display: 'flex', flexDirection: 'column', gap: 10 }}>
          <form
            className="menu-row"
            onSubmit={(e) => {
              e.preventDefault();
              create();
            }}
          >
            <input className="input" type="search" placeholder="搜索姓名" aria-label="搜索姓名" value={search} onChange={(e) => setSearch(e.target.value)} style={{ width: 140 }} />
            <div className="spacer" />
            <input className="input" placeholder="新人员姓名" aria-label="新人员姓名" value={newName} onChange={(e) => setNewName(e.target.value)} style={{ width: 120 }} />
            <select className="input" value={newLevel} onChange={(e) => setNewLevel(e.target.value)} aria-label="新人员级别">
              {levels.map((l) => (
                <option key={l.id} value={l.id}>
                  {levelLabel(l)}
                </option>
              ))}
            </select>
            <button className="btn primary" type="submit" disabled={!newName.trim() || data.people.some((p) => p.name === newName.trim())}>
              ＋ 新增到本轮
            </button>
          </form>
          <div className="card" style={{ overflow: 'hidden' }}>
            {levels.map((level) => {
              const people = roundIds.map((id) => peopleById.get(id)!).filter((p) => p && p.levelId === level.id && matches(p));
              if (people.length === 0) return null;
              return (
                <div key={level.id}>
                  <div className="group-row" style={{ fontSize: 13, padding: '6px 12px' }}>
                    {levelLabel(level)}<span className="muted" style={{ fontWeight: 400 }}> {people.length} 人</span>
                  </div>
                  {people.map((p) => row(p, !search.trim()))}
                </div>
              );
            })}
            {outOfRound.filter(matches).length > 0 && (
              <div>
                <div className="group-row" style={{ fontSize: 13, padding: '6px 12px' }}>
                  不在本轮排班表里的人员 <span className="muted" style={{ fontWeight: 400 }}>（以前各轮有、本轮没有）</span>
                </div>
                {outOfRound.filter(matches).map((p) => row(p, false))}
              </div>
            )}
          </div>
        </section>

        {selected && (
          <section className="card" style={{ flex: '2 1 320px', padding: 18, display: 'flex', flexDirection: 'column', gap: 14, position: 'sticky', top: 0 }}>
            <div className="menu-row">
              <h2 style={{ margin: 0, fontSize: 16 }}>{selected.name}</h2>
              <span className={'badge ' + STATUS_BADGE[selected.status]}>{STATUS_NAMES[selected.status]}</span>
            </div>
            <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">姓名</span>
              <CommitInput key={selected.id + '-name'} value={selected.name} onCommit={(v) => v.trim() && updatePerson(selected.id, { name: v.trim() })} />
            </label>
            <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">级别</span>
              <select className="input" value={selected.levelId} onChange={(e) => updatePerson(selected.id, { levelId: e.target.value })}>
                {levels.map((l) => (
                  <option key={l.id} value={l.id}>
                    {levelLabel(l)}
                  </option>
                ))}
              </select>
            </label>
            <fieldset style={{ border: 0, padding: 0, margin: 0, display: 'flex', flexDirection: 'column', gap: 6 }}>
              <legend className="muted" style={{ marginBottom: 6 }}>
                状态
              </legend>
              {(Object.keys(STATUS_NAMES) as PersonStatus[]).map((st) => (
                <label key={st} className="menu-row" style={{ fontSize: 13 }}>
                  <input type="radio" name="status" checked={selected.status === st} onChange={() => updatePerson(selected.id, { status: st })} />
                  {STATUS_LONG[st]}
                </label>
              ))}
            </fieldset>
            {shiftPicker('只允许上这些班（不选 = 按级别权限）', selected.onlyShifts, (v) => updatePerson(selected.id, { onlyShifts: v }))}
            {shiftPicker('禁止上这些班', selected.bannedShifts, (v) => updatePerson(selected.id, { bannedShifts: v }))}
            <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">备注</span>
              <CommitInput key={selected.id + '-note'} value={selected.note ?? ''} placeholder="输完点别处或按回车保存" onCommit={(v) => updatePerson(selected.id, { note: v })} />
            </label>
            <div className="menu-note">改级别、状态、特例只影响之后的排班和调班，已经排好的班不会被改动。</div>
          </section>
        )}
      </div>
    </>
  );
}

// 输入框先在本地改，离开输入框或按回车才保存。
// 中文输入法拼字时会连续触发很多次输入事件，每次都保存会打断输入法，导致打不进字
function CommitInput({ value, onCommit, placeholder }: { value: string; onCommit(v: string): void; placeholder?: string }) {
  const [text, setText] = useState(value);
  const commit = () => {
    if (text !== value) onCommit(text);
  };
  return (
    <input
      className="input"
      value={text}
      placeholder={placeholder}
      onChange={(e) => setText(e.target.value)}
      onBlur={commit}
      onKeyDown={(e: KeyboardEvent<HTMLInputElement>) => {
        if (e.key === 'Enter' && !e.nativeEvent.isComposing) commit();
      }}
    />
  );
}
