import { describe, expect, it } from 'vitest';
import { roundDates } from './dates';
import { generateNextRound, rotateWithLocks } from './rounds';
import { currentCells } from './schedule';
import { createSeedData } from './seed';
import { cellKey } from './types';

describe('生成下一轮', () => {
  it('能轮转的人往下挪一位，锁定的人留在原位', () => {
    expect(rotateWithLocks(['a', 'b', 'c', 'd'], () => false)).toEqual(['d', 'a', 'b', 'c']);
    expect(rotateWithLocks(['a', 'L', 'c', 'd'], (x) => x === 'L')).toEqual(['d', 'L', 'a', 'c']);
  });

  it('班不动人滚动：新一轮第 k 行的人用原来第 k 行的班；调休变空白格；不轮班的人不动', () => {
    const data = createSeedData();
    const cur = data.rounds[0];
    const { round, clearedLeaveCells } = generateNextRound(data, { useModified: false });
    const oldDates = roundDates(cur.startDate, cur.days);
    const newDates = roundDates(round.startDate, round.days);
    expect(round.startDate).toBe('2026-11-02');
    expect(round.index).toBe(cur.index + 1);

    const name = (n: string) => data.people.find((p) => p.name === n)!;
    // 技术员 5 人：张敏 王磊 刘洋 陈静 杨帆 → 杨帆 张敏 王磊 刘洋 陈静
    const levelOrder = round.personIds!.map((id) => data.people.find((p) => p.id === id)!).filter((p) => p.levelId === 'L1');
    expect(levelOrder.map((p) => p.name)).toEqual(['杨帆', '张敏', '王磊', '刘洋', '陈静']);
    // 张敏现在在第 2 行，用的是原来王磊那一行（夜1 × 7）
    expect(round.base[cellKey(name('张敏').id, newDates[0])]).toEqual(cur.base[cellKey(name('王磊').id, oldDates[0])]);
    // 方海利不轮班，留在原位
    const l2 = round.personIds!.map((id) => data.people.find((p) => p.id === id)!).filter((p) => p.levelId === 'L2');
    expect(l2.map((p) => p.name)).toEqual(['孙浩', '赵琳', '方海利']);
    // 刘洋原来那一行第 12 天是调休，新一轮接这一行的人那天是空白格
    expect(round.base[cellKey(name('王磊').id, newDates[11])]).toBeNull();
    expect(clearedLeaveCells).toBeGreaterThan(0);
  });

  it('选“用修改后的排班”时，本轮的修改会带过去', () => {
    const data = createSeedData();
    const cur = data.rounds[0];
    const zhang = data.people.find((p) => p.name === '张敏')!;
    const d0 = roundDates(cur.startDate, cur.days)[0];
    const s32 = data.shifts.find((s) => s.name === '3-2')!;
    data.changes.push({ id: 'c1', roundId: cur.id, kind: 'manual', title: '', note: '', at: '', operator: '', undone: false, changes: [{ personId: zhang.id, date: d0, from: null, to: { shiftId: s32.id }, role: 'manual' }] });
    expect(currentCells(data)[cellKey(zhang.id, d0)]).toEqual({ shiftId: s32.id });
    const { round } = generateNextRound(data, { useModified: true });
    // 张敏原来那一行（第 1 行）由杨帆接
    const yang = data.people.find((p) => p.name === '杨帆')!;
    expect(round.base[cellKey(yang.id, round.startDate)]).toEqual({ shiftId: s32.id });
  });
});
