# Decisions

Record durable project, architecture, product, workflow, and technical decisions here.

## 2026-09-19 — Recurrence replaces frequency/days strings
`Chore.frequency` (freeform string) + `Chore.days` (string list) were never actually read
back by any scheduling logic — they were display-only. Replaced with a structured
`Recurrence` struct (Once/Daily/Weekly[+interval weeks/specific weekdays]/Monthly) that
the new due-date rollover engine (`ChoreManager::computeNextDueDate`) actually consumes.
Legacy household files are migrated automatically on load (`Recurrence::fromLegacy`), so
no manual data migration step is needed.

## 2026-09-19 — Theme is a global app setting, not per-profile or per-household
Previously `Client` (the user profile) had a light/dark `theme` field. Redesigned as 5
built-in named presets (`ThemeManager`) applied globally and persisted in
`app_state.json`, not tied to a household or profile — reasoning: the theme is a display
preference of whoever is using the app right now, not household data that should travel
with a specific family's chore list.

## 2026-09-19 — Theme switch rebuilds the UI rather than patching live widgets
Some widgets (e.g. `RoundedButton`'s cached base color, `wxStaticText` foreground colors
set at construction) don't re-read `Palette::*` on every paint. Rather than auditing and
patching every such call site (and keeping that list correct forever), a theme switch
destroys and rebuilds the entire content area (`RebuildThemedUI`) — consistent with the
app's existing "destroy and recreate from scratch" pattern already used by every
`Refresh*List()`.

## 2026-09-19 — Sidebar navigation replaces the top wxNotebook tab strip
Switched from `wxNotebook` to a custom left-hand `SidebarItemPanel` list driving a
`wxSimplebook`, to make room for a 4th top-level view (Today) without a cramped tab bar,
and to match the card-based "bubbly" visual language used elsewhere instead of native
tab chrome.

## 2026-09-19 — Assignment is implicit, not a stored field
A chore's assigned doer is still derived from which doer's `assignedChores` container
holds it (not a field on `Chore` itself). `ChoreManager::findAssignedDoer` /
`getAssignedDoerName` were added as the single place that answers "who has this chore,"
so the UI and `assignChoreToDoer` never re-derive that scan independently — kept the
existing storage model rather than introducing a redundant `assignedDoerId` field that
could drift out of sync.

## Prior architectural context (from commit history, not re-litigated this session)
- `fd3d67a` (2026-08-09): rewrote the original console app as a wxWidgets desktop GUI;
  introduced the category registry (replacing fixed Easy/Medium/Hard subclasses),
  multi-household support (one JSON file per household via `HouseholdRegistry`), and the
  "bubbly" custom-drawn visual language (`Palette`, `RoundedButton`, `CardPanel`).
- `32d5a21` (2026-08-22): finished applying that visual language app-wide and replaced
  the native toolbar with a custom header bar.
