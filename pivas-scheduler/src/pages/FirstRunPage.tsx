import { ImportPanel } from '../components/ImportPanel';
import { useAppStore } from '../store/appStore';

// 第一次打开软件、还没有任何数据时：导入现有的 Excel 排班表
export function FirstRunPage() {
  const loadSampleData = useAppStore((s) => s.loadSampleData);
  return (
    <div style={{ height: '100%', overflow: 'auto', background: 'var(--bg)' }}>
      <div style={{ maxWidth: 920, margin: '0 auto', padding: '48px 24px', display: 'flex', flexDirection: 'column', gap: 20 }}>
        <div className="menu-row" style={{ gap: 14 }}>
          <div className="rail-logo" style={{ margin: 0, background: 'var(--rail)' }}>静配</div>
          <div>
            <h1 style={{ margin: 0, fontSize: 24 }}>静配中心排班系统</h1>
            <div className="muted" style={{ fontSize: 14 }}>第一次使用：先导入现在的 Excel 排班表</div>
          </div>
        </div>
        <div className="card" style={{ padding: 24 }}>
          <ImportPanel existing={null} />
        </div>
        <div className="muted" style={{ fontSize: 13 }}>
          姓名的字体颜色用来区分级别：黑色 = 技术员，红色 = 高级技术员，紫色 = 第三方；名字是黑色但只上 A/B/C/D/T 的人算药师。导入前可以逐个核对。
          <button className="btn" style={{ marginLeft: 12 }} onClick={loadSampleData}>
            先用示例数据看看
          </button>
        </div>
      </div>
    </div>
  );
}
