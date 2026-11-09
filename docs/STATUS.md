# Status — 2026-11-09

Branch: implementation/native-foundation. Initial repository had only Readme.md.
Overall implementation is incomplete. Milestone 1 has a working Ghidra/Unicorn
analysis toolchain, inventory and original-routine comparisons. Milestone 2 has
original asset loading, provisional room preparation and movement/replay code;
original gameplay comparison and native rendering validation remain pending.
Data ingestion, RNG and character stat calculations are now implemented and checked.
The integration session now connects combat, enemy turns, inventory, equipment,
health/death, progression and a new port save/load format. Scheduling/AI and
complete gameplay parity remain unverified.

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
- Monster scaling: 2,048 original full-function fixtures / 61,440 fields match.
- XP table: all 350 original thresholds in seven input configurations match.
- Six local CTest groups pass, including deterministic session and save/load tests.
- Actual macos-15 arm64 CI run 42581482510 passed all nine checks, including
  AppKit window input/drawing and real libc RNG agreement. Source is on the
  implementation branch; original files remain private and outside GitHub.
- Ghidra successfully imported/analyzed the game; reproducible targeted exports.
- Original Starting Room and hero SPR prepare successfully from supplied data.
- Embedded startup version 1.1.4 traced through the formatter's actual arguments.
- Reproducible replay NNEES.WW ends at x=4,y=6, turn=7 under provisional policy.

Blockers and limits:
- Original full-game reference execution is still unavailable. Native arm64
  build and synthetic window smoke now work through actual Mac CI. Testing
  original-asset rendering and complete gameplay remains pending.
- Complete gameplay systems remain to be implemented and compared.
- Additional version suffix/mod state and exact expansion scope remain unverified.
- GameFrame broad suite: 424 passed, 11 failed. Remaining optional/unrelated
  failures are recorded; repository checks and original comparisons pass.
- Combat uses a separate imported libc rand() stream; MT verification alone
  does not establish deterministic combat.

Current work: native scene renderer is being connected to the turn session and
original content definitions, with combat-window CI and portable app packaging.
DoRandomize and seed flow are identified; decision/arithmetic combat rules are
compared, while complete DoCombat/triggers and original HP/death side-effect
comparisons remain unfinished. Session HP/death behavior exists but is not yet
a full original transition comparison.
Obtain original room captures and native Mac build evidence when Mac access is
available. See RESUME.md.
