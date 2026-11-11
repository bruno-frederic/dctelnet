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

- [x] MA0-1 Test: with APP_RENDERER_CONSOLE_DEVICE or IBMCON_DEVICE set, ConWrite never
      calls VtWrite and still reaches CMD_WRITE. Sentinel on both. (Host test with shim,
      or rig run on each renderer.)
      DONE 2026-10-30 rig 3, same connect with the art: console.device dbgVtWriteCount 0,
      dbgConWriteCount 10; ibmcon.device dbgVtWriteCount 0, dbgConWriteCount 14.
      Evidence: thoughts/spot/ma1-10-rig.md.      PARTIAL 38d5f49: Renderer_Route (src/renderer.c) is the one decision ConWrite
      dispatches on; tests/test_renderer.c proves console and ibmcon never route to the
      builtin engine. Open: the sentinel counts (VtWrite 0, CMD_WRITE 1+) through ConWrite
      itself, by rig run on each renderer.
- [x] MA0-2 (2026-10-30, after MA1-MA4 merged at 3e8308c: test_renderer test_the_cycle_visits_all_four_in_bit_order passes in the container run) Cycle test: the Renderer cycle (guis.c:1180-1184) still visits all four
      renderers in bit order after the work in MA1-MA3.
      Test exists (test_renderer "cycle visits all four", plus RendererLabelsMatch in
      guis.c). Rerun after MA1-MA3, then tick.
- [x] MA0-3 Pin: the vtcon submodule commit used by the finished work is committed in
      the superproject; `git submodule status` shows it with no `+` or `-`.
      DONE 2026-10-30: vtcon 3247ba91, no `+`/`-`. Push pending owner OK: 3247ba91 is on
      no UP-Term remote branch yet (planned branch dctelnet/vtcon-pin).
- [x] MA0-4 Pin retro32-term (branch heads/feat/layer-aware, not a release): record its
      commit in the same commit as MA0-3.
      DONE 2026-10-30: retro32-term 0a02a3a (MA2-9 pen map, one commit ahead of
      origin/feat/layer-aware 2aec2db; owner chose to fast-forward feat/layer-aware at the
      #56 push). .gitmodules branch = feat/layer-aware; submodule origin is
      git@github.com:spotUP/retro32-term.git.
- [x] MA0-5 LICENSE restored to the original upstream text (owner decision, todos.md:89).
      Ask the owner for the source text if the first commit before 33147c6 does not hold it.
      DONE: LICENSE at HEAD is byte-identical to 33147c6 ("Add License", Bruno FREDERIC);
      the no-licence commit fafed36 is on no branch.
- [x] MA0-6 repos.lock row for dctelnet-vtcon updated to branch feature/pr-vtcon-engine
      and the new HEAD, via the lock tool. Needs MA0-7 first.
      DONE upterm 6914a94 (pushed): row reads feature/pr-vtcon-engine 38d5f49, equal to
      git rev-parse HEAD. The commit is not on GitHub yet (no push without the owner), so a
      fresh bootstrap cannot fetch it until the owner pushes.
- [x] MA0-7 upterm-bootstrap update_lock uses os.path.exists so .git files count
      (todos.md:95). Test: update_lock on a tree with a .git file changes the row.
      DONE upterm bc99e23 (pushed): is_checkout() used by sync_one and update_lock;
      tests/test_bootstrap.sh case 5 fails without the fix, passes with it.

## Done

Checklist at zero open. MA0-1 shows the sentinel count 0 for VtWrite and 1 or more for the
device write. MA0-6 verified by reading the row back from the lock file and comparing with
`git rev-parse HEAD` of dctelnet-vtcon. No push without the owner (upstream rules).
