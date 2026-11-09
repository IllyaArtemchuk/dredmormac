# Status — 2026-11-09

Branch: implementation/native-foundation. Initial repository had only Readme.md.
Overall implementation is incomplete. Milestone 1 has an inventory and original
routine comparison through Unicorn; Ghidra setup remains blocked. Milestone 2
has original asset loading, provisional room preparation and movement/replay
code; original gameplay comparison and native rendering validation remain pending.

Verified:
- Drive archive downloaded, all ZIP CRCs valid; original retained outside git.
- i386 executable identified; named symbols and dependencies inventoried.
- C++17 core/replay/distance programs compile on Linux, CTest checks pass.
- Distance: 8,200 original executions, zero mismatches.
- Original Starting Room and hero SPR prepare successfully from supplied data.
- Embedded startup version 1.1.4 traced through the formatter's actual arguments.
- Reproducible replay NNEES.WW ends at x=4,y=6, turn=7 under provisional policy.

Blockers and limits:
- Full JDK needed for Ghidra; host only has JRE. Download attempts failed.
  Unicorn/Capstone analysis works, so this does not block isolated routine work.
- No actual Mac or original full-game execution available in this session.
- New AppKit target has not been compiled; arm64 build and smoke test pending.
- Complete gameplay systems remain to be implemented and compared.
- Additional version suffix/mod state and exact expansion scope remain unverified.

Exact next action: isolate `init_genrand` at 0x190800 and `genrand_int32` at
0x190870, map their state, capture seeded output in Unicorn, and compare a
portable replacement. Obtain an original Starting Room capture in a legacy
macOS environment to compare spawn, collision, door turns and tiles when access
is available. See RESUME.md.
