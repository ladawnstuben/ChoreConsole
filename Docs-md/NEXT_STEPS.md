# Next Steps

All known implementation work is committed and pushed (2026-10-07). Candidate next items,
none started — confirm with the user which (if any) to pursue:

1. A UI affordance to change a chore's recurrence from the inline detail panel on the
   Chores tab (recurrence editing is still only in the "Modify Selected" full dialog).
2. Expose `getDoerLifetimeCompletions`-style badges/history elsewhere (e.g. History tab).
3. Re-confirm `ChoreConsole.vcxproj.user` / `.vs/` / `x64/` build output is `.gitignore`d
   (they don't show in `git status`, so likely fine).
4. Decide whether to delete the untracked `CLAUDE.md.pre-persistence.bak` (left out of
   the commit deliberately; only delete with the user's OK).
