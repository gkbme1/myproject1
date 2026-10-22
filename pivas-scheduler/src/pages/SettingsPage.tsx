import { useEffect, useState } from 'react';
import { SaveIndicator } from '../components/SaveIndicator';
import type { AppData } from '../domain/types';
import { useAppStore } from '../store/appStore';

export function SettingsPage({ data }: { data: AppData }) {
  const updateSettings = useAppStore((s) => s.updateSettings);
  const [operator, setOperator] = useState(data.settings?.operator ?? '管理员');
  const [dataDir, setDataDir] = useState('');
  useEffect(() => {
    window.pivasApi?.getDataDir().then(setDataDir);
  }, []);

  return (
    <>
      <header className="page-header compact">
        <h1>设置</h1>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <section className="card" style={{ padding: 18, display: 'flex', flexDirection: 'column', gap: 14, maxWidth: 720 }}>
          <label style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
            <span style={{ fontWeight: 500 }}>操作人</span>
            <span className="muted">每条修改记录都会记下操作人，几个人轮流排班时方便查是谁改的</span>
            <input
              className="input"
              style={{ width: 240 }}
              value={operator}
              onChange={(e) => setOperator(e.target.value)}
              onBlur={() => updateSettings({ operator: operator.trim() || '管理员' })}
            />
          </label>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
            <span style={{ fontWeight: 500 }}>显示</span>
            <span className="muted" style={{ lineHeight: 1.8 }}>
              Ctrl + 放大，Ctrl - 缩小，Ctrl 0 还原；F11 全屏 / 退出全屏。屏幕小的时候缩小一点，一屏能看到更多人。
            </span>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
            <span style={{ fontWeight: 500 }}>数据位置</span>
            <span className="muted">{dataDir || '浏览器本地存储（开发模式）'}</span>
          </div>
          <div className="muted">版本 0.2.0 · Electron 22（支持 Win7 SP1）</div>
        </section>
      </div>
    </>
  );
}
