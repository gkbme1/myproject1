/// <reference types="vitest" />
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Electron 22 内核是 Chromium 109，打包目标按它来，保证 Win7 上能跑。
// base 用相对路径，打包后通过 file:// 加载 dist/index.html。
export default defineConfig({
  base: './',
  plugins: [react()],
  build: {
    target: 'chrome109',
    outDir: 'dist',
    emptyOutDir: true,
  },
  server: { port: 5173, strictPort: true },
  test: { include: ['src/**/*.test.ts'] },
});
