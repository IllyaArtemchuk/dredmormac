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
Save version 3 persists selected skills and ability fingerprint; load accepts v1/v2.
Native load now passes item and ability rules. Synthetic native fixture includes
a passive to exercise this path on the next CI push.

SkillSelectionController.* is added. Normal installation startup opens a skill
picker, requiring exactly seven and hiding deprecated skills. Smoke startup
uses the deterministic fixture profile, then separately drives the actual picker
checkboxes and validates count/order and Begin enablement. Passed run 42590387752.

Trap/toolkit type correction: constructors prove toolkit=11, trap=12; old enum
labels were reversed even though numeric inventory placement matched. Corrected
belt eligibility name to trap and re-ran 3,840 original operations successfully.
Save v3 loader computes legacy fingerprints and translates old IDs for v1/v2.
Synthetic pre-correction v2 fixture verifies migration; run 42591221995 passed.
Private item-construction.txt has item registration, Item/Food/Misc constructors
and CreateItem dispatch; toolkit-trap-types.txt has the type/stack constructors.
Continue starting item creation/auto-equipping; no complete grant path exists yet.

Basic instance factory make_inventory_item handles stackability and optional wand
charges. verify_wands.py matches 1,045 original charge loops/final RNG states.
GameContent appends built-in Lockpick/Spores definitions (external item count +2),
excluded from external-content fingerprint to preserve old saves/indices.
Private consumable-constructors.txt, item-visual-build.txt and trap-build.txt
recover constructors/graphics setup. Trap::Build sets stackable true. Wand charge
max is exclusive and one-wide intervals still draw. Casting/effects remain absent.
Next: complete base starting loadouts and auto-equipping. Supplied base random
loadouts use only mushroom/potion/wand/reagent/component pools; all weapon/armour
loadouts name fixed definitions. Misc loadouts each give one lockpick (repeated
entries produce four/eight), not an amount override. Weapon amount>1 creates that
stack then additionally grants one weapon; preserve observed +1 behavior.

CharacterBuilder/create_character now initialize filtered starting gear, basic
random consumables, builtins and clothing; end with rebuilt stats/full HP/MP.
Ten local groups pass. check_starting_content.py exercised 256 characters across
all 36 base skills without failure (integration only, not original reference).
Native normal installation scenes enable starting_loadout; developer scenes keep
empty loadouts by default. Native seed is elapsed application ms before picking
skills; developer scene seed and CLI --seed are explicit/reproducible.
Session::create_ground_item uses its own libc RNG atomically, allocates a free ID
across all containers, and retains charges. drop now rejects equipped identities
and definition mismatches. New synthetic character-root fixture tests complete
initial inventory/clothing paths and save/load. Pending native CI.
Next fidelity work: weapon/armour ability buffs and full starting-creation trace.
Private equipment-ability-buffs.txt exports Weapon::FixBuffs (0x13ba4e) and
Armour::FixBuffs (0x38340); their headless log is work/equipment-ability-buffs.log.

Native starting-loadout run 42594615983 passed nineteen checks + package smoke.
Equipped weapon training is added to abilities/equipment rebuilding. Original
verify_equipped_passives.py executes Weapon::EquipOnPlayer + its actual virtual
Player::RebuildAttributes, matching 1,024 / 63,488 fields, including 235 melee+
bow cases. ASCII string compare supplied; UpdateSpellsForEquipment skipped.
Bow modifiers no longer augment melee damage. Dual melee still disabled.
RepairWeaponSwitch.java restores two computed-jump overrides (plain references
alone did not suffice) and is integrated into analyze-original.sh. Export is
weapon-equip-repaired.txt privately. New source awaits native CI.
Room rules are corrected to read child flags; verify_templates.py independently
checks all fields over 346 templates. Base: 191 rooms, six shops, three zoos, all
roomset zero. The supplied expansion3 loadout amount=" 3" requires original
atoi-style prefix handling; legacy_number.hpp now supports room/loadout attrs.

Weapon-training run 42596610606 passed nineteen checks + packaging smoke.
Current work: full level-generation geometry. Private generation-flow.txt has
GenLevelContent1. FIFO door loop picks MT-bounded templates until five eligible
attempts; invalid candidates do not count. Scan candidate doors row-major, place
on first fit, append new door positions in WriteTemplate's x-major order. Failed
doors become wall; generation retries whole layout unless non-shop rooms >15
and floorCount>2999 (short game >10 / >799). Need preserve GenRoomName RNG too.
New private generation-names-and-size.txt exports GenRoomName (0xd3670) and
GenLevelContent (0x65c80); headless log work/generation-names-and-size.log.

Room-rule run 42597407998 passed nineteen checks + package smoke.
Generator prerequisites added: Random::bounded matches original inline range
selection in 1,080 fixtures / 9,720 outputs with no stubs. Room stamp coin refactor
still matches 1,024. names.cpp matches 1,028 full GenRoomName calls and both RNG
states. read_room_name_words matches 1,024 original text callback nodes, including
architecturePluralDB's singular-copy bug. Base pools are in GameContent; fixed
room names retained. Eleven local groups pass; native run pending.
Full generation must call random naming per written room (3 MT decisions plus
libc word picks) unless usefixedname is nonempty. Private generation-names-and-size
and name-word-loader exports establish this. Next: geometry/FIFO placement loop,
then original control-flow comparison before content population.

Geometry comparison is now implemented in dungeon.cpp / verify_dungeon.py, with
an explicit active template pool and optional resolved branch starters. 256
synthetic cases / 339 attempts and 64 base supplied cases / 68 attempts match
GenLevelContent1 (0x195bac) through BuildDoors entry (0x192d6e). Reports privately:
dungeon-geometry.json and dungeon-base-geometry.json. Includes full cells, graph,
room names, initial player and both RNG streams. Twelve local groups pass.
The original retry retains special-wall bit 0x80 even in empty/new floor cells.
Automatic starters at depth>0 need at least three doors; the current decompiler
output omitted the counter's significance, but disassembly 0x196300..0x196316
and real-template execution confirm it. Named branch starters/depth0 bypass it.
write_template's final integer is depth (Level+0x64), renamed from branch.
Resource guards are port policy and fail atomically. Template registration and
branch configuration remain inputs. Private room-registration-and-doors.txt has
EndElement 0x19bf20, LoadRoomTemplates 0x199ce0, BuildDoors 0x192d6e.
Next: door construction/plans, then content/scripts/population; full level native
play remains absent. New geometry native validation pending push.

Geometry native run 42601954130 passed all twenty-one checks and package smoke.
Door assembly planning is implemented as build_door_plan in dungeon.cpp.
verify_doors.py matches 1,024 original BuildDoors executions / 32,244 assemblies /
521 repairs with constructor/place/Open recording boundaries. Door entity and
sprite side effects are not included. Private door-behavior.txt and
door-interaction.txt export constructor, Build, Open, Close, ToggleDoor, Clicked,
RemoteActivation and GameObj constructor/save. Next: verified toggle/lock/obstruction
transitions, then room scripts/content. New door planning awaits native CI.

Door-plan native run 42602496973 passed. toggle_door/activate_remote_door are now
added in dungeon.cpp; verify_door_actions.py matches 1,408 full ToggleDoor and
RemoteActivation executions with controlled occupancy/UI boundaries. Includes
exhaustive384 + random1024. The raw toggle ignores locks, remote checks first;
an open center cannot close. End-cap closing checks only itself/immediate neighbor,
not far end, and obstructed requests return handled=true. Lockpick/kick and
animation/render state remain absent. New transition code awaits native CI.
Private room-content-flow.txt exports WriteDungeonContentFromRoomRecords0x193682,
RoomElementRecord CreateInLevel0x1a1d30/UpdateInLevel0x1a22e0 and RoomScript
condition/success loops. Next: content placement and scripts plus native integration.
