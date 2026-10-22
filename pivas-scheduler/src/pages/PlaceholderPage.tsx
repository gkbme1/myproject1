import { SaveIndicator } from '../components/SaveIndicator';

const PLANNED: Record<string, { title: string; items: string[] }> = {
  rules: {
    title: '规则',
    items: ['开关每条排班规则，设置是“拦截”还是“只提示”', '修改规则参数：连班天数、每轮班次上限、每日人数等', '跨轮次回看上一轮末尾几天'],
  },
  stats: {
    title: '统计',
    items: ['每人本轮各班次次数、A/T 次数、夜班次数', '调休 / 补休结余', '按人员导出统计表'],
  },
  data: {
    title: '数据',
    items: ['从现有 Excel 模板导入排班', '导出排班结果到 Excel', '查看自动备份并一键恢复'],
  },
  settings: {
    title: '设置',
    items: ['操作人姓名', '全屏 / 窗口模式', '数据文件位置'],
  },
};

export function PlaceholderPage({ page }: { page: string }) {
  const info = PLANNED[page] ?? { title: page, items: [] };
  return (
    <>
      <header className="page-header">
        <h1>{info.title}</h1>
        <div className="spacer" />
        <SaveIndicator />
      </header>
      <div className="page-body">
        <div className="card" style={{ padding: 32, maxWidth: 640 }}>
          <div style={{ fontSize: 15, fontWeight: 700, marginBottom: 10 }}>这个模块将在后续版本提供，计划包括：</div>
          <ul style={{ margin: 0, paddingLeft: 20, lineHeight: 2 }}>
            {info.items.map((item) => (
              <li key={item}>{item}</li>
            ))}
          </ul>
        </div>
      </div>
    </>
  );
}
