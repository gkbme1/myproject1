import type { ReactNode } from 'react';
import type { PageId } from '../store/appStore';
import { useAppStore } from '../store/appStore';

const svgProps = { viewBox: '0 0 24 24', fill: 'none', stroke: 'currentColor', strokeWidth: 1.8, 'aria-hidden': true } as const;

const ICONS: Record<PageId, ReactNode> = {
  schedule: (
    <svg {...svgProps}>
      <rect x="3" y="4" width="18" height="17" rx="2" />
      <path d="M3 9h18M8 2v4M16 2v4M8 13h3M13 13h3M8 17h3" />
    </svg>
  ),
  leave: (
    <svg {...svgProps}>
      <rect x="3" y="4" width="18" height="17" rx="2" />
      <path d="M3 9h18M8 2v4M16 2v4M8.5 14.5l2.5 2.5 4.5-5" />
    </svg>
  ),
  people: (
    <svg {...svgProps}>
      <circle cx="9" cy="8" r="3.5" />
      <path d="M2.5 20c.8-3.5 3.4-5.5 6.5-5.5s5.7 2 6.5 5.5M16 4.5a3.5 3.5 0 0 1 0 7M18 14.8c2 .7 3.2 2.5 3.6 5.2" />
    </svg>
  ),
  shifts: (
    <svg {...svgProps}>
      <rect x="3" y="3" width="7" height="7" rx="1" />
      <rect x="14" y="3" width="7" height="7" rx="1" />
      <rect x="3" y="14" width="7" height="7" rx="1" />
      <path d="M17.5 14v7M14 17.5h7" />
    </svg>
  ),
  rules: (
    <svg {...svgProps}>
      <path d="M12 3l8 3v6c0 4.5-3.4 8-8 9-4.6-1-8-4.5-8-9V6z" />
      <path d="M8.5 12l2.5 2.5 4.5-5" />
    </svg>
  ),
  stats: (
    <svg {...svgProps}>
      <path d="M4 20V10M10 20V4M16 20v-7M22 20H2" />
    </svg>
  ),
  data: (
    <svg {...svgProps}>
      <ellipse cx="12" cy="5.5" rx="8" ry="2.5" />
      <path d="M4 5.5v13c0 1.4 3.6 2.5 8 2.5s8-1.1 8-2.5v-13M4 12c0 1.4 3.6 2.5 8 2.5s8-1.1 8-2.5" />
    </svg>
  ),
  settings: (
    <svg {...svgProps}>
      <circle cx="12" cy="12" r="3" />
      <path d="M12 2v3M12 19v3M2 12h3M19 12h3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M4.9 19.1L7 17M17 7l2.1-2.1" />
    </svg>
  ),
};

export const PAGE_NAMES: Record<PageId, string> = {
  schedule: '排班',
  leave: '假期',
  people: '人员',
  shifts: '班次',
  rules: '规则',
  stats: '统计',
  data: '数据',
  settings: '设置',
};

const TOP_ITEMS: PageId[] = ['schedule', 'leave', 'people', 'shifts', 'rules', 'stats', 'data'];

export function NavRail() {
  const page = useAppStore((s) => s.page);
  const setPage = useAppStore((s) => s.setPage);

  const item = (id: PageId) => (
    <button key={id} className={'rail-item' + (page === id ? ' on' : '')} onClick={() => setPage(id)} aria-current={page === id ? 'page' : undefined}>
      {ICONS[id]}
      {PAGE_NAMES[id]}
    </button>
  );

  return (
    <nav className="rail" aria-label="全局导航">
      <div className="rail-logo">静配</div>
      {TOP_ITEMS.map(item)}
      <div className="spacer" />
      {item('settings')}
    </nav>
  );
}
