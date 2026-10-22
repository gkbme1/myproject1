@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

rem 打 Windows 安装包和绿色版（32 位 + 64 位），结果在 release 文件夹
rem 用国内镜像下载 Electron 和打包工具，避免从 GitHub 下载太慢或失败
set ELECTRON_MIRROR=https://npmmirror.com/mirrors/electron/
set ELECTRON_BUILDER_BINARIES_MIRROR=https://npmmirror.com/mirrors/electron-builder-binaries/

where node >nul 2>nul
if errorlevel 1 (
  echo 没有找到 Node.js，请先安装 Node.js（https://nodejs.org，选 LTS 版本）后再运行。
  pause
  exit /b 1
)

echo [1/2] 安装依赖（第一次比较慢，之后会快很多）...
call npm install --no-audit --no-fund
if errorlevel 1 (
  echo 安装依赖失败，请把上面的报错截图发给开发人员。
  pause
  exit /b 1
)

echo [2/2] 编译并打包...
call npm run dist:win
if errorlevel 1 (
  echo 打包失败，请把上面的报错截图发给开发人员。
  pause
  exit /b 1
)

echo 打包完成，安装包和绿色版在 release 文件夹里。
start "" "%~dp0release"
pause
