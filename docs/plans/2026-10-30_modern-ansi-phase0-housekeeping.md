# MA0 Housekeeping: fallback renderers, submodule pin, licence, repos.lock

Parent: 2026-10-30_modern-ansi-engine.md. Tree: ~/Code/DCTelnet/dctelnet-vtcon.

## Goal

The repo is in a state where the later phases can land cleanly and nothing is lost.

## Files to touch

- src/DCTelnet.c:417-465 (ConWrite branches): do not remove. Add a test that the other
  renderers still get their bytes.
- src/prefs.h:146-149, 197-201 (APP_RENDERER_* bits, in bit order, guarded by the
  compile-time check at guis.c:1184).
- src/third_party/vtcon: submodule, pinned 232065f. .gitmodules lines for vtcon point at
  https://github.com/spotUP/UP-Term.git.
- LICENSE (43 lines; commit 33147c6 "Add License"). Owner chose to restore the original
  upstream text. Row: /Users/spot/Code/UP-Term/vtcon/thoughts/spot/todos.md line 89.
- /Users/spot/Code/UP-Term/upterm/repos.lock line 19 (dctelnet-vtcon row).
- /Users/spot/Code/UP-Term/upterm/bin/upterm-bootstrap, update_lock: tests isdir(.git) but
  dctelnet-vtcon has a .git FILE. todos.md line 95. Fix is os.path.exists.

## Checklist

- [ ] MA0-1 Test: with APP_RENDERER_CONSOLE_DEVICE or IBMCON_DEVICE set, ConWrite never
      calls VtWrite and still reaches CMD_WRITE. Sentinel on both. (Host test with shim,
      or rig run on each renderer.)
- [ ] MA0-2 Cycle test: the Renderer cycle (guis.c:1180-1184) still visits all four
      renderers in bit order after the work in MA1-MA3.
- [ ] MA0-3 Pin: the vtcon submodule commit used by the finished work is committed in
      the superproject; `git submodule status` shows it with no `+` or `-`.
- [ ] MA0-4 Pin retro32-term (branch heads/feat/layer-aware, not a release): record its
      commit in the same commit as MA0-3.
- [ ] MA0-5 LICENSE restored to the original upstream text (owner decision, todos.md:89).
      Ask the owner for the source text if the first commit before 33147c6 does not hold it.
- [ ] MA0-6 repos.lock row for dctelnet-vtcon updated to branch feature/pr-vtcon-engine
      and the new HEAD, via the lock tool. Needs MA0-7 first.
- [ ] MA0-7 upterm-bootstrap update_lock uses os.path.exists so .git files count
      (todos.md:95). Test: update_lock on a tree with a .git file changes the row.

## Done

Checklist at zero open. MA0-1 shows the sentinel count 0 for VtWrite and 1 or more for the
device write. MA0-6 verified by reading the row back from the lock file and comparing with
`git rev-parse HEAD` of dctelnet-vtcon. No push without the owner (upstream rules).
