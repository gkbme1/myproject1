import { useMemo } from 'react';
import { roundDates } from '../../domain/dates';
import { cellLastRole, currentCells, getRound, roundChanges } from '../../domain/schedule';
import type { AppData, CellValue, Level, Person, ShiftType } from '../../domain/types';
import { cellKey } from '../../domain/types';

export interface PersonGroup {
  level: Level;
  people: Person[];
}

// 排班表各处共用的派生数据，数据不变就不重复计算
export function useScheduleView(data: AppData) {
  return useMemo(() => {
    const round = getRound(data);
    const dates = roundDates(round.startDate, round.days);
    const cells = currentCells(data, round.id);
    const roles = cellLastRole(data, round.id);
    const shiftsById = new Map<string, ShiftType>(data.shifts.map((s) => [s.id, s]));
    const peopleById = new Map<string, Person>(data.people.map((p) => [p.id, p]));
    const levelsById = new Map<string, Level>(data.levels.map((l) => [l.id, l]));
    const changes = roundChanges(data, round.id);

    // 本轮改过的格子：至少有一条未撤销的修改落在这个格子上
    const modified = new Set<string>();
    for (const cs of changes) {
      if (cs.undone) continue;
      for (const ch of cs.changes) modified.add(cellKey(ch.personId, ch.date));
    }

    // 这一轮表里的人，按导入时 Excel 的行序；老数据没有记录就按人员列表
    const roundPeople = round.personIds
      ? round.personIds.map((id) => peopleById.get(id)).filter((p): p is Person => !!p)
      : [...data.people].sort((a, b) => a.order - b.order);
    const rowNumber = new Map(roundPeople.map((p, i) => [p.id, i + 1]));

    const groups: PersonGroup[] = [...data.levels]
      .sort((a, b) => a.order - b.order)
      .map((level) => ({ level, people: roundPeople.filter((p) => p.levelId === level.id) }))
      .filter((g) => g.people.length > 0);

    const shiftOfCell = (value: CellValue | undefined): ShiftType | null =>
      value && 'shiftId' in value ? shiftsById.get(value.shiftId) ?? null : null;

    const rowShifts = (personId: string) => dates.map((d) => shiftOfCell(cells[cellKey(personId, d)]));

    const appliedCount = changes.filter((c) => !c.undone).length;

    return { round, dates, cells, roles, shiftsById, peopleById, levelsById, roundPeople, rowNumber, changes, modified, groups, shiftOfCell, rowShifts, appliedCount };
  }, [data]);
}

export type ScheduleView = ReturnType<typeof useScheduleView>;
