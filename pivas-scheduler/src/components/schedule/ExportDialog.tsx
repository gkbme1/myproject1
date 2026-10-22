import { useState } from 'react';
import type { AppData } from '../../domain/types';
import { buildScheduleWorkbook, exportFileName, saveWorkbook } from '../../export/exportXlsx';

export function ExportDialog({ data, onClose }: { data: AppData; onClose(): void }) {
  const [markChanges, setMarkChanges] = useState(true);
  const [busy, setBusy] = useState(false);
  const [result, setResult] = useState('');

  const run = async () => {
    setBusy(true);
    setResult('');
    try {
      const buffer = await buildScheduleWorkbook(data, { markChanges });
      const saved = await saveWorkbook(buffer, exportFileName(data));
      setResult(saved ? `已保存到：${saved}` : '已取消');
    } catch (e) {
      setResult('导出失败：' + (e instanceof Error ? e.message : String(e)));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="overlay" style={{ background: 'rgba(27, 36, 48, 0.45)' }}>
      <div className="dialog" role="dialog" aria-modal="true" aria-labelledby="export-title" style={{ maxWidth: 480, gap: 14 }}>
        <h1 id="export-title" style={{ margin: 0, fontSize: 20 }}>
          导出 Excel
        </h1>
        <div className="muted" style={{ fontSize: 13, lineHeight: 1.8 }}>
          导出当前这一轮的排班，格式和导入用的模板一样（姓名颜色区分级别），导出的文件也可以再导入。另附一张“修改记录”表。
        </div>
        <label className="menu-row" style={{ fontSize: 13 }}>
          <input type="checkbox" checked={markChanges} onChange={(e) => setMarkChanges(e.target.checked)} />
          给本轮改过的格子上底色（橙=手工改过，蓝=调休，红=被联动改班）
        </label>
        {result && <div className="menu-note" style={{ fontSize: 12, wordBreak: 'break-all' }}>{result}</div>}
        <div className="menu-row" style={{ justifyContent: 'flex-end' }}>
          <button className="btn" onClick={onClose}>
            关闭
          </button>
          <button className="btn primary" onClick={run} disabled={busy}>
            {busy ? '正在导出…' : '选择保存位置并导出'}
          </button>
        </div>
      </div>
    </div>
  );
}
