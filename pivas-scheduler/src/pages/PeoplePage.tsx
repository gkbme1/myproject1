import { useState } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import type { AppData, Person, PersonStatus } from '../domain/types';
import { useAppStore } from '../store/appStore';

const STATUS_NAMES: Record<PersonStatus, string> = {
  active: '在岗',
  noRotate: '不轮班',
  frozen: '冻结',
};
const STATUS_BADGE: Record<PersonStatus, string> = {
  active: 'ok',
  noRotate: 'warn',
  frozen: 'gray',
};

export function PeoplePage({ data }: { data: AppData }) {
  const updatePerson = useAppStore((s) => s.updatePerson);
  const [selectedId, setSelectedId] = useState<string>(data.people[0]?.id ?? '');
  const [search, setSearch] = useState('');

  const levels = [...data.levels].sort((a, b) => a.order - b.order);
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const selected = data.people.find((p) => p.id === selectedId) ?? null;
  const matches = (p: Person) => !search.trim() || p.name.includes(search.trim());

  const ruleText = (p: Person) => {
    const parts: string[] = [];
    if (p.onlyShifts.length) parts.push('只上 ' + p.onlyShifts.map((id) => shiftsById.get(id)?.name).join('、'));
    if (p.bannedShifts.length) parts.push('不上 ' + p.bannedShifts.map((id) => shiftsById.get(id)?.name).join('、'));
    return parts.join(' · ') || '—';
  };

  return (
    <>
      <header className="page-header">
        <h1>人员</h1>
        <span className="muted">共 {data.people.length} 人 · 改动立即生效并自动保存</span>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body" style={{ flexDirection: 'row', alignItems: 'flex-start', gap: 16 }}>
        <section style={{ flex: '3 1 420px', minWidth: 0, display: 'flex', flexDirection: 'column', gap: 12 }}>
          <div className="menu-row">
            <h2 style={{ margin: 0, fontSize: 16 }}>人员列表</h2>
            <div className="spacer" />
            <input className="input" type="search" placeholder="搜索姓名" aria-label="搜索姓名" value={search} onChange={(e) => setSearch(e.target.value)} style={{ width: 160 }} />
            <button className="btn" disabled title="后续版本提供">
              ＋ 新增人员
            </button>
          </div>
          <div className="card" style={{ overflow: 'hidden' }}>
            {levels.map((level) => {
              const people = data.people.filter((p) => p.levelId === level.id && matches(p)).sort((a, b) => a.order - b.order);
              if (people.length === 0) return null;
              return (
                <div key={level.id}>
                  <div className="group-row" style={{ fontSize: 13, padding: '10px 14px' }}>
                    {level.name}（{level.shortName}）<span className="muted" style={{ fontWeight: 400 }}> {people.length} 人</span>
                  </div>
                  {people.map((p) => (
                    <button
                      key={p.id}
                      className="table-row"
                      onClick={() => setSelectedId(p.id)}
                      style={{
                        all: 'unset',
                        boxSizing: 'border-box',
                        display: 'grid',
                        gridTemplateColumns: '1fr 90px 1.4fr',
                        alignItems: 'center',
                        width: '100%',
                        minHeight: 42,
                        padding: '0 14px',
                        borderBottom: '1px solid var(--line-soft)',
                        cursor: 'pointer',
                        background: p.id === selectedId ? 'var(--primary-soft)' : '#fff',
                        boxShadow: p.id === selectedId ? 'inset 3px 0 0 var(--primary)' : undefined,
                      }}
                    >
                      <span style={{ fontWeight: 500 }}>{p.name}</span>
                      <span>
                        <span className={'badge ' + STATUS_BADGE[p.status]}>{STATUS_NAMES[p.status]}</span>
                      </span>
                      <span className="muted">{ruleText(p)}</span>
                    </button>
                  ))}
                </div>
              );
            })}
          </div>
        </section>

        {selected && (
          <section className="card" style={{ flex: '2 1 300px', padding: 18, display: 'flex', flexDirection: 'column', gap: 14 }}>
            <div className="menu-row">
              <h2 style={{ margin: 0, fontSize: 16 }}>{selected.name}</h2>
              <span className={'badge ' + STATUS_BADGE[selected.status]}>{STATUS_NAMES[selected.status]}</span>
            </div>
            <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">姓名</span>
              <input className="input" value={selected.name} onChange={(e) => updatePerson(selected.id, { name: e.target.value })} />
            </label>
            <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">级别</span>
              <select className="input" value={selected.levelId} onChange={(e) => updatePerson(selected.id, { levelId: e.target.value })}>
                {levels.map((l) => (
                  <option key={l.id} value={l.id}>
                    {l.name}（{l.shortName}）
                  </option>
                ))}
              </select>
            </label>
            <fieldset style={{ border: 0, padding: 0, margin: 0, display: 'flex', flexDirection: 'column', gap: 6 }}>
              <legend className="muted" style={{ marginBottom: 6 }}>状态</legend>
              {(Object.keys(STATUS_NAMES) as PersonStatus[]).map((st) => (
                <label key={st} className="menu-row" style={{ fontSize: 13 }}>
                  <input type="radio" name="status" checked={selected.status === st} onChange={() => updatePerson(selected.id, { status: st })} />
                  {st === 'active' ? '在岗，正常轮班' : st === 'noRotate' ? '在岗，不参与轮班和自动联动' : '冻结（暂时不排，如休产假）'}
                </label>
              ))}
            </fieldset>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              <span className="muted">人员特例</span>
              <span style={{ fontSize: 13 }}>{ruleText(selected)}</span>
              <span className="muted">（编辑“只允许 / 禁止上的班次”将在后续版本提供）</span>
            </div>
            <div className="menu-note">改级别或状态只影响之后的排班和调班，已经排好的班不会被改动。</div>
          </section>
        )}
      </div>
    </>
  );
}
