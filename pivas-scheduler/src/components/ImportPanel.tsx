import { useMemo, useRef, useState } from 'react';
import { addDays, shortDate, weekdayName } from '../domain/dates';
import { createDefaultLevels, createDefaultShifts } from '../domain/seed';
import type { AppData, LevelId } from '../domain/types';
import {
  applyImport,
  colorName,
  resolveLevel,
  shiftNameSet,
  suggestLevelChoice,
  summarizeDraft,
} from '../import/buildImport';
import type { LevelChoice } from '../import/buildImport';
import type { ImportDraft } from '../import/parseSchedule';
import { parseScheduleSheet } from '../import/parseSchedule';
import { readFirstSheet } from '../import/readXlsx';
import { useAppStore } from '../store/appStore';

interface Loaded {
  fileName: string;
  draft: ImportDraft;
}

// 选 Excel → 预览核对（颜色对应的级别、认不出的格子）→ 确认导入
export function ImportPanel({ existing }: { existing: AppData | null }) {
  const replaceData = useAppStore((s) => s.replaceData);
  const fileInput = useRef<HTMLInputElement>(null);
  const [loaded, setLoaded] = useState<Loaded | null>(null);
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);
  const [startDate, setStartDate] = useState('');
  const [choice, setChoice] = useState<LevelChoice>({ colorLevels: {}, overrides: {} });
  const [showPeople, setShowPeople] = useState(false);

  const levels = existing?.levels ?? createDefaultLevels();
  const shifts = useMemo(() => existing?.shifts ?? createDefaultShifts(), [existing]);
  const names = useMemo(() => shiftNameSet(shifts), [shifts]);

  const onFile = async (file: File) => {
    setError('');
    setBusy(true);
    try {
      const sheet = await readFirstSheet(await file.arrayBuffer());
      const draft = parseScheduleSheet(sheet);
      setLoaded({ fileName: file.name, draft });
      setStartDate(draft.startDate);
      setChoice(suggestLevelChoice(draft));
    } catch (e) {
      setLoaded(null);
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setBusy(false);
    }
  };

  const draft = loaded?.draft;
  const summary = useMemo(() => (draft ? summarizeDraft(draft, shifts) : null), [draft, shifts]);
  const resolved = useMemo(
    () => (draft ? draft.people.map((p) => ({ person: p, levelId: resolveLevel(p, choice, names) })) : []),
    [draft, choice, names],
  );
  const colors = useMemo(() => {
    if (!draft) return [];
    const map = new Map<string, string[]>();
    draft.people.forEach((p) => map.set(p.color, [...(map.get(p.color) ?? []), p.name]));
    return Array.from(map.entries()).map(([color, people]) => ({ color, people }));
  }, [draft]);
  const autoPharmacists = resolved.filter((r) => r.levelId === 'L4' && !choice.overrides[r.person.name] && choice.colorLevels[r.person.color] !== 'L4');

  const confirm = () => {
    if (!loaded) return;
    if (existing && !window.confirm('导入后会以这张表开始新的一轮排班。当前这一轮和它的修改记录会保留在历史数据里。确定导入吗？')) {
      return;
    }
    replaceData(applyImport(existing, loaded.draft, { fileName: loaded.fileName, startDate, choice }));
  };

  const levelSelect = (value: LevelId, onChange: (v: LevelId) => void, label: string) => (
    <select className="input" style={{ height: 30 }} value={value} onChange={(e) => onChange(e.target.value)} aria-label={label}>
      {levels.map((l) => (
        <option key={l.id} value={l.id}>
          {l.name}（{l.shortName}）
        </option>
      ))}
    </select>
  );

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
      <div className="menu-row" style={{ gap: 10 }}>
        <input
          ref={fileInput}
          type="file"
          accept=".xlsx"
          style={{ display: 'none' }}
          onChange={(e) => {
            const f = e.target.files?.[0];
            if (f) onFile(f);
            e.target.value = '';
          }}
        />
        <button className="btn primary" style={{ height: 40, padding: '0 18px', fontSize: 14 }} onClick={() => fileInput.current?.click()} disabled={busy}>
          {busy ? '正在读取…' : loaded ? '换一个文件' : '选择 Excel 排班表（.xlsx）'}
        </button>
        {loaded && <span className="muted" style={{ fontSize: 13 }}>{loaded.fileName}</span>}
      </div>
      {error && <div className="menu-note" style={{ color: 'var(--danger)', background: '#fdecec', fontSize: 13 }}>读取失败：{error}</div>}

      {draft && summary && (
        <>
          <div className="card" style={{ padding: 16, display: 'flex', flexDirection: 'column', gap: 10 }}>
            <div style={{ fontSize: 15, fontWeight: 700 }}>{draft.title}</div>
            <div className="menu-row" style={{ gap: 16, fontSize: 13 }}>
              <label className="menu-row" style={{ gap: 6 }}>
                <span className="muted">第一天</span>
                <input className="input" type="date" value={startDate} onChange={(e) => e.target.value && setStartDate(e.target.value)} />
              </label>
              <span>
                {shortDate(startDate)} {weekdayName(startDate)} ~ {shortDate(addDays(startDate, draft.days - 1))} {weekdayName(addDays(startDate, draft.days - 1))}，共 {draft.days} 天
              </span>
              <span>{draft.people.length} 人</span>
            </div>
            {draft.warnings.map((w) => (
              <div key={w} className="menu-note" style={{ color: 'var(--warn)', background: 'var(--warn-bg)' }}>
                {w}
              </div>
            ))}
          </div>

          <div className="card" style={{ padding: 16, display: 'flex', flexDirection: 'column', gap: 10 }}>
            <div style={{ fontSize: 14, fontWeight: 700 }}>人名颜色 → 级别</div>
            {colors.map(({ color, people }) => (
              <div key={color} className="menu-row" style={{ gap: 10 }}>
                <span className="swatch" style={{ background: color.startsWith('#') ? color : '#d0d5dd', width: 18, height: 18 }} />
                <span style={{ width: 64, fontWeight: 500, color: color.startsWith('#') ? color : undefined }}>{colorName(color)}</span>
                <span className="muted" style={{ width: 52 }}>{people.length} 人</span>
                {levelSelect(choice.colorLevels[color] ?? 'L1', (v) => setChoice({ ...choice, colorLevels: { ...choice.colorLevels, [color]: v } }), `${colorName(color)}名字的级别`)}
                <span className="muted" style={{ flex: 1, minWidth: 0, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                  {people.slice(0, 6).join('、')}
                  {people.length > 6 ? ' …' : ''}
                </span>
              </div>
            ))}
            {autoPharmacists.length > 0 && (
              <div className="menu-note" style={{ fontSize: 12 }}>
                以下 {autoPharmacists.length} 人名字是黑色、但只上 A/B/C/D/T，已自动归为药师：{autoPharmacists.map((r) => r.person.name).join('、')}
              </div>
            )}
            <div className="menu-row" style={{ gap: 14 }}>
              {levels.map((l) => (
                <span key={l.id} style={{ fontSize: 13 }}>
                  {l.name} <b>{resolved.filter((r) => r.levelId === l.id).length}</b> 人
                </span>
              ))}
              <div className="spacer" />
              <button className="btn" onClick={() => setShowPeople(!showPeople)}>
                {showPeople ? '收起' : '逐个核对级别'}
              </button>
            </div>
            {showPeople && (
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(230px, 1fr))', gap: 8 }}>
                {resolved.map(({ person, levelId }) => (
                  <div key={person.name} className="menu-row" style={{ gap: 8 }}>
                    <span style={{ width: 64, fontWeight: 500, color: person.color.startsWith('#') ? person.color : undefined }}>{person.name}</span>
                    {levelSelect(levelId, (v) => setChoice({ ...choice, overrides: { ...choice.overrides, [person.name]: v } }), `${person.name}的级别`)}
                  </div>
                ))}
              </div>
            )}
          </div>

          {(summary.unknownTexts.length > 0 || summary.newLeaveShifts.length > 0) && (
            <div className="card" style={{ padding: 16, display: 'flex', flexDirection: 'column', gap: 8 }}>
              {summary.newLeaveShifts.length > 0 && (
                <div style={{ fontSize: 13 }}>会新增的节日补休班次：{summary.newLeaveShifts.join('、')}</div>
              )}
              {summary.unknownTexts.length > 0 && (
                <>
                  <div style={{ fontSize: 13 }}>这些格子认不出是哪个班次，会原样保留成自定义内容：</div>
                  <div className="menu-row">
                    {summary.unknownTexts.map((u) => (
                      <span key={u.text} className="badge gray">
                        {u.text} × {u.count}
                      </span>
                    ))}
                  </div>
                </>
              )}
            </div>
          )}

          <div className="menu-row">
            <button className="btn primary big-button" style={{ padding: '0 28px' }} onClick={confirm}>
              导入并开始排班
            </button>
            <span className="muted" style={{ fontSize: 13 }}>
              导入一次就行：之后每次打开软件都会直接接着上次的排班和修改继续。
            </span>
          </div>
        </>
      )}
    </div>
  );
}
