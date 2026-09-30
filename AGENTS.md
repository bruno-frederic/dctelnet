# AI Agent Instructions

- Read `CONTRIBUTING.md` before making changes. It is the source of truth for project architecture, coding conventions, compatibility requirements, and build instructions.
- Keep changes focused and preserve existing user changes. Do not revert or reformat unrelated work.
- For code changes, run the official `make ci` build from `src/` before considering the work complete. Documentation-only changes do not require a build.
- Treat `make ci` as the authoritative compilation check. A successful build with another compiler or toolchain does not replace it.
- If a required check cannot be run, explain why and report it as unverified.
- Create local commits only for changes made for the current task, after required checks pass. Do not include pre-existing or unrelated changes. Do not amend existing commits or push unless explicitly asked.
- Follow any more-specific `AGENTS.md` instructions in directories being changed.