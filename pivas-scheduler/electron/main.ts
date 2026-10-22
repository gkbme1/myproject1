import { app, BrowserWindow, ipcMain, Menu } from 'electron';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

// 数据文件放在 exe 所在目录旁边的 data 文件夹里（绿色版拷走就带走数据）；
// 装在 Program Files 这类没有写权限的目录时，退回到用户目录。
function resolveDataDir(): string {
  if (app.isPackaged) {
    // 便携版的 exe 会先解压到临时目录运行，要用它告诉我们的原始位置
    const exeDir = process.env.PORTABLE_EXECUTABLE_DIR || path.dirname(app.getPath('exe'));
    const besideExe = path.join(exeDir, 'data');
    try {
      fs.mkdirSync(besideExe, { recursive: true });
      fs.accessSync(besideExe, fs.constants.W_OK);
      return besideExe;
    } catch {
      // 没有写权限，走下面的用户目录
    }
  }
  const userDir = path.join(app.getPath('userData'), 'data');
  fs.mkdirSync(userDir, { recursive: true });
  return userDir;
}

const DATA_FILE_NAME = 'schedule-data.json';
const BACKUP_KEEP_DAYS = 30;

let dataDir = '';

// 先写临时文件再改名，断电或强退时不会留下写了一半的数据文件
function writeFileAtomic(filePath: string, content: string) {
  const tmpPath = filePath + '.tmp';
  fs.writeFileSync(tmpPath, content, 'utf8');
  fs.renameSync(tmpPath, filePath);
}

// 每天第一次保存时留一份备份，只保留最近 BACKUP_KEEP_DAYS 份
function backupOncePerDay(content: string) {
  const backupDir = path.join(dataDir, 'backups');
  fs.mkdirSync(backupDir, { recursive: true });
  const today = new Date();
  const stamp = `${today.getFullYear()}-${String(today.getMonth() + 1).padStart(2, '0')}-${String(today.getDate()).padStart(2, '0')}`;
  const backupFile = path.join(backupDir, `schedule-data-${stamp}.json`);
  if (fs.existsSync(backupFile)) {
    return;
  }
  writeFileAtomic(backupFile, content);
  const backups = fs
    .readdirSync(backupDir)
    .filter((name) => name.startsWith('schedule-data-') && name.endsWith('.json'))
    .sort();
  for (const oldName of backups.slice(0, Math.max(0, backups.length - BACKUP_KEEP_DAYS))) {
    fs.unlinkSync(path.join(backupDir, oldName));
  }
}

function registerIpc() {
  ipcMain.handle('data:load', () => {
    const filePath = path.join(dataDir, DATA_FILE_NAME);
    if (!fs.existsSync(filePath)) {
      return null;
    }
    return fs.readFileSync(filePath, 'utf8');
  });

  ipcMain.handle('data:save', (_event, content: string) => {
    writeFileAtomic(path.join(dataDir, DATA_FILE_NAME), content);
    backupOncePerDay(content);
    return { savedAt: new Date().toISOString() };
  });

  // 关窗口时界面还有没写盘的修改，用同步方式写完再关
  ipcMain.on('data:saveSync', (event, content: string) => {
    try {
      writeFileAtomic(path.join(dataDir, DATA_FILE_NAME), content);
      event.returnValue = true;
    } catch {
      event.returnValue = false;
    }
  });

  ipcMain.handle('data:dir', () => dataDir);
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1440,
    height: 900,
    minWidth: 1280,
    minHeight: 720,
    show: false,
    title: '静配中心排班系统',
    backgroundColor: '#EEF1F5',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });

  // 默认最大化铺满屏幕；F11 切换真正的全屏
  win.once('ready-to-show', () => {
    win.maximize();
    win.show();
  });
  win.webContents.on('before-input-event', (_event, input) => {
    if (input.type === 'keyDown' && input.key === 'F11') {
      win.setFullScreen(!win.isFullScreen());
    }
  });

  // 只加载本地页面，不允许跳转到外部网址
  win.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  win.webContents.on('will-navigate', (event) => event.preventDefault());

  const devServerUrl = process.env.VITE_DEV_SERVER_URL;
  if (devServerUrl) {
    win.loadURL(devServerUrl);
  } else {
    win.loadFile(path.join(__dirname, '..', 'dist', 'index.html'));
  }
}

// Win7（内核版本 6.1）的老显卡驱动容易白屏，关掉硬件加速
if (os.release().startsWith('6.1')) {
  app.disableHardwareAcceleration();
}

Menu.setApplicationMenu(null);

if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on('second-instance', () => {
    const win = BrowserWindow.getAllWindows()[0];
    if (win) {
      if (win.isMinimized()) win.restore();
      win.focus();
    }
  });

  app.whenReady().then(() => {
    dataDir = resolveDataDir();
    registerIpc();
    createWindow();
  });

  app.on('window-all-closed', () => app.quit());
}
