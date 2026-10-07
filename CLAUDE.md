# ChoreConsole

<!-- BEGIN MANAGED CLAUDE PERSISTENCE -->
## Durable project memory and resume rules

Project purpose: Family chore-management tool.

Persistent project state lives under Docs-md/. Claude auto-memory is redirected to Docs-md/Claude-Memory/ for this repository.

At the start of every session:
1. Read Docs-md/PROGRESS.md, Docs-md/NEXT_STEPS.md, Docs-md/DECISIONS.md, and the most recent entries in Docs-md/WORK_LOG.md.
2. Read Docs-md/Claude-Memory/MEMORY.md, then open any relevant topic memory files it references.
3. Inspect git status and recent git history before assuming what changed.
4. Briefly reconstruct where work stopped, then continue the highest-priority unfinished item. Do not redo completed work.

While working:
- After every meaningful implementation, debugging, research, testing, documentation, or design milestone, update Docs-md/PROGRESS.md and Docs-md/WORK_LOG.md.
- Keep Docs-md/NEXT_STEPS.md current and ordered by priority.
- Record durable architectural, product, workflow, or technical decisions in Docs-md/DECISIONS.md.
- Use auto-memory for information useful in future sessions. Prefer topic memories for architecture, coding/implementation, debugging, testing, research, workflows, project management, and user preferences/feedback.
- Keep MEMORY.md concise as an index; put detail in topic files.
- Before saying a task is complete or ending a session, persist the current state to Docs-md.
- Never store passwords, API keys, private keys, tokens, or other secrets in project memory/progress files.
- Do not force-push, delete branches, or publish/release anything unless explicitly instructed.

If conversation history is unavailable, the files under Docs-md are the source of truth for resuming work.
<!-- END MANAGED CLAUDE PERSISTENCE -->



