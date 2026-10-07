---
name: coding
description: "ChoreConsole implementation conventions — build tooling, UI refresh idiom, and recurring gotchas"
metadata: 
  node_type: memory
  type: project
  originSessionId: 060144eb-cee9-4c95-b5b7-672b9733fc62
  modified: 2026-09-19T20:16:44.208Z
---

# Coding / Implementation

## Build
- Visual Studio 2022 (v143 toolset), C++20, vcpkg manifest mode (`vcpkg.json`) for
  wxWidgets. Build from the command line with:
  `"C:\Program Files\Microsoft Visual Studio\2022\<Edition>\MSBuild\Current\Bin\amd64\MSBuild.exe" ChoreConsole.sln /p:Configuration=Debug /p:Platform=x64`
- The `AppLocalFromInstalled` post-build step invokes `pwsh.exe` and fails with exit 9009
  if PowerShell 7 isn't on PATH — MSBuild falls back to Windows PowerShell automatically
  and the build still succeeds; this is a harmless warning, not a real build failure.

## UI conventions (main.cpp, `ChoreApp` namespace)
- **Destroy-and-recreate over patch-in-place** is the standing idiom: every
  `Refresh*List()` clears and rebuilds its child windows from scratch rather than
  diffing. A full theme switch follows the same pattern at a larger scale
  (`MainFrame::RebuildThemedUI` destroys/rebuilds the whole content area) — this is
  consistent with existing app conventions, not a special case.
- **Per-keystroke commit**: free-text fields (doer notes, the inline chore detail panel's
  Name/Location/Notes/typed-Category) write straight through to the model + `saveData()`
  on every `wxEVT_TEXT`, with a `suppressDetailEvents`-style guard to avoid re-entrant
  writes while a form is being programmatically populated.
- **Always refresh every affected view after a mutation**: an action on one tab (e.g.
  assigning a chore) typically needs to refresh Chores, Chore Doers, History, *and*
  Today, plus the inline detail panel if it's showing the affected chore — grep existing
  handlers (e.g. `AssignSelectedChore`, `FinishBulkAction`) for the current full list
  rather than guessing which views are "obviously" affected.
- Weekday names must always be produced/compared in the `"C"`/classic locale
  (`ss.imbue(std::locale::classic())`) — see [[architecture]] for why.
- A lambda captured by a button's `wxEVT_BUTTON` handler must copy any string it needs
  (not capture by reference) when the handler itself triggers a `Refresh*()` that destroys
  the button — see the `doerNameParam` comment in `OnTodayMarkDone` for the concrete
  use-after-free this avoids.

## Testing
See [[testing]] — no automated test suite exists yet; verification is build + manual run,
cross-checked against `TestData/` runtime files actually changing shape as expected.
