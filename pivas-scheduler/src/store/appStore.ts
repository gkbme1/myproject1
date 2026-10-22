import { create } from 'zustand';
import { lastAppliedIndex, makeChangeSet, redoCandidateIndex, roundChanges } from '../domain/schedule';
import type { ChangeExtra } from '../domain/schedule';
import type { AppData, AppSettings, CellChange, ChangeKind, Person, Round, RuleConfig, ShiftId, ShiftType } from '../domain/types';
import { createSeedData } from '../domain/seed';
import { saveAppData, saveAppDataSync } from '../persistence/storage';

export type PageId = 'schedule' | 'people' | 'shifts' | 'rules' | 'stats' | 'data' | 'settings';
export type ScheduleTab = 'grid' | 'history' | 'requests';
export type SaveStatus = 'idle' | 'saving' | 'saved' | 'error';

interface AppState {
  data: AppData | null;
  loadError: string;
  page: PageId;
  scheduleTab: ScheduleTab;
  saveStatus: SaveStatus;
  savedAt: string | null;
  saveError: string;
  showResume: boolean;
  previousActiveAt: string | null;
  historyFocus: { personId: string; date: string } | null; // 修改历史页顶部要展开修改链的格子

  openCellHistory(personId: string, date: string): void;
  clearHistoryFocus(): void;

  needsFirstImport: boolean; // 第一次使用，还没有任何排班数据

  init(data: AppData | null): void;
  replaceData(data: AppData): void; // 导入 Excel 后整体换成新数据
  loadSampleData(): void;
  setLoadError(message: string): void;
  setPage(page: PageId): void;
  setScheduleTab(tab: ScheduleTab): void;
  dismissResume(): void;

  commitChange(kind: ChangeKind, title: string, changes: CellChange[], extra?: ChangeExtra): void;
  updateRules(rules: RuleConfig[]): void;
  updateSettings(patch: Partial<AppSettings>): void;
  switchRound(roundId: string): void;
  addRound(round: Round): void;
  addPerson(person: Person, addToCurrentRound: boolean): void;
  setRoundPersonOrder(personIds: string[]): void;
  restoreData(data: AppData): void;
  undo(): void;
  redo(): void;

  updateShift(shiftId: ShiftId, patch: Partial<ShiftType>): void;
  addShift(shift: ShiftType): void;
  updatePerson(personId: string, patch: Partial<Person>): void;
}

const SAVE_DELAY_MS = 300;
let saveTimer: ReturnType<typeof setTimeout> | null = null;

// 关闭窗口时如果还有排队中的保存，立刻同步写掉，保证最后一步不丢
window.addEventListener('beforeunload', () => {
  if (!saveTimer) return;
  clearTimeout(saveTimer);
  saveTimer = null;
  const data = useAppStore.getState().data;
  if (data) saveAppDataSync({ ...data, lastActiveAt: new Date().toISOString() });
});

export const useAppStore = create<AppState>((set, get) => {
  // 每次数据变化都安排一次保存；连续操作只写最后一次
  const scheduleSave = () => {
    if (saveTimer) clearTimeout(saveTimer);
    set({ saveStatus: 'saving' });
    saveTimer = setTimeout(async () => {
      saveTimer = null;
      const data = get().data;
      if (!data) return;
      try {
        const savedAt = await saveAppData({ ...data, lastActiveAt: new Date().toISOString() });
        set({ saveStatus: 'saved', savedAt, saveError: '' });
      } catch (e) {
        set({ saveStatus: 'error', saveError: e instanceof Error ? e.message : String(e) });
      }
    }, SAVE_DELAY_MS);
  };

  const updateData = (fn: (data: AppData) => AppData) => {
    const data = get().data;
    if (!data) return;
    set({ data: fn(data) });
    scheduleSave();
  };

  return {
    data: null,
    loadError: '',
    page: 'schedule',
    scheduleTab: 'grid',
    saveStatus: 'idle',
    savedAt: null,
    saveError: '',
    showResume: false,
    previousActiveAt: null,
    historyFocus: null,
    needsFirstImport: false,

    openCellHistory(personId, date) {
      set({ page: 'schedule', scheduleTab: 'history', historyFocus: { personId, date } });
    },
    clearHistoryFocus() {
      set({ historyFocus: null });
    },

    init(data) {
      if (!data) {
        set({ needsFirstImport: true });
        return;
      }
      set({
        data,
        previousActiveAt: data.lastActiveAt,
        showResume: !!data.lastActiveAt,
      });
      scheduleSave();
    },
    replaceData(data) {
      set({ data, needsFirstImport: false, page: 'schedule', scheduleTab: 'grid', historyFocus: null, showResume: false });
      scheduleSave();
    },
    loadSampleData() {
      set({ data: createSeedData(), needsFirstImport: false, page: 'schedule', scheduleTab: 'grid' });
      scheduleSave();
    },
    setLoadError(message) {
      set({ loadError: message });
    },
    setPage(page) {
      set({ page });
    },
    setScheduleTab(tab) {
      set({ page: 'schedule', scheduleTab: tab });
    },
    dismissResume() {
      set({ showResume: false });
    },

    commitChange(kind, title, changes, extra = {}) {
      if (changes.length === 0) return;
      updateData((data) => ({
        ...data,
        changes: [...data.changes, makeChangeSet(data.currentRoundId, kind, title, changes, { operator: data.settings?.operator, ...extra })],
      }));
    },
    updateRules(rules) {
      updateData((data) => ({ ...data, rules }));
    },
    updateSettings(patch) {
      updateData((data) => ({ ...data, settings: { operator: '管理员', ...data.settings, ...patch } }));
    },
    switchRound(roundId) {
      updateData((data) => ({ ...data, currentRoundId: roundId }));
      set({ historyFocus: null });
    },
    addRound(round) {
      updateData((data) => ({ ...data, rounds: [...data.rounds, round], currentRoundId: round.id }));
      set({ page: 'schedule', scheduleTab: 'grid', historyFocus: null });
    },
    addPerson(person, addToCurrentRound) {
      updateData((data) => ({
        ...data,
        people: [...data.people, person],
        rounds: addToCurrentRound
          ? data.rounds.map((r) => (r.id === data.currentRoundId && r.personIds ? { ...r, personIds: [...r.personIds, person.id] } : r))
          : data.rounds,
      }));
    },
    setRoundPersonOrder(personIds) {
      updateData((data) => ({
        ...data,
        rounds: data.rounds.map((r) => (r.id === data.currentRoundId ? { ...r, personIds } : r)),
      }));
    },
    restoreData(data) {
      set({ data, page: 'schedule', scheduleTab: 'grid', historyFocus: null, showResume: false });
      scheduleSave();
    },

    undo() {
      updateData((data) => {
        const own = roundChanges(data, data.currentRoundId);
        const idx = lastAppliedIndex(own);
        if (idx < 0) return data;
        const targetId = own[idx].id;
        return {
          ...data,
          changes: data.changes.map((c) => (c.id === targetId ? { ...c, undone: true, undoneAt: new Date().toISOString() } : c)),
        };
      });
    },

    redo() {
      updateData((data) => {
        const own = roundChanges(data, data.currentRoundId);
        const idx = redoCandidateIndex(own);
        if (idx < 0) return data;
        const targetId = own[idx].id;
        return {
          ...data,
          changes: data.changes.map((c) => (c.id === targetId ? { ...c, undone: false, undoneAt: undefined } : c)),
        };
      });
    },

    updateShift(shiftId, patch) {
      updateData((data) => ({
        ...data,
        shifts: data.shifts.map((s) => (s.id === shiftId ? { ...s, ...patch } : s)),
      }));
    },
    addShift(shift) {
      updateData((data) => ({ ...data, shifts: [...data.shifts, shift] }));
    },
    updatePerson(personId, patch) {
      updateData((data) => ({
        ...data,
        people: data.people.map((p) => (p.id === personId ? { ...p, ...patch } : p)),
      }));
    },
  };
});
