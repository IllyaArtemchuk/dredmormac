# Resume

Read AGENTS.md, STATUS.md, FINDINGS.md and DEVIATIONS.md. This is not a complete
game. Do not overwrite source with the GameFrame scaffold.

Current cloud paths (private material, never commit):
- Repository: /workspace/dredmormac
- Original archive: /workspace/work/Archive.zip
- GameFrame: /workspace/work/tools/game-frame
- GameFrame home: /workspace/work/gameframe-home
- Scaffold project: /workspace/work/dredmor-analysis (tooling only)
- Original app: /workspace/work/gameframe-home/games/dredmor/game/Dungeons of Dredmor.app
- Analysis and derived: sibling analysis/ and derived/ in games/dredmor/.
- Tool environment: /workspace/work/tools/game-frame/.venv
- Ghidra: /workspace/work/tools/ghidra_12.1.4_PUBLIC
- Working Java compiler home: /workspace/work/tools/java-21-reproduced
- Logs: /workspace/work/gameframe-setup.log, gameframe-tests.log, dredmor-ghidra.log.

Use `PATH=/workspace/work/tools/game-frame/.venv/bin:$PATH scripts/build.sh`.
Run original comparison with that environment's Python, following README.
Do not execute the entire original binary as a Linux program.

The repository now also has its own .venv. Use
`PATH="$PWD/.venv/bin:$PATH" scripts/build.sh`, then
`scripts/verify-original.sh DATA_ROOT PRIVATE_ANALYSIS_DIR` to run distance,
RNG, stat-fragment, full-function character fixtures and database count checks.
The C++ build additionally requires Expat development headers/library.

Ghidra is working. The installed Java runtime contains jdk.compiler; only its
javac launcher was absent. Reproduce the local compiler home with
`python3 scripts/java-compiler-home.py /usr/lib/jvm/java-21-openjdk-amd64 NEW_LOCAL_HOME`.
Set JAVA_HOME, GHIDRA_INSTALL_DIR and GAMEFRAME_ROOT, then run
`scripts/analyze-original.sh ORIGINAL_BINARY PRIVATE_ANALYSIS_DIR`.
This uses the saved project named dredmor; GameFrame's ghidra-run.sh assumes a
project named re, so do not point it at this project unchanged.

Reports: random.json, stats.json, character.json, catalog-base.json,
catalog-all.json and recovered-functions.txt in the private analysis directory.
The final file is decompiler output and must remain outside tracked source.

Fresh environment: clone repository, run scripts/setup.sh, obtain the user's
archive privately, verify recorded SHA256, inspect ZIP names/types, and extract
regular app files outside git. Preserve the original ZIP for legacy Mac use;
the analysis extraction deliberately skips framework links and executable modes.
Use scripts/inventory.py to reproduce inventory and scripts/prepare_scene.py for
the scene. Do not put private transfer URLs in source, logs to share, or commits.

Mac handoff: install Python 3.11+, CMake and Xcode command line tools; use the
same original data root, prepare scene locally so absolute asset paths are valid,
run scripts/build-macos.sh, then launch the app binary with scene.json. Record
actual build/launch evidence; fix compile errors before declaring Mac support.
Modern macOS cannot run the original i386 game, so native port testing and
original gameplay capture may require different machines.

`.github/workflows/native-macos.yml` runs on implementation-branch pushes and
workflow_dispatch. Actual run 42585590201 passed all fourteen checks on standard
macos-15 arm64, including native combat and relocated/ad-hoc-signed package smoke.
The development app artifact contains no original assets. Direct animated SPR
loading now has local decoder tests and 698-file/3,633-frame palette comparisons;
its native synthetic animation smoke passes. Save/Load menu controls are added;
their native atomic roundtrip smoke passes. Room-placement kernel matches
69,632 original executions; generation-flow.txt is a private export of
WriteTemplate, GenLevelContent1, Branch::GetTileID and branch XML callback. Original asset
visual comparison and full original gameplay parity remain pending.

Continue from the ordered tasks. Source has no selected distribution license;
do not import GPL code or publish releases without resolving project licensing.

Room-stamping loop comparison: scripts/verify_stamp.py, 1,024 matches. Private
room-constructor.txt confirms arrays initially zero; digit script positions stay
zero until later population. Numeric RoomDefinition::numeric_template is distinct
from provisional instantiate collision. scripts/verify_templates.py compares all
346 templates independently. Full generation remains pending.

Native startup selection is added: no arguments opens a file/folder picker;
argument can be original .app, data directory, or prepared scene JSON. A new
synthetic-content installation fixture exercises direct loading, movement and
save/load. Its actual native CI execution passed in run 42586734212 (16 checks).

Native inventory controller is separate in platform/macos/InventoryController.*.
RoomView supplies bag/belt/equipment rows and core transfer callbacks. G picks up;
I opens inventory, with slot selection and Equip/Unequip controls. Explicit scene
ground_items are a developer fixture facility, not original room population.
New native-inventory smoke picks up a synthetic cap, drives table selection and
both transfer buttons, checks restored stats, then runs save/load. All eighteen checks passed in run 42587414308.

Starting items: selection.cpp preserves original edible/booze sampling and rare
fallback; verify_selection.py matches 2,048 original calls with a supplied rand
boundary and null current level. verify_loadout.py matches 4,096 class/always
loadout gates. Typed skill.loadouts and starting_loadout return plans only.
Private starting-skills.txt has full BuildPlayerFromSkills/RebuildAttributes;
loadout-rules.txt has skill callback, GiveStartingAbilities, GetRandomEdible/Booze,
GiveAndEquipWeapon/Armour. Full starting grants and ability aggregation pending.

Abilities module and session profile added. verify_passives.py compares full
original Player::RebuildAttributes with passive abilities: 1,024/63,488 matches,
only ASCII string-compare import supplied, no equipment/buffs/polymorph. Private
ability-gates.txt and skill-finalize.txt hold constructors and callback details.
Session::character_profile sets selected skills/rules on fresh sessions, fills
resources, and equipment rebuild recalculates base unarmed/shield eligibility.
Weapon-family/archery/thrown ability rebuilding is still separate and missing.
Save version 2 persists selected skills and ability fingerprint; load accepts v1.
Native load now passes item and ability rules. Synthetic native fixture includes
a passive to exercise this path on the next CI push.

SkillSelectionController.* is added. Normal installation startup opens a skill
picker, requiring exactly seven and hiding deprecated skills. Smoke startup
uses the deterministic fixture profile, then separately drives the actual picker
checkboxes and validates count/order and Begin enablement. Pending next Mac run.
