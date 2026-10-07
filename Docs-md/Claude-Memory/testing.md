---
name: testing
description: "How ChoreConsole changes get verified — no automated tests, build + runtime-artifact cross-check instead"
metadata: 
  node_type: memory
  type: project
  originSessionId: 060144eb-cee9-4c95-b5b7-672b9733fc62
  modified: 2026-09-19T20:16:53.425Z
---

# Testing / Validation

There is no automated test suite for this project. Validation is:

1. **Build clean**: `MSBuild ChoreConsole.sln /p:Configuration=Debug /p:Platform=x64`
   (see [[coding]] for the exact command and the harmless `pwsh.exe` post-build warning).
   Zero warnings/errors is the bar — this codebase currently builds with none, so a new
   warning is worth investigating, not ignoring.
2. **Runtime cross-check via TestData/**: since there's no test harness, whether a feature
   actually ran (not just compiled) can often be confirmed by inspecting the runtime data
   files it should have touched — e.g. `TestData/app_state.json` picking up a new `"theme"`
   key, or `TestData/households/*.json` chores gaining `recurrence`/`last_completed_date`
   fields. These files are gitignored/untracked, so they reflect real local runs, not
   fixtures — a useful signal for "was this exercised for real" when reconstructing session
   state, but don't rely on them as a substitute for actually launching the app to check UI
   behavior.
3. For UI changes, launch the built `x64\Debug\ChoreConsole.exe` and exercise the golden
   path plus edge cases by hand — per the general "test the UI before claiming success"
   rule.
