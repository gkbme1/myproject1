import { formatTime } from '../domain/dates';
import { useAppStore } from '../store/appStore';

export function SaveIndicator() {
  const status = useAppStore((s) => s.saveStatus);
  const savedAt = useAppStore((s) => s.savedAt);
  const saveError = useAppStore((s) => s.saveError);

  if (status === 'error') {
    return (
      <div className="save-status error" title={saveError}>
        <span className="dot" />
        保存失败，请检查磁盘空间或权限
      </div>
    );
  }
  if (status === 'saving') {
    return (
      <div className="save-status saving">
        <span className="dot" />
        正在保存…
      </div>
    );
  }
  return (
    <div className="save-status">
      <span className="dot" />
      {savedAt ? `已自动保存 · ${formatTime(savedAt)}` : '已自动保存'}
    </div>
  );
}
