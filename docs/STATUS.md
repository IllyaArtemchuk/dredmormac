# Status — 2026-11-09

Branch: implementation/native-foundation. Initial repository had only Readme.md.
Overall implementation is incomplete. Milestone 1 has a working Ghidra/Unicorn
analysis toolchain, inventory and original-routine comparisons. Milestone 2 has
original asset loading, provisional room preparation and movement/replay code;
original gameplay comparison and native rendering validation remain pending.
Data ingestion, RNG and character stat calculations are now implemented and checked.

Verified:
- Drive archive downloaded, all ZIP CRCs valid; original retained outside git.
- i386 executable identified; named symbols and dependencies inventoried.
- C++17 core/replay/distance programs compile on Linux, CTest checks pass.
- Distance: 8,200 original executions, zero mismatches.
- RNG: 28,550 original outputs, including double bits, zero mismatches.
- Stats: 2,568 original fragment runs / 61,632 fields, zero mismatches.
- Character: 1,027 original full-function fixtures / 30,810 fields, zero
  mismatches; excludes abilities, equipment, buffs and polymorph.
- Native catalog: all 40 databases / 4,882 records agree with independent XML
  parser counts. Base only: 17 databases / 3,565 records. No merge policy claimed.
- Combat decision/arithmetic core: 4,101 original paths match fixed rand tapes.
- Inventory: 3,840 original operations match all 57 slots after each operation.
- Equipment eligibility: 684 original function executions match.
- Typed metadata/modifiers parse for all 751 supplied item definitions.
- Ghidra successfully imported/analyzed the game; reproducible targeted exports.
- Original Starting Room and hero SPR prepare successfully from supplied data.
- Embedded startup version 1.1.4 traced through the formatter's actual arguments.
- Reproducible replay NNEES.WW ends at x=4,y=6, turn=7 under provisional policy.

Blockers and limits:
- No actual Mac or original full-game execution available in this session.
- New AppKit target has not been compiled; arm64 build and smoke test pending.
  Manual macos-15 arm64 CI workflow and synthetic native-window test are prepared
  locally; neither has been run on a Mac. No original assets are required by CI.
- Complete gameplay systems remain to be implemented and compared.
- Additional version suffix/mod state and exact expansion scope remain unverified.
- GameFrame broad suite: 424 passed, 11 failed. Remaining optional/unrelated
  failures are recorded; repository checks and original comparisons pass.
- Combat uses a separate imported libc rand() stream; MT verification alone
  does not establish deterministic combat.

Current work: recover monster attribute/scaling rules and connect typed
definitions, equipment and verified combat to a deterministic turn session.
DoRandomize and seed flow are identified; decision/arithmetic combat rules are
compared, while complete DoCombat, triggers and HP delivery remain unfinished.
Obtain original room captures and native Mac build evidence when Mac access is
available. See RESUME.md.
