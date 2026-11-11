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
zero until PointProperty resolves a reference during XML loading (earlier
notes incorrectly deferred this to population). Numeric RoomDefinition::numeric_template is distinct
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

punch_out_dungeon_content now reproduces original0x19263c with 2,048 fixtures /
404,862 cells and no imports; report content-punch-out.json privately. It clears
8 neighbors around each remaining door (observing earlier mutations), then void
above regular floor becomes wall. Floor count, IDs and flags are retained.
Twelve local groups pass. New punch-out native validation pending.
Room-record-finalize.txt privately exports PunchOutDungeonContent, base object
FinalizeInLevel, RoomElementRecord ctor and XML property registrations. Common
percent defaults100, can_push defaults1, id isroomlocal; Finalize places entity,
assigns roomID string and can_push. Point/factory export currently running to
room-point-and-factory.txt. Next: resolve at/x/y, appearance gate and element types.

Door-transition run 42602928512 and punch-out run 42603410256 passed native CI.
room_records.cpp adds take_room_point; verify_room_points.py matches 1,024 full
PointProperty::SetFrom0x1ace5e executions with controlled tiny attribute maps,
string/ASCII/erase boundaries. At consumes attribute, resolves marker and sets
template kind1 immediately; explicitx/y consumes keys without changing geometry.
Marker table remains available; duplicate row markers take last coordinate.
Source numeric_template now applies common object/script point refs. Earlier
notes wrongly kept all digits zero until population. Corrected all346 template
checks pass; basegeometry64cases/67attempts passes after rerun. Thirteen local
groups pass; native correction validation pending push.
Next: common appearance gate and typed room object/element creation plans;
finalize places entity, assigns room-local id and can_push. Scripts still raw.

Point-ingestion run 42604426802 passed twenty-two native checks/package smoke.
RoomDefinition::elements exposes typed RoomElementDefinition. plan_room_element
matches originalCreateInLevel0x1a1d30 in2,573 fixtures, with constructors/finalizer
recorded and their RNG/effects excluded. verify_room_elements.py supports
--content DATA_ROOT; all175 definitions/346 rooms independently agree. Thirteen
local groups pass; native plan validation pending push. Gate: percent>=100 draws
nothing; otherwise accept bucket<=percent, so0 has1% and99 always accepts/draws.
Dispatch is case sensitive; unknown type still accepts but finalizes null. Vending
craft4/food1/bolt0/drink2/thrown3, portal0. Bookshelf contents override only when
nonempty; DiggleGod receives name. No actual entities are constructed by plans.
Next: other room records/constructor side effects and original room population.

Element run42605231486 passed nativeCI/package. Custom definitions/entities are in
room_records.* and RoomDefinition::custom_objects. verify_custom_objects.py runs
actual record/Create, GameObj/Blocker/Engraving constructors, Build and both
finalizers; 2,292 match with sprite/place/id/list and library boundaries. Report
custom-objects.json privately; --content checks all992 definitions/346 rooms.
Movement/vision overrides, push byte, labels/id, sprite calls, frame/animation
flags, registration and appearance/RNG match. No extra constructor RNG observed
outside supplied boundaries. Not yet owned/rendered by Session/native level.
verify_room_sprites.py matches1,024 original SpritePropertySetFrom calls using the
Points helper's synthetic BST chains (library erase supplied). Single png
conflicts with series/Animate; fullseries consumes Animate but ignores its value
because the SpriteRecord overload has null bool destination. Both custom Build
functions set animation flags true regardless. Thirteen local groups pass.
Private custom-room-records.txt exports otherCreate routines and common metadata;
custom-sprites.txt/custom-constructors.txt/custom-build-and-obstruction.txt hold
sprite parsing, constructors, Build and SquareContainsObstruction. New custom
source awaits nativeCI. Next: owning level/session/render integration and other
record classes/population. Keep resource paths/data/decompiler output private.

Custom run42607026309 passed nativeCI. Session now owns custom_objects; public
create_custom_object uses libc RNG atomically, passable combines world + objects.
Movement/enemy spawning/BFS use it. Save formatv4 persists all modeled custom
fields; accepts v1..3 and applies swapped-item migration only to v1/v2. Thirteen
local groups pass; save tests check exact object roundtrip/future draws/malformed
fields/v3 migration. Original custom oracle now checks MT unchanged too (2,292
matches). 256 starting-content initializations pass, report hash40c8db67...
Native SpriteImage supports typed SPR/PNGseries; RoomView creates object images,
draws them and prepares new caches atomically on load. Direct installation scene
enables room_objects; prepared JSON may name the source room. Header bool
instantiate(false) leaves custom collision dynamic but retains old preview spawn.
Fixture content room marker b was corrected to1 and given synthetic sprite path;
make-smoke-scene generates its assets. New --objects fixture has a blocking SPR
and passable PNGseries; CMake adds native-objects fixture/window checks. Native
source awaits actual Mac CI. Verify it and fix compile/runtime failures first.
CLI dredmor-session --room-objects exercises original starting preview metadata;
all14 referenced starting-room resources exist. Full generator consumption and
intervening other constructors remain absent, so whole-game content parity is
not claimed. Next: finish native verification, then other records/interactions,
full-level session/render integration. No stopping at CI checkpoints.

Run42608683842 (01a3e5e) passed24 native checks/package, including new object windows.
RoomMonsterRecord/RoomTrapRecord/RoomLootRecord/RoomHordeRecord definitions and
RoomDefinition::population are added; normalized plan_room_population retains
common gate/position and factory request arguments. verify_population_records.py
matches2,132 original calls with lookup/constructor/spawn/item-factory/finalization
boundaries; --content verifies830 records/346 rooms (165mon,102trap,514loot,49horde).
Original trap name search executes over synthetic pools, but port plan leaves
database identities unresolved. Actual constructors/spawn effects and extra RNG
are not implemented by plans. Thirteen local groups pass. New source awaits Mac CI.
Private population-record-properties.txt confirms loot subtype=random/amount1/
artifact0, horde min/additional=-1. custom-room-records.txt holds all four Create
functions. Next: actual factories and ordered full room content execution, then
full-level integration; do not present request plans as populated/playable levels.

Population run42610022836 passed MacCI. NormalRandom now implements original
box_muller0x85d70; verify_normal.py matches8,384 result/cache states and11,790 mixed
output lines, with host double-log import boundary. MT/sqrt/float instructions
execute original. Cache stores unscaled float spare; MT seed does not reset it.
GameRandom owns normal; savev5 persists validity/spare bits, migratesv1..4 with
empty cache, and retains v4custom objects. Fourteen local groups and original
28,550 random outputs pass. New normal/save source awaits MacCI.
Private box-muller.txt and loot-factory.txt export sampler/GetRandomTrap/full
CreateItemFromTypeAndSubtypeStrings/SpawnMonster/SpawnHorde. Next: random trap
selection and target-depth wrapper. Original GetRandomTrap always samples normal
at mean float(depth)+1 before alternate-depth branching; alternate<0 overwrites
target with libc bucket0..19; alternate>=0 samples again at that mean without+1.
Clamp1..20, discard one unconditional pool pick, then up to101 picks: accept
!special && level==target or unconditional attempt101. Debug is not checked.
Then actual loot/factory/population integration; full level remains incomplete.

Normal/save run42611259389 passed25 native checks/package. select_trap now matches
1,024 full GetRandomTrap calls (529 fallback101), with actual normal/MT and tweak/
log/rand boundaries. Compares selected index/target/attempts/libc/cache/nextMT8.
Session::select_trap_candidate owns this state; save tests compare20 future calls
across reload. Fourteen local groups pass. New trap source awaits native CI.
ItemDefinition.trap adds measured trigger/mount/facing/charge/spell/origin/target
metadata. Critical fix: <trap level> overrides item-root level (shared field238).
verify_trap_definitions.py matches actual entryCtor55a4c + trapXMLcallback4f7d0 in
1,024 fixtures/1,366 nodes and independently checks53 supplied traps. Callback
userdata must point to a counter, not null (tail increments it). Private
item-parser.txt/item-entry-defaults.txt give complete parser and entry defaults.
Enum trigger once0/charges1/other2; mount floor0/wall1/other2; facing south0/west1/
north2/east3, unknown retains previous; missing origin retains previous, other
trap fields reset from each child. Full trigger/casting/disarm remains pending.
Next: actual loot/item factory and trap activation, ordered population, full-level
session/native integration. loot-factory.txt has factory18ac20 and spawns. Factory
type literals: artifact,zorkmids,misc,reagent,component,potion,wand,weapon,bolts,
armour,trap; unknown random types return null. Non-random subtype bypasses type
except artifact/zorkmids/misc and calls CreateItem(name). Misc lockpick/lockpicks
map0, spores1. Quantity>1 overrides count and enables stacking; <=1 keeps ctor
defaults. Artifact path's decompiler has a suspicious weapon loop; inspect actual
instructions before implementing it. Keep all supplied data/outputs private.

Trap run42612861333 failed AppleClang compilation: tests assign streams={} and
the implicit aggregate member initialization cannot select DarwinRandom's explicit
defaulted constructor. GameRandom.libc now has an explicit default member
initializer{1}, preserving seed1 and allowing portable aggregate reset. Re-run Mac
CI before treating trap integration validated. Currency core work is underway;
see the current working tree and its verify_currency.py report before continuing.

Currency core is now in currency.hpp/cpp, with typed AbilityDefinition.currency_bonus
and ItemKind::currency10. verify_currency.py matches5,166 actual ctor/factory/
wallet cases with sprite/UI/config/library boundaries and nextMT8 unchanged.
15local groups pass. No Session wallet or Currency InventoryItem payload exists
yet; that is the next integration. Value lives at originalobject1c8, separate
from quantity9c; factory count override must not multiply money. Base Currency
quantity1/stackfalse; icon thresholds10/50/250/1000/4000 choosecash0..5. Bonuses
are positive eligible floats applied sequentially with truncation after each.
Type zorkmids roll and named zorkmids have different distributions (currency.cpp).
Give/Take wrap int32 and allowdebt; INT_MIN input has a port guard for original
infinite delegation. Private currency-and-named-items.txt exportsCtor, Build,
Give/Take, SteppedOn, PutInInventory and CreateItem(name); loadout-rules.txt confirms
zorkmidbuff percent/percentage -> float130. Original named lookup is case sensitive
and searches all four item pools in order, unlike current ambiguity-rejecting
preview lookup. Native currency validation pending push. Next: Session balance,
separate currency_value item payload, builtin appended afterLockpick/Spores,
autopickup/G, savev6 and native wallet/smoke. New typed currency bonus must enter
ability-content fingerprint with legacy v1..5 hash migration when Session uses it.

Currency core run42615446594 passed actual Mac CI/package, resolving the earlier
Clang initializer failure in the descendant. Session now owns player.zorkmids and
InventoryItem.currency_value; create_ground_currency uses builtin subtype2,
appended afterLockpick/Spores without shifting existing indices/fingerprints.
Movement/G pickup bypasses bag capacity, credits the separate value only, and is
atomic on transfer failure. Negative balances/wrapping persist. Savev6 includes
wallets, payloads and currency_bonus float bits in ability hash; loaders1..5 use
legacy hash. Synthetic tests/fixtures/port-v5-rng-objects.xml was generated using
the df8cac9 serializer and synthetic skills/object/cache. Fifteen local groups
pass; native wallet and17-zorkmid inventory smoke await new CI. Next: actual
item/loot factories, activation and ordered population/full-level integration.

Wallet run42617482250 passed26 native checks/package (job129160872552). Simple
factory is in factory.hpp/cpp and Session::create_ground_loot. Explicit tables:
four ordered named/potion/wand/bolts/trap pools and global reagent pool. Named
lookup returns the first exact match, including unsupported-first NULL; subtype
not random bypasses type except artifact/zorkmids/misc. Both currency roll paths
retain separate amount/count. Random trap delegates measured selector. Random
weapon/armour/artifact and named artifact generation throw before mutation.
verify_factory.py matches2,100 original factory executions, actual trap sampler
with log/tweak boundary, currency constructors/Build with sprite boundary, other
constructors recording; compares selected definition/kind, count override, money,
libc/cache/nextMT8. Synthetic case tables only, not real registration. Sixteen local
groups pass. Latest factory source awaits MacCI. Next: original registration
metadata and tables, other selectors/artifacts, actual room population ordering.

Simple-factory run42618377334 passed27 native checks/package. register_factory_item
now implements end-callback544f0 tables, including non-special type6 bolt filtering,
all weapons and other kinds, global alchemical entries even when mask0, and no
append for converted records. verify_item_tables.py matches1,024 fixtures /
32,759 entries with reserved vectors, conversion-stack and empty parent-stack
inputs; complete original callback executes, empty weapon spell string. With
--content independently verifies565 base items/66 reagents. GameContent base tables
register mask7, confirmed startup immediate0xb7c82 beforecall0xb7c8a; ESIbase0xb69c1
plus16798c gives game//itemDB.xml at21e34d. Other startup masks5/1/7 correspond to
expansion/expansion2/expansion3 but conversion mutation is still absent, so expanded
catalog table getter throws instead of guessing. Native ground_items now use named
factory/tables; inventory fixture exercises this next CI push. Sixteen local groups
pass. Next factory gaps: random weapon/armour selectors, artifact creation; then
ordered room content/population and full-level native integration.

Registration/native run42619110918 passed27 Mac checks/package. Gear selectors in
selection.cpp now match2,048 full original GetRandomWeapon4e270/GetRandomArmour4eb80
calls: normal+MT actual, tweak/log/libc boundaries, chosen entry, artifact-spawned
flags, libc/cache/nextMT8. Weapon alternate>=0 consumes second normal but keeps the
original target; armour uses it. Initial means are depth, not+1. Offset wraps, clamp
weapon0..20/armour0..18; actual bucket is target+1. Weapon lowers target after100
rejects, resetting counter0 (next lower bucket can get101 draws); armour does not
lower nonempty all-invalid pools. Weapon excludes craft/debug/special/type5/6 and
artifact unless allowed+notspawned, then marks spawned; armour ignores artifacts.
Port guards exhausted buckets and draw budgets atomically. Factory only connects
non-artifact gear; artifact generation remains guarded, so session unique-artifact
state has not been added yet.
build_gear_buckets mirrors splitUpItemLists4f400: levels0..19, named pool order,
matchingkind and !special; higher two port buckets empty for safe descending.
verify_gear_buckets.py matches256 fixtures /8,228 entries + supplied base ingestion.
verify_factory.py expanded actual gear selectors still matches2,100 cases. Seventeen
local groups pass; gear source awaits MacCI. Private weapon-armour-selection.txt
and item-level-buckets.txt are exports; keep them private. Next: artifact creation
and special factory artifact branch (decompiler weapon loop needs disassembly),
then room content/population ordering and full-level session/native integration.

Gear run42620199153 passed28 Mac checks/package (job129170050695). Artifact core
in artifact.hpp/cpp is now implemented; verify_artifact.py matches1,024 full
GenArtifactName3c290/CreateArtifact3c540 executions with actual MT/naming/rolls,
Buff zero-modifier ctor and item virtual name/description boundaries. Named items
still consume generated-name RNG before replacing name, then use definitionquality
(overrides a deliberately different passed quality in oracle). Description pools
and per-category rolls compare, including free primary additions at categories
76..81 and duplicate secondary ranges with index15 skipped. Damage writes both
buff and weapon arrays only for weapons. Normal cache untouched. Eighteen local
groups pass. Artifact factory/instance/equipment/save integration still absent.
ArtifactWords loads9 tables from base text; verify_artifact_words.py matches2,048
original callbacks, all900 supplied word entries independently, including empty/
missingtext/plural and case variants. GameContent.artifact_words accessor added.
Next: optional Artifact instance payload, original factory quality/RNG behavior,
unique state, equipment aggregation and save migration; full room population still
pending. Private artifact-creation.txt/artifact-name-and-buff.txt exports. Source
awaits next MacCI. Do not stop at CI milestones.

Artifact core run42621347955 passed Mac validation/package. InventoryItem now has
optional Artifact; Session::create_ground_artifact(def,quality,words,pos) constructs
basic item then artifact atomically. artifact_request reads rootartifact and child
quality for fixed-name overrides. Ground/bag/equipment ownership/pickup retain data.
Factory still rejects artifacts; explicit artifact construction is the current API.
Equipment applies artifact primary/secondary/resist; armour generic damage, weapon
extra damage once in Session attack stats (weapon Buff damage excluded). HP/MP
flat changes apply on equip, subtract on removal with original HP>=1 removal clamp.
verify_artifact_equip.py matches1,024 full equip/FixBuffs/Player+HittableApplyBuff/
Rebuild paths /65,536 stats+resources, string/allocation boundaries, spellupdate
skipped; no passive abilities/dual wield/replacement/unequip execution comparison.
Private artifact-equip-buff.txt/hittable-apply-buff.txt/artifact-resource-removal.txt.
Savev7 nested artifact text/modifier/weapon arrays; v1..6 load. New synthetic
port-v6-wallet.xml was emitted by actual7aa0860 v6 serializer using legacy_profile
plus125 wallet/23groundcash; verifies objects/cache/wallet/ability-hash migration.
Native --inventory --artifact fixture generates synthetic text words and q10 cap,
checks displayed name after pickup/equip/unequip then atomic save/load. Eighteen
local groups pass; new native artifact source awaits CI.
Next: complete artifact factory and unique state. Private artifact-factory-
disassembly.txt confirms original artifact-type weapon branch constructs/discards
one weapon then selects/constructs another. Type5 infinite loop is real but normal
GetRandomWeapon excludes5/6. Depth quality multiplier231888=double0.25; quality
range is floor(depth/4), +1 afterlibc pick, or1 if nonpositive. Explicit weapon/
armour artifact=true rolls1..3 (rand multiply6>>32), not1..6. Named subtype ignores
passed artifact flag; named definitionartifact calls depth-based quality draw then
CreateArtifact overrides todefinition quality, preserving that discarded draw.
Artifact type uses Level argument depth; named uses curLevel. All this factory
integration still pending; preserve raw original inputs/outputs outside tracked git.

Artifact factory integration is now implemented in make_loot_item. Context has
artifact_words pointer and optional level_depth (Level argument depth vs curLevel
SpawnDepthContext). ItemFactoryState.spawned_artifacts is copied/committed with
RNG; Session owns it. Gear selection flags apply only to random weapon artifacts;
named construction does not mark them. Generic typeartifact preserves discarded
first weapon and depth-quality draw. Explicit weapon/armour flag quality1..3;
named subtype ignores passed flag, but named definitionartifact rolls discarded
depth quality then overrides inside CreateArtifact. Random armour flagfalse may
select a definitionartifact and create a plain base item, without generation.
Reagent/component entry-based construction likewise does not auto-create artifacts.
verify_artifact_factory.py matches1,024 full original outer/named factory calls /
134 discarded weapons, actual selectors/CreateArtifact, gear/Buff constructor and
virtual text boundaries plus floor/log/tweak/libs. Compares definition/kind/count,
payload/quality/allmodifiers/weapon damage, unique flags, libc/cache/nextMT8.
Existing2,100 simple cases and18 local groups pass. Session tests compare24 future
artifact creations after reload, failed placement rollback and duplicate/invalid
unique-state saves. Savev7 optional spawned-artifacts container supports earlier
v7 empty state. Native artifact fixture now marks synthetic cap namedartifact and
uses real factory (no explicit quality bypass). Ownership run42623376554 queued
for runner (13:12UTC); factory changes still await push/CI. Next: ordered room
content execution/population and complete constructor effects/interactions.

Artifact run42624802836 passed31 native checks/package, including new artifact
window; job129185710082. Ownership42623376554 was cancelled whilequeued, not a
source failure. Latest successful descendant includes full artifact factory.
RoomDefinition.records now returns recognized object records in XML order, scripts
separate. BreakableRoomDefinition/RoomPedestalRecord typed, lever/clock/magicblocker
are retained UnimplementedRoomRecord (none in supplied rooms). All2,123 records /
346 rooms independently match order/common metadata;116 breakable and10 pedestal
metadata compare. numeric_template now recognizes point properties for all12 tags.
BreakableRoomObject has base appearance, intact/breaking/broken sprites/counts,
hits, single-breaking-delay override10 whenoneframe. Constructor defaults stopped,
initial broken selects lastbrokenframe but retains movement block until hit causes
break (unlesspassableoverride). CustomRecord virtual finalizer applies passable/
vis overrides even tobreakables; vtable2653a8+10=1a0290 confirms. Hits addone, hit
thenbroken events, nonbrokenframe=min(hits,currentframes-1), breaking animation
request then static lastbrokenframe/collision0. Alreadybrokenreturns0 unchanged.
verify_breakable.py matches1,024 original fullrecord/Ctor/Build/finalizer/hit cases /
4,580 hits, SpriteRecord lookup/place/id/script-events/PlayOnBob boundaries; MT
unchanged. Includes arbitrary signedhitcounts/coords, optionalbreakingfallback and
framecounts, overrides. Custom contained-item path omitted (ctor initializes0).
Pedestal typed name/text, PNGdungeon/podium_scroll.png, blockedmovement/clearvision,
stoppedframe0; verify_pedestal.py matches1,024 fullrecord/ctor/finalizer cases with
sprite/place/id boundaries. Eighteen local groups pass. New source awaits MacCI.
Next: Session ownership/collision/save/native rendering and hit handling, ordered
room content execution, full monster/trap/horde/element constructors/interactions.
Private breakable-and-pedestal-records.txt/breakable-runtime.txt/breakable-attack.txt
exports. Do not commit original resources or private analysis output.

Session owns breakables separately and pedestals in custom_objects(kind3). Bump
hits blocked breakables on passable base tiles; explicit strike_breakable requires
adjacent player. Handled hits consume provisional wait/enemy turn; object_hit/
object_broken events retain order. Enemy pathfinding respects both classes.
create_breakable accepts frame metadata or lazy resource callback invoked only
after appearance gate; failures preserve RNG/session. Optional savev7 <breakables>
retains base object/sprite triples/framecounts/hits/delay with strict state checks;
customkind3 onlyv7, breakablekind2 onlyinside breakable records. Earlier saves read.
Twenty local groups pass, including new breakable-session/resources tests;256
supplied starting characters still pass with unchanged integration digest.
Native RoomView builds supported visual records in relative source order (other
record constructors/interveningRNG still omitted), caches intact/breaking/broken,
draws static savedframes and transient PlayOnBob-style request on presentation
clock. Frame counts validate before load commit. Smoke fixture does hit→wait,
partial save/reload, invalid brokenpath/count rollback, finalhit/open/move and
pedestal collision. CMake adds native-breakable pair; current source awaits CI.

Resource lookup discovered actual missing direct expansion sprite paths. Original
SDL_main fragmentb6b6d..b6e8f withESI=b69c1 records26 PHYSFS_mount1c03d0 requests:
expansion3/2/1 game→/layer/ plus dungeon/items/skills/sprites/sfx/tilesets/ui append;
then dredmorcore.zip→/ prepend; then.→/ append. No coreZIP in suppliedinstallation.
resources.hpp/cpp implements directory mounts with expansion3→2→1→loose lookup,
canonical containment and backslash normalization. Packed coreZIP/modmounts remain
unsupported, explicit guard ifcoreZIPexists. verify_resource_mounts.py matchesall26
originalrequests andindependentlookup of13 breakablesprites (5expansionresolutions).
Native object/monster and installation hero/atlas use resolver. Fixture root has
intact1frame,exp2intact2,exp3intact3; broken4onlyexp2,breaking1onlyloose, proving
precedence/fallback onMac. Next: validate MacCI then full ordered content execution,
monster/trap/horde/element constructors and gameplay scripts/spells/fulllevels.

Run42630170521 passed all35 Mac tests and package smoke. RoomLootRecord integration
now lives in Session::create_room_loot: gate plus factory is atomic; accepted-null
is distinguishable from rejected appearance. Item room_local_id/can_push persist
in every container via optional savev7 attributes (oldv7 defaults empty/1). Native
supported ordered records now include loot; inventory/artifact smoke uses actual
room loot plus17 currency. GameContent::loot_context supplies gold/stddev/words;
its words pointer borrows content lifetime. verify_room_loot.py reuses full artifact
factory oracle with actual record/finalizer and placement/ID inputs;1024 cases match.
Original artifact factory1024 regression cases still match. Report room-loot.json
is private in analysis/. Current20 local groups pass; integration awaits MacCI.
Next: remaining monster/trap/horde/element constructors and full ordered population,
then complete level/session integration, scripts/spells/AI and visual comparison.

Monster::Monster eeace→ee45c changes class ranks with tweak "monster level adder"
before RebuildAttributes: each positive/negative step consumes libc rand and uses
multiply-high(rand*6), selecting0..2. Negative adjustment clamps ranks after all
steps; zero does not clamp. monster_creation_ranks implements wrapping updates,
with a1million-step port budget (not an original limit). Full original Monster/
Hittable/GameObj constructors and RebuildAttributes match2048 fixtures /69632 fields
using sprite-rebuild and tweak/library/allocation inputs. This covers ranks,
primary/secondary stats and libc state, excluding sprite/effects/selection parity.
GameContent::create_monster applies this adjustment; make_monster retains the
stat-only helper. Session room-monster construction runs after its appearance
gate, calls the supplied named/random constructor with owned RNG, marks room_spawned
(original468=1), and persists room ID/push/marker in optional savev7 actor metadata.
Failure/overlap rolls back construction/RNG/IDs; rejected appearances skip callback.
Native ordered supported records now include named monsters with original sprite
lookup and a new synthetic native-monsters fixture. Random monster/horde/trap/
element population, full effects/AI/scripts and full-level play remain unfinished.
Twenty local groups pass. New Mac validation is pending; room-loot run42632516392
was still queued at this checkpoint. No archive/toolchain blocker currently exists.

Monster level selection now matches4096 full original GetRandomMonsterLevel2bb7e
executions, including invalid boost bounds, MT rejection, alternate random depth,
expansion cap9/14 and signed-wrap edge cases. Monster EndElementdc6f0 registration
and InitMonsterTables612b0 match512 fixtures /16552 completed entries across4 named
pools,15x4 buckets and independent diggleHell registration. XML parser remains
outside that oracle. Independent source traversal agrees for all81 base monsters
and9 diggleHell entries. Base monDBLoad mask7 is observed at startupb7b1e/b7b26;
expansion calls use5,1,7 but merge semantics remain unsupported. Source close order
registers nested variants before their parent. Named lookup compares displayName
(falls back to own name, without inheriting the parent's display override) and
returns the first match across pools; actual dc570 probe confirms display/name
distinction. GameContent::create_monster now follows this registration lookup;
stat-only make_monster retains internal-name lookup. Out-of-range nonspecial
bucket indices fail explicitly instead of reproducing original out-of-array writes.
All20 local groups pass. Run42632516392 validated room loot onMac. Monster Mac
run42633520439 failed only because the new test lacked explicit<sstream>; fixed
in e537b02 and rerun42633827494 is queued. Full random SpawnMonster, population,
scripts/AI/spells and whole-level/native visual parity remain incomplete.

Regular fixed-position Level::SpawnMonster63a5a now matches1024 full original
executions through actual NewMonster648da, CanSpawnMonster60876, Monster/Hittable/
GameObj constructors and attribute rebuilding. Sprite rebuild, actual placement,
tweaks/library and unbalanced search-tree insertion/decrement are supplied boundaries.
Compared selected identity, rejected maxspawns candidates, resulting class/stats,
spawn byte354 bits2/8, all counts, libc state and next8MT. This is the room path
with flags(false,false,true,true), excluding platoons and random-position/terrain
branches. GetRandomMonsterLevel may return-1 before SpawnMonster's pre-clamp table
index; that invalid original read is excluded and port fails explicitly.
Monster maxspawns defaults0 (unlimited), confirmed entryctor e0dfc; atdepth-1 any
positive limit fails eligibility regardless current count. Named room construction
does not increment random spawn counts or draw the two spawn-flag MT values.
Random construction increments the internal-name count then MTbounded5/10 sets
raw flags8/2. Existing provisional BFS AI does not yet interpret those flags.
GameContent/Session now support both named and random room records with shared
owned RNG/state. Optional savev7 monster-counts and actor monster-flags persist
state; absent fields migrate empty/zero. Gate rejection skips callbacks; overlap/
factory failures roll back RNG/counts/IDs. Resumed mixed appearance/random spawns
and malformed counts pass; all20 local groups pass. Native fixture now includes
named plus random records, sprites, full-byte push/ID/count/flag checks and saves.
Run42634769177 passed all37 Mac checks including prior named-monster integration
and packaging, superseding the missing-sstream failure. Current random-spawn
source awaits MacCI. Full horde/trap/element population, complete constructor
sprite/effect semantics, scripts/spells/AI and whole-floor gameplay remain unfinished.

Random monster integration run42635717291 passed all37 Mac checks and packaging.
New traps.hpp/cpp models Trap/Item/GameObj construction, origin scans, sticky
Draw detection/reveal, player/monster step avoidance, spell requests, charge/
destruction changes and repeat disarm outcomes. verify_trap_runtime.py matches1024
full original sequences through Trap ctor10888a, TeleportTo1079f2, Draw1078a2,
SteppedOn1077ca, MonsterSteppedOn10753c, BlowUpTrap1081c4 and DisarmTrap107f62.
Sprites, base placement, blocker/safeTeleport, dynamic-cast wizard flag, spell
lookup/execution, player-idle/UI/XP/achievement and library inputs are boundaries.
Compared state after every call, requested origin and spell caster/target/mode/
death description, outcomes and next8MT. Cases include both caster modes, missing
spells, wrapping charges, origin scans across tile3/4, disabled/wizard/playerplaced
flags and two disarms to verify one-time XP. No original code/assets copied.
Ceiling traps use target position as spell origin; wall/floor traps request a
separate origin blocker. Floor scans consume MTbounded3. Player avoidance halves
the wrapping sum of secondary17/12/16; monster avoidance uses the full sum. Draw
uses Manhattan sight (secondary17), remembers discovery, and reveals sprite when
trap level<secondary16. A missing player spell leaves charges unchanged; monster
missing-spell path still consumes charges. Disarm XP is scale*(level+1) once,
except playerplaced; successful ceiling-affinity disarm draws MTbounded2. The
original "trap is destroyed" text branch does not actually enqueue destruction;
port exposes that outcome separately and preserves the observed state.
TrapMap rejects unchecked original out-of-map reads and fails atomically after
any tentative RNG draws. Twenty-one local groups pass, with lifecycle/rollback
checks. Current trap runtime awaits MacCI and Session/save/native ownership;
spell execution, full level queues and trap UI/audio remain unimplemented.

Trap runtime run42637405068 passed all38 actual Mac tests and packaging. Combined
RoomTrapRecord1a0ffe→GetRandomTrap4e410/named lookup→full Trap/Item/GameObj ctor→
TrapTeleportTo1079f2→base finalizer19f45c now matches1024 original creations via
verify_room_trap.py (multiple inheritance composes Traps and TrapOracle boundaries).
Compared accepted/null, actual selected definition, origin/state, full push/roomID,
libc/normal cache and next8MT. Resource/base placement/origin blocker/safeTeleport/
ID-binding and library inputs remain supplied. Earlier1024 runtime sequences still
pass. Reports room-traps.json and trap-runtime.json remain private in analysis/.
Session owns active trap instances separately from loot; item-ID allocation and
save duplicate checks include them. Appearance rejection bypasses all resources;
missing named trap commits accepted-null. Placement/map mismatch failures roll
back gate/selection/origin RNG and IDs. Optional savev7<traps> stores all modeled
state, origin requests and room metadata; earlier saves migrate empty. Named lookup
ordering, resumed random+appearance creation, bad maps and cross-container duplicate
IDs pass in21 local groups. Trap movement activation, owning origin blockers,
spell effects and native presentation are still pending. Source awaits own MacCI;
next dependency is measured spell execution rather than silently omitting effects.

Room trap/session ownership run42638509915 passed Mac CI and packaging. New
resolve_spell_damage models full DoSpellDamageca940 decisions/arithmetic. Its4096
full original executions match, using two owned libc draws, floorf/string/library
inputs, offscreen UI/audio and virtual TakeDamage delivery boundaries. Fixtures
cover primary/secondary scaling attributes, negative base/resist, float rounding,
large integers, NaN/infinity/signedzero and wraparound. Positive delivery totals
and explicit death text are also checked; HP mutation/effect dispatch are excluded.
Haywire always consumes its chance draw even when disabled, then suppresses spell
resistance when successful; it does not itself multiply this function's damage.
Resistance always consumes a second draw even when forced unresisted. Physical
channels truncate(float(base)+scale*attribute), subtract resistance only for a
positive channel, sum signed values then subtract armour/clamp0. Elemental channels
add base to floorf(scale*attribute), subtract resistance only when positive and
clamp each channel0. Resisted physical divides4, elemental divides2. These rules
are distinct from melee, especially negative physical channels and rounding.
New CLI dredmor-spell-damage and verify_spell_damage.py reproduce the comparison;
private report spell-damage.json. Existing21 local groups pass with damage tests.
Current source awaits MacCI. Full spell effect/type/target parsing and dispatch,
HP/effects/buffs, trap movement integration and native presentation remain pending.
Private spell-dispatch-and-parser.txt exports994e0/82abe/7c970/85924 for next work.

Spell damage source run42639187152 passed Mac CI and packaging. New spells.hpp/cpp
parses generic direct-child effect fields and damage/scaling arrays, preserving
raw spell/effect XML for pending execution. Defaults follow spellEffect ctor82abe:
primaryScale-1,secondaryScale3,percent100,floorScaleF1,resistable/affectsCorpses1.
Full actual effect StartElement7c970 + ctor match1536 synthetic cases, including
legacy numeric prefixes, float bits/nonfinite, target flags, chances and strings.
Generic property names ignore ASCII case, while ConvertEffectType54fc0 and
ConvertEffectTypeF82530 fallback names are case sensitive (crushing/crushingF).
Independent source ingestion agrees for947 supplied spells /1631 direct effects
across selected layers. Source report spell-effects.json stays private. Unknown
fields/options/buffs and type enum semantics are not claimed by this comparison.
GameContent exposes spells and exact-name first registration lookup; original
EndElement82150 appends to_spellDB and findSpell7c760 linearly returns first match.
Synthetic fixture tests duplicate names/first match, case behavior and defaults.
All21 local groups pass. Current parser source awaits MacCI. Target/effect dispatch,
buffs, HP changes, trap movement/native integration and other spell types remain
unfinished. Private spell-effect-dispatch.txt contains ApplyEffectAt904c0 (~6142
lines); spell-registration.txt records EndElement/findSpell. ConvertSpellType probes:
self0,missile1,adjacent2,area3,cone7,target8,targetfloor9,targetmonster22,template24.

Session::cast_damage_spell now connects self/adjacent/target/targetfloor damage
casts to owned RNG, HP, events/death and existing provisional XP policy, without
mana/cooldowns/turn scheduling. Unsupported effect/type/buff/mine/attack execution
fails atomically. verify_spell_cast.py executes2048 full original DoActualSpellLoop
994e0→ApplyEffectAt904c0 damage→DoSpellDamageca940 paths plus actual empty-buff
helpers. Hittable/Monster virtual identity/name and HP delivery, VFX/synchronous
Bob playback/UI/logging/library are boundaries. Accepted targeting, ordered damage
requests/flags, resulting HP under supplied delivery and libc state match. Original
HP/death/XP effects and other entity/effect types are not verified by this harness.
Self/adjacent force allowHaywire=true regardless caller flag; target/targetfloor
forward it. Damage-effect percent100 skips a chance draw; other values compare
libc bucket<=percent, so percent0 accepts bucket0. Self applies at origin, not the
explicit target argument. Adjacent checks wrapping square distance<2. Target
bounds reject without effect RNG. Session tests cover kills/XP, zero-percent edge,
self/adjacent bounds, mixed unsupported-effect rollback and resumed chance casts.
All21 local groups pass. Parser run42640439157 is inprogress; current basic-cast
source awaits MacCI. Automatic trap movement/casting, spell rules/save identity,
origin blocker ownership and native trap presentation are the next integration.

Basic cast run42642317094 passed Mac CI/packaging. Session now activates traps
on player and enemy movement using measured avoidance, spell requests, basic
damage casts and charge/destruction state. Actor terrain metadata parses/inherits/
persists, and nonzero monster terrain skips trap steps. Missing player spells
leave charges unchanged; missing monster spells consume charges per original.
Unsupported effects fail the entire provisional turn atomically, not silently.
Deferred pending traps stay in the owned vector, remain saved and are inactive;
original destruction-queue processing/global ID lifecycle is not yet reproduced.
Spell profiles are immutable shared definitions. Optional savev7<spell-rules>
content identity includes raw XML and all modeled typed effect fields/float bits;
load requires matching definitions. Older saves without identity migrate when a
profile is supplied. Native setup and reload attach the installation's spell rules.
Origin Blocker ctor a9860→a9732 and Builda91fa now execute in both trap oracles,
with sprite and SafeTeleport boundaries. They match movement-blocking1/vision0,
frame0/playing+looping1; full1024 runtime and1024 record comparisons still pass.
Trap::DestroyObject108baa only enqueues itself; the separate origin blocker remains.
Session collision and native origin sprites preserve that independent blocker
through trap depletion. Original tile/list attachment and visual timing remain
outside the oracle. Native record execution now includes traps with numeric room
cells, hidden/revealed stopped sprites, animated origins and sticky detection.
Loads prepare every trap image before replacing session/cache. New native-traps
fixture tests real movement damage, charges, reload, spell-identity/resource rollback
and persistent origin collision. Linux logic probe:HP22→15→8 (damage8 minus armour1),
charges2→1→0, origin(1,2). Full22 local groups pass with new activation/terrain/
profile-migration/rollback tests; current integration awaits MacCI. Complete spell
families/buffs, disarm UI, actual spell VFX/audio, horde/element population and
full-level play remain unfinished. Next: validate MacCI while extending effects.

Heal effects now execute with measured ApplyEffectAt behavior: currentHP<1 is
unchanged; amount adds with signed32 wrap; amountF contributes only when positive,
using the chosen scaling attribute and float conversion; only an upper maxHP clamp
is applied in the original. resolve_healing matches4096 actual heal branches,
including float/nonfinite/signed edge cases. Session cast_spell now handles mixed
damage+heal, emits spell_heal events, and rejects negative resulting HP atomically
because current save/actor invariants do not support that pathological state.
verify_spell_cast.py now matches4096 mixed actual DoActualSpellLoop→ApplyEffectAt
casts, combining original heal HP changes and supplied damage HP delivery; primary/
secondary scaling, chance gates, targeting and libc state remain covered. Existing
22 local groups pass. Full effect families/buffs/recursion, original damage HP/death
side effects and original whole-game scheduling remain incomplete.
Native trap run42645003384 failed an opaque smoke assertion despite the equivalent
portable movement probe passing. d2930ad adds stage/state diagnostics; rerun
42646040605 is inprogress. This is being fixed; no native-trap success is claimed.

Native trap/healing validation is now complete for this scope: actual macos-15
arm64 run42646389690 on4857128 passed all41 checks and development-app packaging.
The failed second activation was a synthetic input bug: the fixture sent arrow
characters with keyCode0, but keyDown dispatches arrow keyCodes123/124. Diagnostic
run42646040605 showed unchanged turn2/HP15/charges1. Correct keyCodes replay both
moves; damage, depletion, save/load, bad spell/resource rollback and persistent
origin collision now pass onMac. No gameplay workaround was introduced.
Original spell damage4096 edge fixtures, healing4096 and mixed damage/heal cast4096
comparisons pass; all22 local groups pass. Original assets remain private; source
is pushed only to implementation/native-foundation, with no main merge/release.
Overall game remains incomplete. Remaining major work: complete spell families,
buffs and recursive/delayed effects; full combat/AI and turn scheduling; horde/
element/script population; full generated-floor session/camera/doors/transitions;
original UI/audio and original-asset/full-game visual and gameplay comparison.
There is no current archive-transfer or native-build blocker. Continued work is
implementation and validation, not awaiting further user permission or assets.

Immediate trigger effects (ApplyEffectAt case44, around941b3) now execute nested
spell lookup/casts. Triggers consume a libc roll even at percent100; bucket0 passes
percent0. Missing child returns from the current parent's effect loop, leaving
prior effects intact. Child origin is the caster's current position and child
allowHaywire is always true. First matching spell wins, including duplicate names.
verify_triggers.py matches4096 actual nested DoActualSpellLoop/ApplyEffectAt/findSpell
chains with mixed damage/heal, all4 supported dispatch types, missing/duplicate
children, self-property values, ordered requests, HP and libc state. Identity/name,
damage HP delivery, UI/animation/library remain boundaries. Deferred amount!=0,
buff gates, other spell families and original HP/death effects remain excluded.
Session recursion has a port safety budget of64 frames/4096 calls; budget/unsupported
child failures roll back the entire root cast. Missing child is normal early return.
New spell-triggers tests cover origin, order, zero chance, resumed RNG/profile saves,
cycles and delayed/unsupported rollback; all23 local groups pass. Previous4096 mixed
cast comparisons still pass. Native trap fixture now triggers a separate damage
spell, exercising nested casts through movement/resources/save/load. Current source
awaits MacCI. Continue full spell families, scheduled effects/buffs and level play.

Spellpoints effects (ApplyEffectAt case37) now update global player MP regardless
of caster/target occupancy, with caster primary/secondary scaling, signed amount
wrap, positive-only amountF and lower0/upperMaxMP clamps. No current-HP gate applies.
verify_spellpoints.py matches4096 actual arithmetic cases plus4096 full mixed
DoActualSpellLoop damage/heal/spellpoints casts, including monster casters, empty
targets, chance/targeting, HP/MP, request order and libc state. Virtual identities/
names, damage HP delivery and UI/library remain boundaries; other families/buffs
and original damage HP/death are excluded. Existing4096 heal cases still pass after
sharing resource arithmetic. Session emits spell_mana events; nested mana casts,
clamps, global recipient and save/resume tests pass in23 local groups. Native trap
fixture now spends3MP before triggering and restores1MP in its damage child, proving
net2MP per activation through reload. Current mana source awaits MacCI. Immediate
trigger run42688935604 is inprogress; full game remains unfinished.

Immediate-trigger run42688935604 and mana run42689569756 passed42 Mac checks and
packaging. Template AoE now loads all35 base manTemplateDB templates and executes
ordered area effects, including nested trigger/damage/healing/mana. Planner matches
4096 actual DoActualSpellLoop template dispatches; integrated AoE matches2048 actual
full casts on safe interior coordinates. Ordered damage flags, HP/MP, libc, random
selection, last matching explicit templateID, first anchor and coordinate wrapping
are compared; identity/name, damage HP delivery, UI/library are boundaries. Records
iterate x ascending/y descending. Anchored1 uses supplied origin; other anchored
values use target. Anchored0 ignores facing. Facing2 is identity;0 mirrorsY;1 swaps
x/y;3 maps(-y,x). These transforms were verified rather than replaced by rotations.
Player::WalkTo11c6ee writes east3/west1/north2/south0; successful provisional moves
now update a saved player-facing value (port default2 for older saves). Original
animation/input queue and facing on attack/blocked paths remain unverified.
Optional savev7 facing and spell-rules maneuvers hash preserve orientation and
raw/typed template rules; old saves lacking these fields migrate default/current
rules. Changed rule cells or invalid facing reject before session replacement.
Template22 has three short rows (7declared/6characters). Actual parser cf870 reads
the terminating zero and leaves those cells unwritten; port normalizes missing
cells empty. This is an explicit compatibility policy, not measured heap content.
Unknown symbols/invalid dimensions fail; extra row characters are ignored like the
original. Independent ingestion covers35 templates and notes3normalized cells.
Actual template dispatch can call ApplyEffectAt outside the map and perform invalid
reads (probe at9698b). Port clips those effect coordinates; coordinate comparison
still covers outside/wrapping values, full-effect comparison excludes unsafe reads.
Empty-effect maneuver attacks, affectOnlyHittables, buffs and visual playback stay
unsupported. Session tests cover ordering, identities, facing, saves/changed rules,
atomic failures and bounds; all24 groups pass. Existing4096 trigger chains,4096 mana
arithmetic+4096 mixed resource casts still match after effect-loop refactor. Native
trap fixture now triggers a template damage/mana child and saves its template hash.
Current AoE source awaits MacCI. Next: scheduled effects/buffs, broader spell types,
full population/scripts and complete generated-floor play.

Area-spell integration a39f97b passed actual macos-15 arm64 run42692141736: all43
checks and relocated development-app packaging. This includes template rules/facing
save tests and the native trap→trigger→template damage/mana replay, with both
activations, depletion, save/reload and invalid resource/rule rollback. Twenty-four
local groups pass. New source is pushed on implementation/native-foundation only.
No original assets/reports were committed and main/releases remain unchanged.
Next concrete work is deferred trigger/buff scheduling and additional spell types,
then horde/element/script population and complete generated-floor session/UI flow.
The overall reimplementation remains incomplete; these validations cover the named
subsystems and supplied original-routine comparisons, not whole-game parity.


Scheduled spell integration now supports ApplyEffectAt deferred trigger44 and
repeat dot43, plus Hittable::UpdateTriggers45326. 2048 original sequences match
creation, DoResistancec9490, counters, eligibility, child heal/mana HP/MP, queue
fields and libc state across multiple passes. Per-actor counters wrap int32,
decrement before firing, and remove zero entries after the traversal. One-shot
entries fire at zero even on dead owners; repeat entries require live owners.
Initial repeat count0 becomes-1 and persists. New entries are visited during the
same pass; global player position supplies scheduled cast origin regardless of
stored caster. A shared4096-operation budget rejects cycles atomically.
Session advances these passes at its existing provisional turn boundary. Full
original action-queue placement, overlapping actor object-list order, buffs/taxa,
Steam achievements and missing dot-child behavior remain excluded. Player dot
resistance is rolled before the dead/corpses gate; monster eligibility checks it
first. Buff gates remain explicit unsupported errors.
Saves containing pending spells use port format8 with matched spell rules,
validated owner/caster/index/flags and ordered per-actor queues. This prevents old
format7 readers silently discarding scheduled work. Saves without pending work
continue using7; versions1..7 still load. Pending rule replacement rejects.
Tests cover wait scheduling, no tick on blocked input, nested same-pass additions,
monster-caster self origin, wrap, resumed effects, malformed identities and cycle
rollback. All25 local groups pass. Native trap smoke now also saves/resumes a
countdown and a repeating heal/mana spell; current source awaits MacCI.


Scheduled sourceb780907 passed actual macos-15 arm64 run42697451427,
job129432973508: all44 checks plus relocated development-app packaging. Native
trap smoke exercised delayed countdown firing and three repeat heal/mana passes,
including save/reload while both kinds were pending. No original assets uploaded.
Further original call-site analysis traced GameTick2b480: level object virtual
Update passes run before player virtual UpdateBuffs+Hittable::UpdateTriggers;
Monster::Updatee9b1a updates triggers before buffs and subsequent AI, when43c==0.
Session now runs each monster trigger pass before its AI, then player triggers.
A test verifies a monster-created player trigger can fire in that same turn.
Complete level-object ordering, speed/time flags and buff passes remain pending.

Buff metadata is now typed from the actual Buff constructoradae4 and spell XML
callback7c970 (StartElement; effect branch7fdf1): time/stacking/target/upkeep/equipment flags,
strings and primary/secondary/damage/resistance modifiers. Unknown children stay
in source. 1024 synthetic definitions plus all268 supplied buff definitions
across base/expansion layers match actual original callback fields. No expansion
merge/execution scope is inferred from this ingestion. Constructor and Bob
initialization execute; SpriteMgr identities, string/ASCII/atof/allocation are
controlled boundaries. Invalid modifier IDs reject instead of reproducing
out-of-bounds writes. Highbuff aliases primarybuff; sightbuff replaces secondary18.
The Hittable::UpdateBuffs44fc0 timer decision matches2048 original passes. A
nonpermanent counter decrements with int32 wrapping and expires only below zero
when useTimer is true. Untimed counters still decrement. Permanent counters do
not decrement. Pending permanent buffs are deleted without virtual deapplication;
other pending nonpermanent buffs deapply before deletion. Virtual stat removal
and destruction are recording boundaries, not verified full buff side effects.
All26 local groups pass. Buff application/stat rebuilding, stacking identity,
upkeep, gates and native buff display remain unfinished; typed or XML buff casts
still reject atomically, so metadata is not silently treated as active gameplay.
Current order/metadata source awaits MacCI.


Buff metadata/order source10777d5 passed actual macos-15 arm64 run42704964742,
job129459245528: all45 checks, native scheduled effects/save-reload, and relocated
packaged-app smoke. All26 local groups and the original buff comparisons pass.
The comparison script now includes an explicitly original-only expiry/rebuild
probe: actual Player::ApplyBuff11a880, Hittable::UpdateBuffs44fc0,
Player::DeapplyBuff1233cc and Player::RebuildAttributes1218de execute with no
skills/abilities/equipment. A time0 buff contributes7 slashing damage and3 toxic
resistance; expiry removes/deletes the buff, but those values remain7/3 until
an explicit subsequent attribute rebuild resets them0/0. This follows rebuilding
while the expired buff is still present in the vector. The destructor is a
recording boundary. This is observed legacy behavior, not port buff execution
or a generalized comparison of all stat/death/upkeep paths. BuffSort115a90 orders
by buff name. Retain this distinction when implementing active buff state.

Next implementation: active player/monster buff application and removal, ordered
stacking/icon identities, upkeep and trigger gates with matching saves, followed
by additional spell dispatch/effect families and complete generated-floor play.
The overall reimplementation is still incomplete; no completion percentage has
been inferred from kernel tests or code line counts. Archive/tools and actual
native compilation are available, rather than external blockers.


Active player stat buffs now apply after spell effects for supported self/target/
adjacent/template dispatch. Primary/secondary flats compose with character,
equipment and ability rebuilding; HP/MP flat changes and clamps follow measured
apply/deapply paths. Nonstacking refresh replaces the lifetime without reapplying
stats or resetting upkeep. Stack caps use supplied icon identities; ordering is
by parent spell name. Expiry cancels scheduled triggers with that parent name.
Legacy damage/resistance residue survives expiry until a later rebuild, matching
the original rather than eagerly correcting it. Player buff timers run before
player triggers at the existing turn boundary.
Mana upkeep uses its own wrapping countdown, consumes1MP per interval, and marks
the buff for removal if MP is exhausted. All upkeep payments precede the shared
timer/deapplication pass. Timed and untimed buffs are supported. Save format9
persists ordered spell/buff indices, lifetime/upkeep counters and matched typed
buff rules. Versions1..8 remain readable; active rules cannot be replaced. Bad
indices/order/content and unsupported state transitions reject atomically.
verify_buff_session.py matches1024 original full sequences through Player::ApplyBuff,
Hittable apply/deapply/update, Player::UpdateBuffs, Player::DeapplyBuff, Buff copy/
sorting and full Player::RebuildAttributes. Each operation compares primary6,
secondary24, damage16, resistance16, HP/MP and ordered buff identity/counters;
port save/resume operations are interleaved. Original string/allocation, icon
identity and destructor boundaries are supplied. No abilities/equipment in the
comparison fixture; their existing composition tests remain separate.
Monster-owned buffs, lethal apply transitions, non-mana upkeep, equipment/status
callbacks, icon-resource aliases and large equal-name sort ties remain excluded.
They must not be inferred from these comparisons. Unsupported buff families fail
atomically. Native trap smoke now also tests active buff apply/refresh, mana
upkeep, mid-buff save/reload, expiry residue and a subsequent rebuild. All27 local
groups pass. Current active-buff source awaits MacCI; full game remains incomplete.


Active player-buff sourcec5e7daf passed actual macos-15 arm64 run42719251155,
job129508693431: all46 checks and relocated packaged-app smoke. Native active
buff application, refresh, mana upkeep, pending save/reload, expiry residue and
subsequent rebuilding passed. Original1024 full player sequences also match.

Trigger44 now honors requireBuffOnTrigger/requireBuffOnNotTrigger and their names.
Immediate casts consume chance first and inspect caster buffs. Deferred triggers
store flags/names, consume chance at firing and inspect owner buffs. Missing buff
conditions suppress the cast; zero-count entries still expire. Names refer to
parent spell names rather than buff tags or icons. Dot43 retains its original
default gates instead of copying trigger-only fields. Generic requirebuff9c
remains explicitly unsupported. Monster buff application is still unavailable;
monster buff vectors are consequently empty in the supported session state.
verify_trigger_gates.py matches2048 actual original immediate/deferred sequences,
including owner/caster differences, required/forbidden/both conditions, absent or
empty names, percent-1/0/1/50/99/100/101, per-pass global player MP, trigger counts
and libc state. Original buff-name fixtures, virtual identities, strings/lists/
allocation are supplied boundaries. Real lookup/dispatch/resource arithmetic and
UpdateTriggers execute. Port save/resume is inserted before firing.
Save10 preserves gated queue fields and compatible active-buff state; versions1..9
remain readable. Downgraded gated saves and malformed flags/identities reject.
Tests also cover active buffs expiring before the player's gated trigger pass.
Native trap smoke now saves/reloads a pending gated resource spell while its
required buff is active, then checks upkeep, firing and expiry. All28 local
groups pass. Current trigger-gate source awaits MacCI. Full game remains unfinished.


Trigger-gate sourcea6db78e passed actual arm64 macos-15 run42719863912,
job129510770319: all47 checks and relocated packaged-app smoke, including gated
pending spell saves with active buffs and mana upkeep.

Buff ordering is now explicit and independent of host std::sort tie behavior.
verify_buff_order.py matches1024 full identity permutations through original
BuffSort115a90, introsort12abd0, insertion12aeb0/12af90 and heap fallback. Inputs
include lengths0..4096, equal names, sorted/reversed data and forced depth0..3
plus the normal computed budget. String/comparison/memmove library boundaries
are supplied; original partition, recursion, insertion and heap code execute.
Player::DeapplyBuff sorts during Hittable's timer traversal, so entries can be
revisited or skipped. The session now mirrors that movement and stores primary/
secondary buff flats independently of the active vector. Original deapplication
can repeat for one entry; cleanup can delete another without deapplying it.
A controlled24-identical-buff case leaves12 active entries after the first pass
but zero net primary flats; the second pass removes those12 and leaves-12 primary
flat, even with no active buffs. A subsequent rebuild retains this legacy result.
verify_buff_session.py now matches1024 full sequences including256 large-stack
cases (24/32/48 applications), mana exhaustion, sorting during expiry and port
save/resume. All compared primary/secondary/damage/resistance/HP/MP and ordered
identity/lifetime/upkeep fields agree after each operation. The prior limited
large equal-name scope is superseded for these modeled stat/upkeep families.
Save11 retains independent buff flats, including empty active-vector residuals.
Older saves1..10 migrate their modeled flats from their active entries. Gated
queues remain supported, and source/rule replacement rejects while residual buff
state exists. All28 local groups pass, including the measured24-stack fixture,
small-save9 migration, empty-vector residual save/reload and rebuild. The2048
trigger-gate comparison still matches with save11. Native smoke now exercises
large-stack expiry, midpoint saves and residual-state saves. The room-view HUD
shows up to three active buff names and remaining turns, with an overflow count.
Current sorting/state/HUD source awaits MacCI. Monster buffs, other upkeep/status
families, lethal apply paths, generic requirebuff and full floor play remain work.


Buff sort/residual/HUD source33d4912 passed actual arm64 macos-15 run42721134617,
job129515080122: all47 checks and relocated packaged-app smoke, including24-stack
expiry ordering, midpoint saves and residual-stat saves after the active vector
is empty. The room-view name/timer HUD compiles on actual AppleClang.

The generic ApplyEffectAt requirebuff9c condition is implemented. Despite its
name, it permits effects when at least one eligible spell-template buff icon is
missing from the target. If no eligible template/target or every eligible icon
is already present, ApplyEffectAt returns from the whole remaining effect loop.
A rejected chance skips only that effect, so later effects still execute. Chance
precedes this condition. Subsequent spell buff application still executes after
a condition-driven return. Template applytoplayer flags gate eligibility;
player and live unbuffed monster target paths are modeled. This gate uses icon
strings, unlike trigger conditions which use parent spell names.
verify_effect_gate.py matches2048 actual original self/adjacent/target/targetfloor
casts with zero/one/two templates, existing/missing icons, applytoplayer flags,
percent/chance and empty/outside targets. Accepted state, global player MP and
libc state agree. Buff application is a no-op recording boundary in this oracle;
buff icons and virtual identity/library are supplied. Separate active-buff
comparisons and integration tests cover modeled application/removal. Monster
owned buffs and original VFX remain excluded. The4096 spellpoints arithmetic
and4096 mixed resource cast regressions still match after common chance handling.
All29 local groups pass. Native smoke now checks conditional resource use on
first application, suppression while a matching buff is present and save/reload.
Current generic-gate source awaits MacCI. Next major gap: owned monster buffs
need retained constructor/stat profiles and persistence before safe rebuilding;
remaining statuses/upkeep, more spell families and complete generated-floor play
also remain. The overall game is not finished.


Generic-gate sourcee24f24d passed actual arm64 macos-15 run42722079171,
job129518259367: all48 checks and relocated packaged-app smoke, including first
application, saved buff state and suppression of a repeated conditional effect.

Owned monster stat buffs now execute with retained MonsterStatInput profiles.
GameContent retains actual adjusted class ranks, definition/flat/percentage
fields, current constructor depth and HP tweak values for each created monster.
Hittable apply/deapply updates flats, HP/MP, damage and resistance, followed by
full monster rebuilding. Monster self application appends without player
refresh/cap/sorting policies; target application honors the original icon-based
nonstacking precheck and self redirection. Monster updates run triggers, then
buff timers, then provisional AI. The generic icon and trigger name conditions
now inspect owned monster buffs. Hittable monster updates do not charge player
mana upkeep; the copied upkeep clock stays unchanged. Expiry removes the owning
parent's scheduled triggers. Monster damage/resistance removal is immediate,
where player rebuilding preserves the separately measured residue quirk.
verify_monster_buffs.py matches1024 actual original full sequences with varied
ranks/definitions/flats/percentages/depth/HP tweaks. It executes Hittable apply,
deapply and UpdateBuffs, Buff copy, and full Monster::RebuildAttributes. Every
operation compares primary6/secondary24/damage16/resistance16/HP/MP/dead flag plus
ordered buff identities/lifetime/upkeep fields, with port save/resume interleaved.
128 fixtures inject HP0/dead1 in the original and use a fatal port cast, then
compare dead-owner expiry. Tweak/string/allocation/destructor and the initial
controlled death transition are boundaries; full original damage delivery, AI,
status/VFX and complete spell-buff targeting remain excluded.
Save12 retains monster profiles and owned buff state; prior saves without such
profiles remain usable but cannot reconstruct unknown randomized constructor
inputs for owned buff rebuilding. Nonfinite profiles and missing/mismatched buff
rules reject before replacement. Save13 retains positive-HP corpse flags: the
original Hittable deapplication floor can leave HP1 while the monster's dead flag
remains set. The port preserves that flag through saves and skips corpse AI;
Monster::Update's following pass resets corpse HP to0. Ordinary HP0 dead actors
migrate without changing older canonical save forms. Lethal buff application and
pathological maxHP0 transitions are still explicitly unsupported.
The2048 trigger-condition and2048 generic-condition comparisons now include
independently present player and monster buffs, including saved owned states.
All30 local groups pass. Native room-monster smoke now applies a targeted owned
buff, checks nonstacking, executes a gated child, saves/reloads the owned state,
and verifies full stat/resource removal while preserving actor images/profiles.
Current monster-buff source awaits MacCI. Complete floor transitions/population,
remaining spells/status/upkeep, AI and original UI/audio are still unfinished.


Monster-buff source1c1706b passed actual arm64 macos-15 run42724448513,
job129526348564: all49 checks and relocated packaged-app smoke. Native owned
monster buffs, gated child, profile/resource save/reload and complete healthy
removal passed in the room-monster fixture.

Player gold upkeep now follows Buff::GetZorkmidUpkeepada70 and Player::UpdateBuffs.
The fee is floorf(float(base)*scale*factor), with two SSE single-precision
multiplications. The selected depth is alternate_depth when alternate is true,
otherwise depth; nonnegative depth uses wrapping depth+1 converted to float,
and negative depth uses the original constant16.0. Out-of-range/nonfinite float
conversion yields INT_MIN; a transfer that would recurse indefinitely still
rejects atomically through the existing currency safeguard. Only positive base
upkeep activates charging, so a negative scale can grant money as in the original.
Each buff's mana then gold processing completes in list order. Insufficient
money marks removal and queues its optional spell name. All queued names resolve
and cast after payment processing but before timer/deapplication passes. A new
callback buff participates in that timer pass; its upkeep starts on the following
pass. A mana-depleted pending buff can still pay gold, and later callback mana
does not rescue it. Missing callback spells are skipped; cyclic/unsupported
callbacks roll back the complete session action.
verify_gold_buffs.py matches4096 actual fee calculations (including alternate/
negative/extreme depths, signed amounts and nonfinite scales) and1024 original
Player::UpdateBuffs/Hittable/rebuild sequences with ordered mana/gold processing,
callbacks, flats/resources/wallet/buff clocks and port save/resume. Floorf/string/
icon/allocation/destructor boundaries are supplied. The sequence oracle supplies
wallet subtraction and records insufficient-funds cast arguments while delivering
+5MP; the port executes the corresponding actual spellpoints child. Full wallet
UI and arbitrary child execution are not covered by this sequence comparison;
separate currency/resource/trigger comparisons remain applicable.
Session depth context persists in save14 for nondefault contexts; older saves
retain depth0 defaults. Native scene dungeon_depth configures it and room loot/
monster/trap construction receives that context. This is not full-floor play or
floor transitions. Native trap smoke tests two payments, midpoint reload/depth
preservation and an insufficient-funds child. All31 local groups pass, including
callback-created buffs, income from negative scales, mana/gold ordering, malformed
depth and cyclic callback rollback. Current gold/context source awaits MacCI.
Next: player-initiated spell selection, measured mana/cooldown/action behavior,
remaining spell dispatch/effects, and complete floor population/transitions/UI.
The overall game remains incomplete despite the validated named subsystems.


Player casting integration now parses typed costs/requirements, downtime/multiAttack/
noAnimation/attack and ability spell refs/triggeroncast (default chance5 when present).
Actual original MPCost82402/GetZorkmidCost82386 match4096 cases; constructor/full
spell-start/requirements callbacks match512 synthetic+947 supplied spell roots
across base/expansion layers; AbilityC2/triggeroncast callback matches512 fixtures.
Floating-point floors/conversions and the spell gold negative-depth factor1.0
are preserved (buff upkeep uses16.0 instead). verify_casting.py supplies library,
filename normalization and sprite-loading boundaries, excluding effect execution.
PlayerSpellCast9ba10 comparison adds1024 action sequences: mana/wallet, multi-cast
arguments, cooldown arming, silence bypass, oncast strict '<' probability and libc
state. No-animation self fixtures supply UI/audio/analytics and -1MP effect delivery;
GameTick boundary counts one tick and decrements positive cooldowns. Port runs its
actual resource effect engine and room scheduler. This is not a comparison of full
GameTick/click visibility, original presentation, arbitrary effects or learning.
Both insufficient gold AND mana consume their own failure-voice random draws.
New Session::use_spell enforces learned refs, equipment and supported target gates,
pays normal spell costs, runs casts then registration-ordered learned oncast hooks,
and advances one turn. Positive cooldowns decrement on ordinary session turns;
cooldown abilities bypass MP/gold/mute checks. Buff mute now blocks ordinary player
casting; other unsupported statuses still reject. Unknown child spell refs skip.
Shared4096-call/64-frame guards and atomic session copies prevent partial failure.
Save15 stores nonzero cooldown state, retaining negative overflow values (original
only decrements positive values). Old formats migrate with empty cooldown state;
typed casting/ability refs get a separate content hash, preserving legacy hashes.
Native A/Character > Spells lists learned costs/cooldowns, self casts immediately,
and other spells await map click; Esc cancels. Synthetic casting smoke checks menu,
selection/click, costs, cooldown disablement and save/reload. Full FOV/obstruction
validation, polymorph/equipment-generated spell lists, remaining dispatch and HP/
booze/level requirement consumers remain unfinished; unsupported casts roll back.
All33 local groups pass. New native casting source awaits actual arm64 MacCI.
Prior gold-upkeep commitb1cb76b passed run42726567130/job129533591405: all50 checks
and relocated package smoke. Full game/floors/AI/remaining effects/UI/audio are
still incomplete. Next work must integrate more original behavior, not infer a
completion percentage from line counts or isolated comparison results.


Casting commitc4651ad compiled and its native casting fixture passed on arm64 Mac;
run42731600775 exposed an older trap smoke's positional XML attribute assumption.
Compatibility fix82d0547 restored content-first output; run42732023110,
job129552130079 passed all54 checks plus relocated packaged-app smoke.
Potion use is now implemented for paid player inventory potions in the room session.
Item potion/casts spell refs retain repeated-tag semantics and case folding. Actual
itemDBEntryC2/StartElement callbacks match512 synthetic potion/casts sequences plus
supplied potion roots across all four content layers. Actual Potion::UsedOn140d26
matches1024 self-use cases: return, quantity/removal and all low-level spell-call
arguments. UI/audio/animation/identify/destruction/inventory-removal are supplied
boundaries; spell delivery supplies controlled MP changes, while the port executes
its actual spellpoints effect. Shops, monster use, original identification/tutorial/
achievements and caller turn scheduling are excluded from that comparison.
Session::drink_potion executes the linked internal spell without player mana/gold/
cooldown costs, consumes exactly one unit without relocating the stack, and then
advances the provisional room scheduler. Missing spell refs still consume the item;
unsupported or cyclic effects roll back the entire action. Separate consumable
content identity guards typed refs on load without changing legacy item hashes.
Native inventory now has Drink. Casting smoke also drinks a two-unit stack through
the inventory controller, saves/reloads the remaining item, then consumes its last
unit and verifies removal/resources/turns. All34 local groups pass; potion/native
source awaits MacCI. Food::UsedOn13f9d2, Hittable::Update4493a and Player::Update12158c
have been inspected for the next integration: food/booze add signed counters114/118;
Hittable consumes positive counters and grants at most1HP/MP per update even at full
resources (counter still decrements). Natural player regeneration uses independent
clocks878/87c and max(1,wrapping(tweak-secondary regen)) intervals; Vampirism prevents
natural HP gain. These food/natural regeneration paths are NOT implemented yet.
The full reimplementation is still unfinished: complete floors/transitions, remaining
spell/status/equipment triggers, AI/combat delivery, food/drink regeneration, original
UI/audio/FOV and end-to-end original reference validation remain substantial work.


Potion commite24f8a3 passed actual arm64 run42732876605/job129554980424: all55
checks plus relocated package smoke, including inventory-controller stack drinking.
Natural player regeneration is now integrated for new content-based characters
whose tweak tables contain hpregenturns and spregenturns. Manual/legacy sessions
without a configured profile retain their prior behavior. Regeneration rates and
independent clocks persist in save16; it also supports prior cooldown/depth/buff/
monster payloads. Player casts reset the mana clock for every main multi-cast;
triggered children and potions do not. Blocked actions do not tick. Native character
and room setup resolve Vampirism by the selected skill name, matching Player's
natural-regeneration skill-name scan. Full object-queue ordering remains provisional;
the session deliberately skips resource recovery for a dead player to prevent its
incomplete death scheduler from resurrecting one. This guard is port integration
policy, not a claim about full original death processing.
verify_regeneration.py compares4096 actual Player::Update12158c calls, including its
actual Hittable::Update4493a, with HP/MP, food/booze counters and natural clocks.
Signed subtraction/increment wrap is preserved; intervals below2 become1. Food/
booze consume positive counters even at full resources; natural clocks reset at
threshold even without a gain; Vampirism blocks only natural HP gain. Time-disabled
updates retain counters/clocks but can still perform a previously-due natural
regeneration check, as in the original. GameObj update, mines, virtual rebuild,
tweaks, sprites/eye candy/string boundaries are supplied; invisibility/polymorph/
water-walking/Steam branches are absent. This is not a full GameTick comparison.
All35 local groups pass, including combined save16 regeneration+cooldown+depth,
blocked actions, resource thresholds, Vampirism and cast-clock reset. Native
regeneration fixture enables real tweak lookup, drains both resources, advances
keyboard turns and checks full recovery across save/load; current source awaits
MacCI. Food/booze counter arithmetic is measured, but inventory food consumption,
its WalkBuffs hooks, dietary traits and their UI are still unfinished.


Configured player update also now performs the existing measured equipment/ability/
buff stat rebuild after regeneration and before buff upkeep/expiry, matching the
healthy Player::Update path (polymorph/rebuild-suppression states remain unsupported).
This clears a prior expiry pass's damage/resistance residue on the following turn;
the expiry turn itself still retains that residue. A session regression checks
both turns. All35 local groups pass after this integration adjustment.
Private next-system analysis: Food::UsedOn calls WalkBuffsc9550 for booze, food,
then consumable hooks. WalkBuffs gathers selected buff/ability/equipment spell
pointers before executing the gathered list; percentage100 bypasses RNG, other
percentages use strict '<'. Equipment traversal and melee-family gates need to
be completed/compared before food/attack trigger integration. No food consumer
or WalkBuffs execution was added by the regeneration change.


Configured regeneration/rebuild commit34ef5a1 passed arm64 run42733979458,
job129558609093: all58 checks and relocated packaged-app smoke. The earlier
regeneration commit0f79a29 also passed58 in run42733649842/job129557521098.
Food/booze inventory self-consumption now uses the measured recovery counters.
Case-insensitive food attributes retain omitted fields across repeated tags, use
legacy atoi semantics, and bind effect= spell refs; casts can overwrite the ref.
Typed hp/mp/meat fields get a separate nutrition content identity on saves.
Session::eat_food adds signed counters with32-bit wrap, runs each learned vegan
flag's Fallen Vegan spell for meat then the item's linked internal spell, consumes
one item in place and advances the provisional room scheduler. Missing item spell
refs still consume; missing required dietary spells and unsupported/cyclic effects
roll back the entire action. Food checks stackability as well as quantity, unlike
potion consumption. Food requires configured regeneration rules, preserving manual/
legacy sessions without invented recovery rates.
Vampirism's restriction is a selected-skill-name check (Player::HasAbility115a3e),
NOT learned ability-name presence. The same persisted skill-name-derived vampire
flag used by natural HP regeneration supplies that gate. Exactly mp==0 is blocked;
negative mp is nonzero and bypasses the restriction, matching the original. Vegan
flags are read from learned abilities' flags veganism attribute; each eligible flag
can cast its penalty. Reactive foodBuff/boozeBuff/consumableBuff WalkBuffs traversal
is still unfinished. Applicable hooks reject atomically instead of silently losing
spells. None of those three hook tags occur in the supplied base/three expansion
XML layers, but this restriction matters to mods and future trigger integration.
verify_food.py matches1024 two-use original Food::UsedOn13f9d2/Hittable::Update4493a
sequences with port save/reload between uses: HP/MP, nutrition, quantities, accepted
uses and dietary/item spell-call arguments/order. Actual selected-skill-name and
Ability::PlayerHasAbility eligibility routines run. WalkBuffs is a supplied boundary
checking booze-food-consumable invocation order with no reactive spells; linked
spells supply controlled -1MP, while the port runs its real resource effect. UI/
audio/animation/inventory-removal/destruction/GameObj/mines are supplied. Paid
player self-use/time-enabled cases only; shops/monster use, arbitrary linked effects,
full object-queue ordering and original UI are excluded. Actual itemDBEntryC2/food/
casts callbacks also match512 synthetic+49 supplied metadata roots across layers.
All36 local groups pass, covering recovery across turns/reloads, accumulation,
selected-name versus ability-name vampire distinction, meat penalty, missing-item
no-op, typed food hash mismatch and unsupported hook/effect rollback. Native Use
handles food and potions, and the HUD displays remaining food/booze counters. New
food smoke uses a two-unit mixed meal through inventory controls, saves/reloads
mid-recovery, consumes its last unit and checks resource/counter exhaustion.
Food/native source awaits MacCI. Full floors/transitions, remaining spell/status
and reactive combat effects, AI/death/object scheduling, original UI/audio/FOV and
full end-to-end reference validation remain unfinished; this is still a room-based
integration, not the complete game.


Food commit73a23d1 passed actual arm64 run42735545209/job129563753840: all61
checks and relocated package smoke. Native mixed-meal inventory use, final stack
removal, recovery counters/HUD and save/reload passed.
Automatic alcohol statuses now follow Player::ApplyDrunkennessBuffs11aab8 after
Hittable's buff timer/cleanup pass. Select the greatest positive booze requirement
not exceeding the remaining booze counter; equal thresholds retain the first
registered entry. A matching first drunken buff is kept, while a mismatched first
one is deapplied and the caller's original vector slot is erased after the deapply
sort (preserving the original slot/identity quirk). Selected internal spells cast
with allowHaywire0 and the original target-y=player-x argument; self dispatch
normalizes that target for the supplied six alcohol spells. The first resulting
marked buff's countdown resets0 after every alcohol update. Display duration280
is presentation and is not stored separately in the port.
Spell requirements/buff XML order determines the drunken marker184. A positive
booze requirement retroactively marks existing templates; later nonpositive
requirements leave old markers intact, while new templates use the current value.
The flag is typed and protected by a separate optional drunken content identity,
preserving earlier buff signatures. Save16 already carries booze and active buffs;
no additional active-state payload is needed. Shared player deapplication now
serves timer expiry and alcohol replacement while preserving flat ghosts and
damage/resistance residue until the subsequent rebuild.
verify_drunkenness.py matches1024 sequences of1..16 actual alcohol updates, using
actual Hittable timers, Player apply/deapply/rebuild and Buff copy. Compare full
primary/secondary/damage/resistance/resources, booze and owned identity/countdown;
port saves/reloads between updates. A harness-generated cdecl thunk invokes the
actual Buff copy and Player apply routines at the DoActualSpellLoop boundary;
that boundary checks every cast argument. Template pool/ranks are controlled,
arbitrary child effects and full GameTick/food processing excluded.512 original
spell XML start/end callback sequences match marker propagation. The existing
1024 full player-buff comparisons (including256 large-stack cases) also pass after
sharing removal code. All37 local groups pass. Native food smoke now checks tier1
then tier2 on successive meals, tier downgrade and complete final removal across
saves/recovery. New alcohol source awaits MacCI.
Display-only Well Fed/Fuelled by Booze vector entries and their original icon/
duration presentation are still absent (the native HUD shows food/booze counts).
Their contribution to the original vector's sorting in mixed large buff groups
is outside the alcohol comparison. Full original Player::UpdateBuffs/GameTick
parity, reactive food/combat hooks, full floor generation/population/transitions,
AI/death scheduling, remaining effects/statuses, original UI/audio/FOV and a
complete reference playthrough are still unfinished. This remains a room-session
integration; those named comparisons do not establish a complete game.


Alcohol status sourcedac9ae7 passed actual arm64 macos-15 run42737308111,
job129569575492: all62 checks and relocated packaged-app smoke. Native meal use,
alcohol tier upgrades/downgrade/removal, recovery and save/reload all passed.
All37 local groups pass; the implementation branch is verified at this source
commit. This final entry records validation only; it does not establish complete
original-game parity. Next major integration work remains full-floor population/
transitions, reactive effect/AI/combat/death scheduling and original UI/audio/FOV.


Prepared-floor travel is now integrated through FloorRegistry/SessionFloor (see
FLOORS.md). Stair plans preserve numeric tile link byte2, x-major/descending-y
BuildStairs order, first-floor up removal, final-floor down suppression (depth9
or14 with the campaign flag), and live wall writes above stairs. Cached arrival
searches preserve x-major/ascending-y exact-link preference, upward fallback and
depth-only level lookup despite branch mismatch. Stairs::SteppedOn depth arithmetic
wraps32bits. verify_stairs.py matches1024 actual BuildStairs192bd6,1024 actual
SteppedOnb92fc dispatches and1024 actual Ascend2b210/Descend2ac50 cached searches.
Constructor/placement, autosave/load/regeneration, render/pathfinding/UI boundaries
are supplied; missing downward links stop before original new-level construction.
Full generation, new-level fallback and on-demand/off-floor regeneration excluded.
Prepared floors now preserve enemies, loot, custom objects, breakables, traps,
stairs and visit clocks across round trips. Character/inventory/equipment/buffs/
triggers/cooldowns/progression and both RNG streams remain global. Item identity
reservations and actor sequences prevent collisions with inactive floors; cross-
floor duplicate identities reject. Enemy-cast player triggers cannot yet cross a
floor boundary and reject atomically; player-owned pending spells can travel.
Cache updates/travel/load are transactional. Save17 stores floor stairs/branch and
item reservation while retaining prior save capabilities. Run format1 stores the
active session plus cached floor payloads, with the active session authoritative
for character state. Readers validate all nested floor payloads and identity
reservations; inactive visit clocks survive serialization. Old session formats
remain supported. Native prepared-floor scenes load/render actual stair SPRs,
travel on movement without bouncing on arrival, switch dimensions/images/HUD,
and save/load whole runs. Assets for inactive floors validate before committing
a run load, so a bad cached sprite cannot partially replace live state.
All38 local groups pass, including paired travel, merged inventory/ID reservations,
active buffs, pending player spells, cooldowns, cached NPC/loot state, whole-run
reloads and invalid/pending-caster rollback. The new native floor fixture descends,
returns for currency, descends again, checks cached NPC state and rejects a bad
inactive-floor asset atomically. Source awaits actual arm64 MacCI.
This is prepared-floor cache integration, not complete generated-floor play.
Inactive floors currently freeze; original OnDemandLoadLevelWithRegen still needs
recovery. Session AI/tick/arrival-obstruction handling remain provisional. Full
ordered room content/decorations/shops/scripts, secret doors, random stair-site
placement, remaining population/AI/effects, original UI/audio/FOV and a reference
playthrough are still unfinished. GenLevelContent1's confirmed phase order is
BuildDoors -> WriteDungeonContentFromRoomRecords (starts PunchOut) -> safe teleports
-> secret doors -> random stair sites -> BuildStairs -> later random population.
Do not reorder those phases or claim the isolated comparison covers the whole run.


Prepared-floor sourcef6b9746 passed actual arm64 macos-15 run42953577281,
job130303307282: all65 checks plus relocated packaged-app smoke. Native stair
rendering, dimension/image swaps, round-trip currency/NPC state, whole-run saves
and invalid inactive-asset rollback all passed. All38 local test groups pass.
Correction to the preceding off-floor-regeneration note: actual OnDemandLoadLevel
864c0 returns immediately when the requested level is already resident. Its
OnDemandLoadLevelWithRegen86b00/86a10 wrapper calls DungeonRearrangeCleanup only
when disk loading fails (after an optional alternate-save-file retry). "Regen"
there means rebuilding an unreadable/missing level, not recovering inactive
monsters' HP. No inactive-level ticking/healing was found in that loader path.
The return-to-level loop's monster virtual114 is RebuildSpritesec494, rather than
a stat update. Full disk serialization/unload/recovery remains unimplemented.
The stairs oracle now executes the actual load wrapper and resident fast path
instead of stubbing them; all3072 comparisons still pass. This strengthens the
resident-cache comparison but does not establish full original floor loading or
GameTick parity. The earlier implication that a missing off-floor HP-regeneration
mechanic had been established was an incorrect interpretation of a function name.
The implementation remains incomplete: generated-floor content/decorations/shops/
scripts and secret doors, random stair-site placement, remaining population/AI/
spell interactions, original UI/audio/FOV and end-to-end original validation are
still outstanding. Prepared-floor travel is now verified, not a complete game.
