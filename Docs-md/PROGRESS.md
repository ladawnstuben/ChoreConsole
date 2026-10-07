# Progress

Last updated: 2026-10-07

## Current objective
Recurrence/theme/navigation feature set is committed and pushed (2026-10-07). No active
objective; waiting on the user to pick the next feature (see NEXT_STEPS.md).

## Completed
- **Recurrence engine** (`ChoreModel.h`): `Recurrence` struct (Once/Daily/Weekly[+interval
  weeks/weekdays]/Monthly) replaces the old freeform `frequency` string + `days` list.
  Legacy household files are migrated on load via `Recurrence::fromLegacy`. Chores track
  `lastCompletedDate`; `ChoreManager::computeNextDueDate` / `checkAndResetDueChores` reset
  a completed recurring chore back to Not Started once its next occurrence is due (checked
  at startup, on household switch, and via a 5-minute `wxTimer` while the app is open).
- **Direct chore assignment**: `ChoreManager::assignChoreToDoer` / `unassignChore` let a
  chore be given to or taken from one doer directly (menu item, "Assign To..." button,
  double-click/Enter, right-click context menu, and bulk multi-select), independent of
  the pre-existing "assign randomly" flow. `findAssignedDoer` / `getAssignedDoerName` are
  the single source of truth for "who has this chore" (assignment is implicit in which
  doer's container holds the `shared_ptr<Chore>`, not stored on the Chore itself).
- **Today tab**: new home view (first sidebar item) showing what's due today grouped by
  doer, with one-tap "Mark Done", and three stat tiles (due today / completed today / top
  streak).
- **Sidebar navigation**: left-hand `SidebarItemPanel` list driving a `wxSimplebook`
  (Today / Chores / Chore Doers / History) replaces the old top `wxNotebook` tab strip.
- **Theme system**: `Theme` / `ThemeManager` with 5 built-in presets (Warm Bubbly,
  Midnight, Minimal Monochrome, Scandinavian Calm, Corporate Clean). Selecting one
  (View > Theme) overwrites the mutable `Palette::*` globals and rebuilds the whole
  content area (`RebuildThemedUI`) since some widgets cache a color at construction.
  Persisted globally (not per-household) in `app_state.json`'s new `"theme"` field via
  `HouseholdRegistry::setThemeName` / `saveAppState()`.
- **Inline chore detail/edit panel**: replaces the old read-only detail textbox on the
  Chores tab with live editable fields (name, category, assigned-to, priority, status,
  earnings, location, notes), writing straight through to the model + `saveData()`.
- **Bulk chore actions**: checkbox multi-select on the chores list with an action bar
  (Assign to... / Mark Done / Delete).
- **Status filters** on the Chores tab (All / Not Started / In Progress / Completed).
- **Doer milestone badges**: `getDoerLifetimeCompletions` + `ComputeDoerBadgeText` show
  a streak or lifetime-completion badge on doer cards/profile.
- **Starter chores**: `ChoreManager::addStarterChores()` seeds 11 curated chores spanning
  every recurrence type, offered when creating a new household or via "Load Starter
  Chores..." in the Chores menu.
- **saveData() now returns bool**; `OnSave` surfaces a failure dialog instead of silently
  swallowing a write error.
- Removed the per-profile light/dark `theme` field from `Client`/`ProfileDialog` (theme is
  now the global, app-wide concern above, not a per-user preference).
- **README.md** updated to describe all of the above (previously described the older
  frequency/notebook-tab feature set).
- Verified the whole diff builds clean (`MSBuild ... /p:Configuration=Debug /p:Platform=x64`,
  0 warnings/errors) and has actually been run: `TestData/app_state.json` already carries
  `"theme": "Warm Bubbly"` and `TestData/households/default.json` already has
  `recurrence`/`last_completed_date` fields on its chores, from a prior real session.
- Set up `Docs-md/` persistence (this file, `NEXT_STEPS.md`, `DECISIONS.md`, `WORK_LOG.md`,
  `Claude-Memory/*`) — this repo had no prior durable session state.

## In progress
- Nothing. Working tree clean after the 2026-10-07 commit + push.

## Blockers / open questions
- None.
