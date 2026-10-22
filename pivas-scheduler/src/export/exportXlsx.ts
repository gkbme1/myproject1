import ExcelJS from 'exceljs';
import { formatDateTime, parseDate, roundDates, shortDate } from '../domain/dates';
import { cellLastRole, currentCells, describeCell, getRound, roundChanges } from '../domain/schedule';
import type { AppData, Person } from '../domain/types';
import { cellKey } from '../domain/types';

// 导出成和导入模板一样的格式（标题行、日期行、星期行、每人一行、姓名颜色区分级别），
// 导出的文件可以直接再导入。另附一张“修改记录”表。

// 和导入时的颜色对应一致：黑色技术员、红色高级技术员、紫色第三方、药师黑色
const NAME_COLORS: Record<string, string> = { L1: 'FF000000', L2: 'FFFF0000', L3: 'FFA0327A', L4: 'FF000000' };

// 改过的格子底色（比界面上浅一些，打印出来字也看得清）
const FILLS = { manual: 'FFFFD8A8', requester: 'FFBFD7FF', cover: 'FFFFC9C9' };

const WEEKDAY = ['日', '一', '二', '三', '四', '五', '六'];

const thin: Partial<ExcelJS.Borders> = {
  top: { style: 'thin', color: { argb: 'FFB0B7C3' } },
  left: { style: 'thin', color: { argb: 'FFB0B7C3' } },
  bottom: { style: 'thin', color: { argb: 'FFB0B7C3' } },
  right: { style: 'thin', color: { argb: 'FFB0B7C3' } },
};

export interface ExportOptions {
  markChanges: boolean; // 给本轮改过的格子上底色
}

export function exportFileName(data: AppData): string {
  const round = getRound(data);
  const start = parseDate(round.startDate);
  return `排班表_第${round.index}轮_${start.getFullYear()}-${start.getMonth() + 1}-${start.getDate()}.xlsx`;
}

export async function buildScheduleWorkbook(data: AppData, options: ExportOptions): Promise<ArrayBuffer> {
  const round = getRound(data);
  const dates = roundDates(round.startDate, round.days);
  const cells = currentCells(data, round.id);
  const roles = cellLastRole(data, round.id);
  const shiftsById = new Map(data.shifts.map((s) => [s.id, s]));
  const peopleById = new Map(data.people.map((p) => [p.id, p]));
  const people: Person[] = (round.personIds ?? [...data.people].sort((a, b) => a.order - b.order).map((p) => p.id))
    .map((id) => peopleById.get(id))
    .filter((p): p is Person => !!p);

  const wb = new ExcelJS.Workbook();
  const ws = wb.addWorksheet('排班表', { views: [{ state: 'frozen', xSplit: 1, ySplit: 3 }] });
  const lastCol = dates.length + 1;
  const first = parseDate(dates[0]);

  ws.addRow([`PIVAS技术员排班表(编号:${first.getMonth() + 1}.${first.getDate()})`]);
  ws.mergeCells(1, 1, 1, lastCol);
  const title = ws.getCell(1, 1);
  title.font = { bold: true, size: 14 };
  title.alignment = { horizontal: 'center' };

  ws.addRow([' 人员\\日期', ...dates.map((d) => shortDate(d).replace('/', '.'))]);
  ws.addRow(['', ...dates.map((d) => WEEKDAY[parseDate(d).getDay()])]);
  ws.mergeCells(2, 1, 3, 1);

  for (const person of people) {
    const row = ws.addRow([person.name, ...dates.map((d) => describeCell(cells[cellKey(person.id, d)] ?? null, shiftsById))]);
    row.getCell(1).font = { color: { argb: NAME_COLORS[person.levelId] ?? 'FF000000' } };
    if (options.markChanges) {
      dates.forEach((d, i) => {
        const role = roles[cellKey(person.id, d)];
        if (!role) return;
        row.getCell(i + 2).fill = { type: 'pattern', pattern: 'solid', fgColor: { argb: role === 'manual' ? FILLS.manual : FILLS[role] } };
      });
    }
  }

  ws.getColumn(1).width = 10;
  for (let c = 2; c <= lastCol; c++) ws.getColumn(c).width = 7;
  ws.eachRow((row, r) => {
    if (r === 1) return;
    row.eachCell({ includeEmpty: true }, (cell, c) => {
      if (c > lastCol) return;
      cell.border = thin;
      cell.alignment = { horizontal: 'center', vertical: 'middle' };
    });
  });

  // 第二张表：本轮修改记录
  const log = wb.addWorksheet('修改记录');
  log.addRow(['时间', '类型', '内容', '具体变动', '备注', '操作人', '状态']);
  log.getRow(1).font = { bold: true };
  const kindName: Record<string, string> = { leave: '调休联动', manual: '手工改班', return: '还班', delete: '删除班次', custom: '自定义内容', import: '导入' };
  for (const cs of roundChanges(data, round.id)) {
    const detail = cs.changes
      .map((ch) => `${peopleById.get(ch.personId)?.name} ${shortDate(ch.date)}：${describeCell(ch.from, shiftsById) || '空白'}→${describeCell(ch.to, shiftsById) || '空白'}`)
      .join('；');
    log.addRow([formatDateTime(cs.at), kindName[cs.kind] ?? cs.kind, cs.title, detail, cs.note, cs.operator, cs.undone ? '已撤销' : '']);
  }
  [16, 10, 36, 70, 20, 10, 8].forEach((w, i) => (log.getColumn(i + 1).width = w));

  return (await wb.xlsx.writeBuffer()) as ArrayBuffer;
}

// Electron 里弹出“另存为”；浏览器里直接下载
export async function saveWorkbook(buffer: ArrayBuffer, fileName: string): Promise<string | null> {
  if (window.pivasApi?.saveFile) {
    return window.pivasApi.saveFile(fileName, new Uint8Array(buffer));
  }
  const blob = new Blob([buffer], { type: 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = fileName;
  a.click();
  URL.revokeObjectURL(url);
  return fileName;
}
