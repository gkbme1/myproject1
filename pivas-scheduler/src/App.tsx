import { useEffect } from 'react';
import { NavRail } from './components/NavRail';
import { ResumeDialog } from './components/ResumeDialog';
import { SchedulePage } from './components/schedule/SchedulePage';
import { DataPage } from './pages/DataPage';
import { FirstRunPage } from './pages/FirstRunPage';
import { PeoplePage } from './pages/PeoplePage';
import { PlaceholderPage } from './pages/PlaceholderPage';
import { ShiftsPage } from './pages/ShiftsPage';
import { StatsPage } from './pages/StatsPage';
import { loadAppData } from './persistence/storage';
import { useAppStore } from './store/appStore';

export function App() {
  const data = useAppStore((s) => s.data);
  const loadError = useAppStore((s) => s.loadError);
  const page = useAppStore((s) => s.page);
  const showResume = useAppStore((s) => s.showResume);
  const init = useAppStore((s) => s.init);
  const setLoadError = useAppStore((s) => s.setLoadError);
  const needsFirstImport = useAppStore((s) => s.needsFirstImport);

  useEffect(() => {
    loadAppData()
      .then(({ data }) => init(data))
      .catch((e) => setLoadError(e instanceof Error ? e.message : String(e)));
  }, [init, setLoadError]);

  if (loadError) {
    return (
      <div className="overlay">
        <div className="dialog">
          <h1 style={{ margin: 0, fontSize: 20, color: 'var(--danger)' }}>数据读取失败</h1>
          <div>{loadError}</div>
          <div className="muted">数据文件没有被改动。可以在“数据”目录的 backups 文件夹里找到每天的自动备份。</div>
        </div>
      </div>
    );
  }

  if (needsFirstImport) {
    return <FirstRunPage />;
  }

  if (!data) {
    return <div style={{ padding: 40 }}>正在读取排班数据…</div>;
  }

  return (
    <div className="app">
      <NavRail />
      <main className="app-main">
        {page === 'schedule' && <SchedulePage data={data} />}
        {page === 'people' && <PeoplePage data={data} />}
        {page === 'shifts' && <ShiftsPage data={data} />}
        {page === 'data' && <DataPage data={data} />}
        {page === 'stats' && <StatsPage data={data} />}
        {(page === 'rules' || page === 'settings') && <PlaceholderPage page={page} />}
      </main>
      {showResume && <ResumeDialog data={data} />}
    </div>
  );
}
