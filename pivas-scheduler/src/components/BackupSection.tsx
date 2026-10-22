import { useEffect, useRef, useState } from 'react';
import { formatDateTime } from '../domain/dates';
import type { AppData } from '../domain/types';
import type { BackupInfo } from '../persistence/storage';
import { parseDataFile } from '../persistence/storage';
import { useAppStore } from '../store/appStore';

// 自动备份列表、恢复备份、导出 / 导入整份数据文件（拷到 U 盘、换电脑用）
export function BackupSection({ data }: { data: AppData }) {
  const restoreData = useAppStore((s) => s.restoreData);
  const [backups, setBackups] = useState<BackupInfo[]>([]);
  const [message, setMessage] = useState('');
  const fileInput = useRef<HTMLInputElement>(null);
  const inElectron = !!window.pivasApi;

  const refresh = () => {
    window.pivasApi?.listBackups().then(setBackups);
  };
  useEffect(refresh, []);

  const restoreFrom = (raw: string, label: string) => {
    const parsed = parseDataFile(raw);
    restoreData(parsed);
    setMessage(`已恢复到 ${label}`);
  };

  const restoreBackup = async (b: BackupInfo) => {
    if (!window.confirm(`把数据恢复到 ${formatDateTime(b.modifiedAt)} 的备份？\n现在的数据会先另存一份备份，不会丢。`)) return;
    try {
      restoreFrom(await window.pivasApi!.restoreBackup(b.name), formatDateTime(b.modifiedAt) + ' 的备份');
      refresh();
    } catch (e) {
      setMessage('恢复失败：' + (e instanceof Error ? e.message : String(e)));
    }
  };

  const exportData = async () => {
    const d = new Date();
    const name = `排班数据_${d.getFullYear()}-${d.getMonth() + 1}-${d.getDate()}.json`;
    const content = new TextEncoder().encode(JSON.stringify(data));
    if (window.pivasApi) {
      const saved = await window.pivasApi.saveFile(name, content);
      setMessage(saved ? `已导出到：${saved}` : '已取消');
    } else {
      const url = URL.createObjectURL(new Blob([content], { type: 'application/json' }));
      const a = document.createElement('a');
      a.href = url;
      a.download = name;
      a.click();
      URL.revokeObjectURL(url);
    }
  };

  return (
    <section className="card" style={{ padding: 18, display: 'flex', flexDirection: 'column', gap: 12 }}>
      <h2 style={{ margin: 0, fontSize: 16 }}>备份与恢复</h2>
      <div className="muted" style={{ fontSize: 13 }}>每天第一次保存时会自动备份一份，保留最近 30 天。改乱了可以恢复到之前某一天。</div>
      <div className="menu-row">
        <button className="btn" onClick={exportData}>
          导出整份数据（拷到 U 盘 / 换电脑用）
        </button>
        <input
          ref={fileInput}
          type="file"
          accept=".json"
          style={{ display: 'none' }}
          onChange={async (e) => {
            const f = e.target.files?.[0];
            e.target.value = '';
            if (!f) return;
            if (!window.confirm(`用 ${f.name} 替换现在的全部数据？`)) return;
            try {
              restoreFrom(await f.text(), f.name);
            } catch (err) {
              setMessage('导入失败：' + (err instanceof Error ? err.message : String(err)));
            }
          }}
        />
        <button className="btn" onClick={() => fileInput.current?.click()}>
          从数据文件恢复
        </button>
      </div>
      {message && <div className="menu-note" style={{ fontSize: 12 }}>{message}</div>}
      {inElectron && (
        <div className="card" style={{ maxHeight: 240, overflowY: 'auto' }}>
          {backups.length === 0 && <div className="muted" style={{ padding: 12 }}>还没有自动备份</div>}
          {backups.map((b) => (
            <div key={b.name} className="table-row" style={{ gridTemplateColumns: '1fr 120px 100px', fontSize: 13 }}>
              <span>{b.name.startsWith('before-restore') ? '恢复前自动另存' : '每日自动备份'} · {formatDateTime(b.modifiedAt)}</span>
              <span className="muted">{(b.size / 1024).toFixed(0)} KB</span>
              <span>
                <button className="btn" style={{ height: 28 }} onClick={() => restoreBackup(b)}>
                  恢复到这里
                </button>
              </span>
            </div>
          ))}
        </div>
      )}
    </section>
  );
}
