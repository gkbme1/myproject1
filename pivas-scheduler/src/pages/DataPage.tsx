import { useEffect, useState } from 'react';
import { BackupSection } from '../components/BackupSection';
import { ImportPanel } from '../components/ImportPanel';
import { SaveIndicator } from '../components/SaveIndicator';
import { formatDateTime, roundDates, shortDate } from '../domain/dates';
import { getRound, roundChanges } from '../domain/schedule';
import type { AppData } from '../domain/types';

export function DataPage({ data }: { data: AppData }) {
  const [dataDir, setDataDir] = useState('');
  useEffect(() => {
    window.pivasApi?.getDataDir().then(setDataDir);
  }, []);

  const round = getRound(data);
  const dates = roundDates(round.startDate, round.days);
  const applied = roundChanges(data, round.id).filter((c) => !c.undone).length;

  return (
    <>
      <header className="page-header">
        <h1>数据</h1>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <section className="card" style={{ padding: 18, display: 'flex', flexDirection: 'column', gap: 4 }}>
          <h2 style={{ margin: '0 0 8px', fontSize: 16 }}>当前排班</h2>
          <div className="kv" style={{ fontSize: 13 }}>
            <span>轮次</span>
            <span>
              第 {round.index} 轮 · {shortDate(dates[0])} ~ {shortDate(dates[dates.length - 1])}
            </span>
          </div>
          <div className="kv" style={{ fontSize: 13 }}>
            <span>来源</span>
            <span>{round.source ? `${round.source.fileName}（${round.source.title}），${formatDateTime(round.source.importedAt)} 导入` : '示例数据'}</span>
          </div>
          <div className="kv" style={{ fontSize: 13 }}>
            <span>导入后的修改</span>
            <span>{applied} 处（都已自动保存，下次打开接着用）</span>
          </div>
          <div className="kv" style={{ fontSize: 13, borderBottom: 0 }}>
            <span>数据位置</span>
            <span>{dataDir ? `${dataDir}（每天自动备份到 backups 文件夹）` : '浏览器本地存储（开发模式）'}</span>
          </div>
        </section>

        <section className="card" style={{ padding: 18, display: 'flex', flexDirection: 'column', gap: 12 }}>
          <h2 style={{ margin: 0, fontSize: 16 }}>从 Excel 导入新的排班表</h2>
          <div className="muted" style={{ fontSize: 13 }}>
            平时不需要再导入。只有拿到新一轮的 Excel 排班表时才用：导入后从新表开始排班，之前的排班和修改记录都会保留。
          </div>
          <ImportPanel existing={data} />
        </section>

        <BackupSection data={data} />
      </div>
    </>
  );
}
