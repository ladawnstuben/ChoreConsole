---
name: project_management
description: ChoreConsole milestone history and current git/session state
metadata: 
  node_type: memory
  type: project
  originSessionId: 060144eb-cee9-4c95-b5b7-672b9733fc62
  modified: 2026-09-19T20:17:14.525Z
---

# Project Management

ChoreConsole is a family chore-management desktop tool for Wayne. Solo hobby-scale
project; commit history shows the pattern of one Claude-assisted session per major
feature push, with a README refresh bundled into the commit once the feature is done.

## Milestone history (from git log, not re-verified each session — check `git log` for
current truth)
- `33cc102`/`f384646` — initial console-app project scaffold.
- `44e4055` — "Finished the application" (original console prototype).
- `fd3d67a` (2026-08-09) — full rewrite as a wxWidgets GUI: category registry,
  multi-household support, bubbly visual language, several bug fixes.
- `32d5a21` (2026-08-22) — finished the bubbly redesign, refreshed README.
- 2026-10-07 commit — recurrence engine, direct assignment, Today tab, sidebar nav,
  themes, inline editing, bulk actions, filters, badges, starter chores + Docs-md.

## Current state as of 2026-10-07
- The recurrence/theme/sidebar/Today-tab feature push (previously uncommitted) was
  committed together with `CLAUDE.md` + `Docs-md/` and pushed to `origin/main`.
- `CLAUDE.md.pre-persistence.bak` intentionally left untracked.

## Standing process notes
- Never commit or push without the user's explicit go-ahead this session, even though the
  established repo pattern is "finish feature → refresh README → commit." Ask before
  doing either step, regardless of how confident the diff looks. (This is a general
  git-safety rule, not project-specific — noted here because it came up concretely this
  session with a large ready-to-commit diff.)
