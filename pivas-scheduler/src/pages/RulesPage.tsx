import { SaveIndicator } from '../components/SaveIndicator';
import { DEFAULT_RULES, describeRule, getRules } from '../domain/rulesEngine';
import type { AppData, RuleConfig } from '../domain/types';
import { useAppStore } from '../store/appStore';

const WEEKDAYS = ['日', '一', '二', '三', '四', '五', '六'];

// 排班规则：开关、拦截 / 只提示、参数。改完立刻对排班表和调班生效
export function RulesPage({ data }: { data: AppData }) {
  const updateRules = useAppStore((s) => s.updateRules);
  const rules = getRules(data);

  const update = (id: string, patch: Partial<RuleConfig>) => {
    updateRules(rules.map((r) => (r.id === id ? ({ ...r, ...patch } as RuleConfig) : r)));
  };

  const num = (rule: RuleConfig, key: string, label: string, min = 0) => (
    <label className="menu-row" style={{ gap: 4, fontSize: 12 }}>
      <span className="muted">{label}</span>
      <input
        className="input"
        type="number"
        min={min}
        style={{ width: 64, height: 28 }}
        value={(rule as unknown as Record<string, number>)[key]}
        onChange={(e) => e.target.value !== '' && update(rule.id, { [key]: Math.max(min, Number(e.target.value)) } as Partial<RuleConfig>)}
      />
    </label>
  );

  const text = (rule: RuleConfig, key: string, label: string, width = 70) => (
    <label className="menu-row" style={{ gap: 4, fontSize: 12 }}>
      <span className="muted">{label}</span>
      <input
        className="input"
        style={{ width, height: 28 }}
        value={(rule as unknown as Record<string, string>)[key]}
        onChange={(e) => update(rule.id, { [key]: e.target.value.trim() } as Partial<RuleConfig>)}
      />
    </label>
  );

  // 班次列表用顿号或逗号分隔
  const list = (rule: RuleConfig, key: string, label: string) => (
    <label className="menu-row" style={{ gap: 4, fontSize: 12 }}>
      <span className="muted">{label}</span>
      <input
        key={((rule as unknown as Record<string, string[]>)[key] ?? []).join('、')}
        className="input"
        style={{ width: 160, height: 28 }}
        defaultValue={((rule as unknown as Record<string, string[]>)[key] ?? []).join('、')}
        onBlur={(e) =>
          update(rule.id, {
            [key]: e.target.value
              .split(/[、,，\s]+/)
              .map((x) => x.trim())
              .filter(Boolean),
          } as Partial<RuleConfig>)
        }
      />
    </label>
  );

  const params = (rule: RuleConfig) => {
    switch (rule.type) {
      case 'maxConsecutiveWork':
        return (
          <>
            {num(rule, 'days', '最多连上(天)', 1)}
            <label className="menu-row" style={{ gap: 4, fontSize: 12 }}>
              <input type="checkbox" checked={rule.exemptNight} onChange={(e) => update(rule.id, { exemptNight: e.target.checked })} />
              夜班、帮夜不计入
            </label>
          </>
        );
      case 'roundGroupLimit':
        return (
          <>
            {list(rule, 'shifts', '班次')}
            {num(rule, 'max', '每轮最多')}
          </>
        );
      case 'dailyHeadcount':
        return (
          <>
            {text(rule, 'shift', '班次')}
            {num(rule, 'min', '每天最少')}
            {num(rule, 'max', '最多')}
          </>
        );
      case 'minRestDays':
        return (
          <>
            {num(rule, 'normal', '一般至少休')}
            {text(rule, 'withShift', '上')}
            {num(rule, 'withShiftDays', '的人至少休')}
          </>
        );
      case 'nightWholeWeek':
        return list(rule, 'shifts', '班次');
      case 'noShiftOnWeekday':
        return (
          <>
            {text(rule, 'shift', '班次')}
            <span className="menu-row" style={{ gap: 2, fontSize: 12 }}>
              <span className="muted">不能排在</span>
              {WEEKDAYS.map((w, i) => (
                <label key={w} className="menu-row" style={{ gap: 1 }}>
                  <input
                    type="checkbox"
                    checked={rule.weekdays.includes(i)}
                    onChange={(e) => update(rule.id, { weekdays: e.target.checked ? [...rule.weekdays, i] : rule.weekdays.filter((x) => x !== i) })}
                  />
                  周{w}
                </label>
              ))}
            </span>
          </>
        );
      case 'noConsecutiveShift':
        return text(rule, 'shift', '班次');
      case 'restAfterNight':
        return <span className="muted" style={{ fontSize: 12 }}>夜班后接着上夜班（如夜1 连上）不算违规</span>;
    }
  };

  return (
    <>
      <header className="page-header compact">
        <h1>规则</h1>
        <span className="muted">“拦截”的规则违反时不能改班、方案不列出；“只提示”的照常执行，但会提醒</span>
        <div className="spacer" />
        <SaveIndicator />
        <button
          className="btn"
          onClick={() => window.confirm('恢复成默认规则？当前的修改会丢掉。') && updateRules(DEFAULT_RULES.map((r) => ({ ...r })))}
        >
          恢复默认规则
        </button>
      </header>
      <div className="page-body">
        <div className="card">
          {rules.map((rule) => (
            <div
              key={rule.id}
              className="table-row"
              style={{ gridTemplateColumns: '40px minmax(260px, 1.4fr) 120px minmax(320px, 2fr)', minHeight: 48, opacity: rule.enabled ? 1 : 0.55 }}
            >
              <span className="center">
                <input type="checkbox" checked={rule.enabled} onChange={(e) => update(rule.id, { enabled: e.target.checked })} aria-label={`启用：${describeRule(rule)}`} />
              </span>
              <span style={{ fontSize: 13, fontWeight: 500 }}>{describeRule(rule)}</span>
              <span>
                <select
                  className="input"
                  style={{ height: 28, color: rule.severity === 'hard' ? 'var(--danger)' : 'var(--warn)' }}
                  value={rule.severity}
                  onChange={(e) => update(rule.id, { severity: e.target.value as RuleConfig['severity'] })}
                  aria-label="违反时"
                >
                  <option value="hard">拦截</option>
                  <option value="soft">只提示</option>
                </select>
              </span>
              <span className="menu-row" style={{ gap: 10, padding: '6px 8px' }}>
                {params(rule)}
              </span>
            </div>
          ))}
        </div>
        <div className="muted" style={{ fontSize: 12 }}>
          级别能上哪些班、哪些班不能调休、哪些班调休后要还班，在“班次”页里设置；个人只上 / 不上某些班，在“人员”页里设置。
        </div>
      </div>
    </>
  );
}
