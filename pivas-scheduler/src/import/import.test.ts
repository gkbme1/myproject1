import ExcelJS from 'exceljs';
import { describe, expect, it } from 'vitest';
import { currentCells, describeCell } from '../domain/schedule';
import type { AppData } from '../domain/types';
import { cellKey } from '../domain/types';
import { applyImport, classifyText, shiftNameSet, suggestLevelChoice, summarizeDraft } from './buildImport';
import { parseScheduleSheet } from './parseSchedule';
import { readFirstSheet } from './readXlsx';
import { createDefaultShifts } from '../domain/seed';

const RED = { argb: 'FFFF0000' };
const PURPLE = { argb: 'FFA0327A' };
const BLACK_THEME = { theme: 1 };

// 按现有模板的样子造一张小表：标题、日期行、星期行，名字颜色区分级别
async function buildWorkbook(): Promise<ArrayBuffer> {
  const wb = new ExcelJS.Workbook();
  const ws = wb.addWorksheet('Sheet1');
  ws.addRow(['PIVAS技术员排班表(编号:8.17)']);
  ws.mergeCells('A1:E1');
  ws.addRow([' 人员\\日期', '8.17', '8.18', '8.19', '8.20']);
  ws.addRow(['', '一（14）', '二（15）', '三（15）', '四（14）']);
  const rows: [string, object, string[]][] = [
    ['张三', BLACK_THEME, ['3—1', '休', '调休张', '夜1 ']],
    ['李四', RED, ['Z破', '4-4', '补五孙', '外出']],
    ['王五', PURPLE, ['3*', '3', '休', '2(3)']],
    ['赵六', BLACK_THEME, ['A', 'B', '休', 'T']],
  ];
  for (const [name, color, cells] of rows) {
    const row = ws.addRow([name, ...cells]);
    row.getCell(1).font = { color };
  }
  return (await wb.xlsx.writeBuffer()) as ArrayBuffer;
}

describe('Excel 导入', () => {
  it('认出表头、日期年份、人员和名字颜色', async () => {
    const sheet = await readFirstSheet(await buildWorkbook());
    const draft = parseScheduleSheet(sheet, new Date(2026, 9, 22));
    expect(draft.title).toBe('PIVAS技术员排班表(编号:8.17)');
    expect(draft.startDate).toBe('2026-08-17'); // 2026-08-17 是周一，和星期行对得上
    expect(draft.days).toBe(4);
    expect(draft.people.map((p) => [p.name, p.color])).toEqual([
      ['张三', '#000000'],
      ['李四', '#FF0000'],
      ['王五', '#A0327A'],
      ['赵六', '#000000'],
    ]);
  });

  it('颜色对应级别，黑色但只上 A/B/C/D/T 的算药师', async () => {
    const draft = parseScheduleSheet(await readFirstSheet(await buildWorkbook()), new Date(2026, 9, 22));
    const data = applyImport(null, draft, { fileName: 't.xlsx', startDate: draft.startDate, choice: suggestLevelChoice(draft) });
    const levelOf = (name: string) => data.people.find((p) => p.name === name)!.levelId;
    expect(levelOf('张三')).toBe('L1');
    expect(levelOf('李四')).toBe('L2');
    expect(levelOf('王五')).toBe('L3');
    expect(levelOf('赵六')).toBe('L4');
  });

  it('格子内容：破折号、空格归一；调休/补休保留原文；认不出的保留成自定义', async () => {
    const draft = parseScheduleSheet(await readFirstSheet(await buildWorkbook()), new Date(2026, 9, 22));
    const data = applyImport(null, draft, { fileName: 't.xlsx', startDate: draft.startDate, choice: suggestLevelChoice(draft) });
    const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
    const cells = currentCells(data);
    const at = (name: string, day: number) => {
      const p = data.people.find((x) => x.name === name)!;
      const v = cells[cellKey(p.id, ['2026-08-17', '2026-08-18', '2026-08-19', '2026-08-20'][day])];
      return { text: describeCell(v, shiftsById), shift: v && 'shiftId' in v ? shiftsById.get(v.shiftId)!.name : null };
    };
    expect(at('张三', 0)).toEqual({ text: '3-1', shift: '3-1' });
    expect(at('张三', 2)).toEqual({ text: '调休张', shift: '调休' });
    expect(at('张三', 3)).toEqual({ text: '夜1', shift: '夜1' });
    expect(at('李四', 2)).toEqual({ text: '补五孙', shift: '补五一' });
    expect(at('李四', 3)).toEqual({ text: '外出', shift: null });
    expect(summarizeDraft(draft, createDefaultShifts()).unknownTexts).toEqual([{ text: '外出', count: 1 }]);
  });

  it('再次导入：按姓名对上已有人员，开始新的一轮，旧的修改记录保留', async () => {
    const draft = parseScheduleSheet(await readFirstSheet(await buildWorkbook()), new Date(2026, 9, 22));
    const first: AppData = applyImport(null, draft, { fileName: 'a.xlsx', startDate: draft.startDate, choice: suggestLevelChoice(draft) });
    first.changes.push({ id: 'c1', roundId: first.currentRoundId, kind: 'manual', title: 'x', note: '', at: '', operator: '', changes: [], undone: false });
    const second = applyImport(first, draft, { fileName: 'b.xlsx', startDate: '2026-08-31', choice: suggestLevelChoice(draft) });
    expect(second.people).toHaveLength(4);
    expect(second.people.map((p) => p.id)).toEqual(first.people.map((p) => p.id));
    expect(second.rounds).toHaveLength(2);
    expect(second.rounds[1].index).toBe(2);
    expect(second.currentRoundId).toBe(second.rounds[1].id);
    expect(second.changes).toHaveLength(1);
  });

  it('班次名识别', () => {
    const names = shiftNameSet(createDefaultShifts());
    expect(classifyText('3 - 4', names)).toEqual({ kind: 'shift', shiftName: '3-4', keepText: false });
    expect(classifyText('3（10）', names)).toEqual({ kind: 'shift', shiftName: '3(10)', keepText: false });
    expect(classifyText('调休余', names)).toEqual({ kind: 'shift', shiftName: '调休', keepText: true });
    expect(classifyText('补端毛', names)).toEqual({ kind: 'shift', shiftName: '补端午', keepText: true });
    expect(classifyText('', names)).toEqual({ kind: 'empty' });
  });
});
