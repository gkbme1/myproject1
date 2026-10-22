import { describe, expect, it } from 'vitest';
import { currentCells, describeCell } from '../domain/schedule';
import { createSeedData } from '../domain/seed';
import { cellKey } from '../domain/types';
import { applyImport, suggestLevelChoice } from '../import/buildImport';
import { parseScheduleSheet } from '../import/parseSchedule';
import { readFirstSheet } from '../import/readXlsx';
import { buildScheduleWorkbook } from './exportXlsx';

describe('导出 Excel', () => {
  it('导出的文件能原样再导入：人员、级别、每个格子都一致', async () => {
    const data = createSeedData();
    const buffer = await buildScheduleWorkbook(data, { markChanges: true });
    const draft = parseScheduleSheet(await readFirstSheet(buffer), new Date(2026, 9, 22));
    expect(draft.startDate).toBe(data.rounds[0].startDate);
    expect(draft.people.map((p) => p.name)).toEqual(data.people.map((p) => p.name));

    const again = applyImport(null, draft, { fileName: 'x.xlsx', startDate: draft.startDate, choice: suggestLevelChoice(draft) });
    const levelOf = (d: typeof data, name: string) => d.people.find((p) => p.name === name)!.levelId;
    for (const p of data.people) expect(levelOf(again, p.name)).toBe(p.levelId);

    const a = currentCells(data);
    const b = currentCells(again);
    const sa = new Map(data.shifts.map((s) => [s.id, s]));
    const sb = new Map(again.shifts.map((s) => [s.id, s]));
    for (const p of data.people) {
      const q = again.people.find((x) => x.name === p.name)!;
      for (const key of Object.keys(a).filter((k) => k.startsWith(p.id + '|'))) {
        const date = key.split('|')[1];
        expect(describeCell(b[cellKey(q.id, date)] ?? null, sb)).toBe(describeCell(a[key], sa));
      }
    }
  });
});
