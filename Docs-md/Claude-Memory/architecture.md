---
name: architecture
description: "ChoreConsole's domain model, persistence layout, and UI architecture (wxWidgets desktop app)"
metadata: 
  node_type: memory
  type: project
  originSessionId: 060144eb-cee9-4c95-b5b7-672b9733fc62
  modified: 2026-09-19T20:16:29.873Z
---

# Architecture

ChoreConsole is a Windows desktop wxWidgets/C++20 app (rewritten from an original console
prototype in [[project_management]]'s commit history). Two source files carry almost
everything:

- **ChoreModel.h** — domain model, namespace `ChoreApp`. Key types: `Chore` (with a nested
  `Recurrence` struct: Once/Daily/Weekly[+interval weeks/specific weekdays]/Monthly) and
  its `EasyChore`/`MediumChore`/`HardChore` subclasses, `ChoreDoer`, generic `Container<T>`,
  `ChoreManager` (owns one household's chores/doers/history, JSON load/save, and the
  due-date rollover engine), and `HouseholdRegistry` (tracks known households, which was
  last open, and the active theme name).
- **main.cpp** — all wxWidgets UI. Custom-drawn "bubbly" visual language: `Palette`
  (mutable `wxColour` globals, swappable at runtime by `ThemeManager`), `RoundedButton`,
  `CardPanel`, `AvatarCircle`, `DoerCardPanel`, `SidebarItemPanel`. Left-hand sidebar
  (`SidebarItemPanel` list) drives a `wxSimplebook` across four views: Today, Chores,
  Chore Doers, History.

## Persistence
- One JSON file per household under `TestData/households/`, written via
  `ChoreManager::saveData()` (now returns `bool` so callers can detect a failed write).
- `TestData/app_state.json` holds the household registry (`last_open_household_file`) and
  the globally-active theme name (`theme`) — rewritten wholesale by
  `HouseholdRegistry::saveAppState()` so adding a new persisted field never risks clobbering
  another one written by a different setter.
- `json.hpp` (nlohmann/json) is the only JSON library used.

## Key model decisions worth knowing before touching this code
- A chore's **assignment is implicit**: there's no `assignedDoerId` field on `Chore` — it's
  derived from which `ChoreDoer::assignedChores` container currently holds the
  `shared_ptr<Chore>`. `ChoreManager::findAssignedDoer` / `getAssignedDoerName` are the one
  place that answers "who has this chore"; go through them rather than re-scanning.
- **Recurrence** (not the old freeform `frequency` string + `days` list) is what the
  due-date engine (`ChoreManager::computeNextDueDate`, `checkAndResetDueChores`) actually
  reads. Weekday names are always stored/compared in English (`"Monday"`..`"Sunday"`) and
  formatted with `std::locale::classic()` specifically so a non-English Windows display
  language can't silently break weekday matching.
- **Theme** is a global app preference (`ThemeManager`, 5 built-in presets), not per-user
  or per-household. Switching themes destroys and rebuilds the whole content area
  (`MainFrame::RebuildThemedUI`) rather than patching every widget in place, because some
  widgets cache a `Palette::*` color at construction instead of re-reading it per paint.

See [[coding]] for implementation conventions. Full rationale for each decision above is
in `Docs-md/DECISIONS.md`.
