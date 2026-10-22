import { createSeedData } from '../domain/seed';
import type { AppData } from '../domain/types';

// Electron 里通过 preload 暴露的接口读写 exe 旁边的数据文件；
// 用浏览器直接打开开发页面时，退回到 localStorage，方便调试界面。

interface PivasApi {
  loadData(): Promise<string | null>;
  saveData(content: string): Promise<{ savedAt: string }>;
  saveDataSync(content: string): boolean;
  getDataDir(): Promise<string>;
}

declare global {
  interface Window {
    pivasApi?: PivasApi;
  }
}

const LOCAL_KEY = 'pivas-scheduler-data';

export interface LoadResult {
  data: AppData;
  isFirstRun: boolean;
}

export async function loadAppData(): Promise<LoadResult> {
  let raw: string | null = null;
  if (window.pivasApi) {
    raw = await window.pivasApi.loadData();
  } else {
    try {
      raw = window.localStorage.getItem(LOCAL_KEY);
    } catch {
      raw = null;
    }
  }
  if (!raw) {
    return { data: createSeedData(), isFirstRun: true };
  }
  const parsed = JSON.parse(raw) as AppData;
  if (parsed.schemaVersion !== 1) {
    throw new Error(`数据文件版本 ${parsed.schemaVersion} 不认识，请升级软件`);
  }
  return { data: parsed, isFirstRun: false };
}

export async function saveAppData(data: AppData): Promise<string> {
  const content = JSON.stringify(data);
  if (window.pivasApi) {
    const result = await window.pivasApi.saveData(content);
    return result.savedAt;
  }
  window.localStorage.setItem(LOCAL_KEY, content);
  return new Date().toISOString();
}

// 关闭窗口前把还没写盘的修改同步写掉
export function saveAppDataSync(data: AppData): void {
  const content = JSON.stringify(data);
  if (window.pivasApi) {
    window.pivasApi.saveDataSync(content);
  } else {
    window.localStorage.setItem(LOCAL_KEY, content);
  }
}
