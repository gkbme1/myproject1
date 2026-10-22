import ExcelJS from 'exceljs';

// 从 xlsx 里读出第一个工作表的文字和字体颜色。
// 颜色统一成 "#RRGGBB"，读不出具体颜色的（主题色、索引色）保留成 "theme:N" / "indexed:N"。

export interface RawCell {
  text: string;
  color: string;
}

export interface RawSheet {
  name: string;
  rows: RawCell[][];
}

const BLACK = '#000000';

// Excel 老的索引色里常见的几个
const INDEXED_COLORS: Record<number, string> = {
  8: BLACK,
  9: '#FFFFFF',
  10: '#FF0000',
  12: '#0000FF',
  64: BLACK, // 系统前景色
};

type FontColor = Partial<{ argb: string; theme: number; indexed: number }>;

export function normalizeColor(color: FontColor | undefined): string {
  if (!color) return BLACK; // 没设颜色 = 自动 = 黑色
  if (color.argb) {
    const hex = color.argb.length === 8 ? color.argb.slice(2) : color.argb;
    return '#' + hex.toUpperCase();
  }
  if (color.theme !== undefined) {
    // 主题色 1 是"文字/背景 1"里的文字色，就是默认的黑色
    return color.theme === 1 ? BLACK : `theme:${color.theme}`;
  }
  if (color.indexed !== undefined) {
    return INDEXED_COLORS[color.indexed] ?? `indexed:${color.indexed}`;
  }
  return BLACK;
}

function cellColor(cell: ExcelJS.Cell): string {
  const value = cell.value as { richText?: { font?: { color?: FontColor } }[] } | null;
  // 富文本取第一段文字的颜色
  if (value && typeof value === 'object' && Array.isArray(value.richText) && value.richText.length > 0) {
    const runColor = value.richText.find((r) => r.font?.color)?.font?.color;
    if (runColor) return normalizeColor(runColor);
  }
  return normalizeColor(cell.font?.color as FontColor | undefined);
}

export async function readFirstSheet(buffer: ArrayBuffer): Promise<RawSheet> {
  const workbook = new ExcelJS.Workbook();
  await workbook.xlsx.load(buffer);
  const sheet = workbook.worksheets[0];
  if (!sheet) throw new Error('这个 Excel 文件里没有工作表');

  const rows: RawCell[][] = [];
  for (let r = 1; r <= sheet.rowCount; r++) {
    const row = sheet.getRow(r);
    const cells: RawCell[] = [];
    for (let c = 1; c <= sheet.columnCount; c++) {
      const cell = row.getCell(c);
      // 合并单元格只认左上角那一格，其余格当空白，免得标题、表头被重复读
      const isMergedSlave = cell.isMerged && cell.master.address !== cell.address;
      cells.push({ text: isMergedSlave ? '' : (cell.text ?? '').toString(), color: cellColor(cell) });
    }
    rows.push(cells);
  }
  return { name: sheet.name, rows };
}
