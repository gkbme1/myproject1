import { contextBridge, ipcRenderer } from 'electron';

// 界面只能通过这几个函数读写数据文件，拿不到 Node 的其它能力
contextBridge.exposeInMainWorld('pivasApi', {
  loadData: (): Promise<string | null> => ipcRenderer.invoke('data:load'),
  saveData: (content: string): Promise<{ savedAt: string }> => ipcRenderer.invoke('data:save', content),
  saveDataSync: (content: string): boolean => ipcRenderer.sendSync('data:saveSync', content),
  getDataDir: (): Promise<string> => ipcRenderer.invoke('data:dir'),
});
