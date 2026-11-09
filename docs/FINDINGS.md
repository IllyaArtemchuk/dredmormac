# Findings

## Supplied material (2026-11-09)

The initial attachment exceeded the executor's 32 MiB transfer limit. The user
provided a Drive link; the authenticated Drive fetch returned a downloadable
file reference. Downloaded 246,307,989 bytes and verified every ZIP member CRC.
No private URL is stored in this repository.

- Archive SHA256: `cce6ecd0e62d23de6795acf28e6edfe21d209bb0f04604e008e44994e528ba3b`.
- ZIP entries: 25,558, including directories and AppleDouble metadata.
- Extracted regular files: 9,316, excluding `__MACOSX` and eight framework symlinks.
  The original archive is retained intact. Extracted copy is for analysis; it is
  not a launchable reconstruction of the bundle's links/permissions.
- Formats: 7,732 PNG, 698 SPR, 393 XML, 318 WAV, 43 OGG, 70 PAL,
  three TTF, one DAT, plus libraries, headers and bundle metadata.
- Main executable: `Dungeons of Dredmor.app/Contents/MacOS/dredmor`.
- Executable SHA256: `a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
- Architecture: thin i386 Mach-O; 27,235 symbol-table entries including duplicates.
- Eight Mach-O files: executable, five dylibs, SDL and SDL_ttf frameworks. Some libraries also have
  x86_64 slices. The game executable itself has no arm64 or x86_64 slice.
- Dependencies include Cocoa, Carbon, OpenGL, OpenAL, SDL, SDL_ttf, expat,
  zlib, libstdc++, libSystem, Objective-C and Steam API.
- Base data and `expansion`, `expansion2`, `expansion3` directories are present.
  Presence does not establish expansion completeness or gameplay scope.
- Info.plist reports CFBundleVersion `1.0`, which is not the patch version.
  The executable's startup version formatter is **1.1.4**: the format string
  at `0x21de18` is referenced at `0xb6abc` in the function starting `0xb69b0`.
  Immediately before the call at `0xb6add`, cdecl stack arguments at offsets
  4, 8 and 12 are populated with 1, 1 and 4 respectively. Confidence: high
  for the embedded patch version, based on static instruction analysis; not
  a captured full-game startup. Additional suffix/mod state remains unverified.

## Verified original routine

`__Z8distanceiiii` at virtual address `0x000f0360`, returning at `0x000f0384`.
The named function subtracts x and y coordinates, squares each with wrapping
32-bit arithmetic, sums with wrapping arithmetic, converts the signed sum to
single precision, takes a single-precision square root, and truncates to int.
Invalid float-to-int conversions produce INT32_MIN on the original SSE path.

`scripts/verify_distance.py` loads the user's actual binary segments in Unicorn
2.1.4, supplies cdecl arguments and a stack, and executes those instructions
without imported-function stubs. It compares returns against the compiled C++
implementation. Fixed seed 0xD0ED; eight directed cases, 4,096 normal randomized
cases, and 4,096 full-int32 randomized cases: **8,200 cases, zero mismatches**.
Confidence: high for this function, under default floating-point rounding.
This is isolated execution of the original routine, not original gameplay.
Raw measurements remain outside source control in analysis/distance.json.

## Room/asset evidence

The supplied base `game/rooms.xml` parses with Python ElementTree and contains
191 room definitions. The first is Starting Room, 9 by 16 cells. It includes
custom blockers, conditional decorations and vending machines. The importer
accepts only a limited symbol set, rejects unknown symbols, and preserves visual
rows independently from a provisional collision grid. It is not a full database
implementation.

`hero_idle_d.spr` has a 9-byte SPR header, one 64x64 frame, a two-byte delay,
768-byte RGB palette and 4,096 indices; total 4,875 bytes. The decoder exports
the first frame, assuming palette index zero is transparent. Trailer-bearing
files are rejected. This assumption is not yet checked against original rendering.
The basic tileset is 384x640; preview crops are provisional.

## Tools and public projects

- GameFrame checkout: `c30e3fc95c748a8ec44e3989faafc4220895cbab`.
  Read its AGENTS.md, re-setup skill, environment guide and setup script.
  Ran setup check and install; created external home and Dredmor project using
  documented commands. Integrated through scripts/gameframe.sh, without copying
  its generated project or game files into this repository.
- Ghidra 12.1.4 archive downloaded and SHA256 checked against GitHub release
  digest `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`.
  Python ZIP extraction did not preserve executable modes; relevant modes fixed.
  Initially could not start because the host Java installation lacks javac.
  Download attempts failed (package indexes unavailable, Adoptium API 403,
  GitHub release repeated 504). Subsequent inspection found jdk.compiler already
  present in the runtime's module image. A local home/launcher invokes the real
  compiler module; javac 21.0.13 works and Ghidra successfully imported, analyzed
  and saved the supplied Mach-O. Reproduced with java-compiler-home.py and
  analyze-original.sh. Missing external macOS libraries produce import warnings;
  targeted decompilation works. An embedded-data PNG diagnostic did not prevent
  analysis and export. Toolchain is now usable.
- GameFrame `tests/run.sh --ghidra`: **407 passed, 28 failed**. Failures include
  absent xxd, image/catalog dependency, Frida export lookup and Ghidra/JDK.
  Re-ran after compiler repair: **424 passed, 11 failed**, including successful
  Ghidra imports, decompilation, labels and references. Remaining failures concern
  xxd/installer extraction, Frida lookup, image catalog/rawimage and fill-gaps.
  These are not passing checks and are not current project blockers. The
  repository's own checks pass. REA not configured.
- Open Dredmor checkout: `58cc5fd9342993bfad57d07a409e1158601b52f4`.
  Godot/GDScript engine, GPLv3 license text. Requires an original installation.
  Reviewed its project structure and asset/room handling as public reference.
  No code or assets copied from it; no license selected for this repository yet.

## Execution strategy

Linux cannot natively launch this macOS application. Isolated i386 routines can
be measured in Unicorn where inputs, memory and dependencies can be established.
Full gameplay reference needs a suitable legacy Mac environment or another
user-supplied supported edition. Modern Apple Silicon cannot directly launch
the supplied 32-bit executable. The new AppKit target needs a real Mac with
Xcode tools for compilation, packaging and actual smoke testing.

## Verified RNG and character calculations

Recovered the MT19937 interfaces: init_genrand at 0x190800, genrand_int32 at
0x190870, init_by_array at 0x190a00, res53 at 0x190b20, real3 at 0x190eb0,
real2 at 0x191070, real1 at 0x191230 and int31 at 0x1913f0. The fixed-width
replacement matches **28,550 original outputs**, covering the implicit default
seed, six scalar seeds, four array seeds (including a 625-element key), twist
boundaries and mixed integer/floating calls. Double returns are stored by a
harness-generated x87 thunk and compared by all 64 bits. Output SHA256:
`572bce3a419ccc13830e32e7e2cf1827ca60a22954fe87c757cb7af85b7638ba`.

An initial oracle mismatch was traced to rewriting a cached emulator call site.
Each floating routine now has its own immutable thunk. A regression test uses
our own synthetic Mach-O/x87 functions, with no commercial bytes, to check
alternating call targets. No gameplay fix was accepted based on the bad oracle.

Player::RebuildAttributes is at 0x1218de. The original fragment [0x121f52,
0x12218f) maps six primary values into 24 secondary fields, applies flat and
percentage modifiers and clamps selected negatives. **2,568 original fragment
executions / 61,632 fields match**. Cases include normal, boundary and full-int32
inputs; integer wrapping and single-precision roundtrips are preserved.

Also executed the entire original Player::RebuildAttributes, without import
stubs, for **1,027 controlled fixtures / 30,810 fields**. Supplied seven selected
skills, class/rank metadata, primary/secondary flat modifiers and percentages;
ability database, equipment and buffs were empty, and polymorph was off. Six
primary and 24 secondary outputs all match. This verifies the class-rank stat
pipeline for that scope, not abilities, equipment or buff aggregation.

The native Expat loader reads all **40 supplied XML databases / 4,882 records**,
including nested monster variants. An independent ElementTree pass agrees with
all document roots, top-level counts and per-kind counts. Base only: **17
databases / 3,565 records**, including narrative text. Sources, duplicate names,
attributes, ordering, content layers and variant ancestry are retained. No
unverified expansion override/inheritance policy is applied.

Combat routines at 0xc93d0, 0xc9490 and 0xcea00 call imported libc rand(), not
only genrand_int32. Their probability divisions use constants 0x147ae15 and
0x28f5c29. Recover libc seed/draw semantics and call order before claiming
deterministic combat. Combat rules have been decompiled, but are not yet
implemented or verified as a complete transition.

## Combat, inventory and wearable-slot recovery

DoRandomize at 0xa2a60 seeds libc srand and MT19937 from SDL_GetTicks. Wizard
key seeding at 0x1b1230 seeds both only when decoded key value is nonzero.
DarwinRandom implements the mathematical Park–Miller stream described in Apple's
published Libc rand.c (apple-oss-distributions/Libc, stdlib/FreeBSD/rand.c).
Its agreement with the actual Mac libc remains pending; a Mac-only CTest compares
36,864 actual libc outputs. Linux libc is deliberately not substituted.

DoCombatDamage at 0xcea00: **4,101 original decision/arithmetic paths match**
the replacement, with fixed rand return tapes supplied at imported boundary
0x1cbf98. Probes stop before UI, recursive counterattack or HP delivery. Tests
cover decision precedence, critical suppression of dodge/counter/block, random
draw counts, physical/elemental reductions and no-effect cases. This is not yet
a comparison of the complete combat transition, triggers, fumbles or HP mutation.
Critical doubles melee power, not all weapon damage; block divides physical
damage by four and elemental damage by two. Original damage-type name order was
read from _damageNames at 0x2602a0 and is represented in combat.hpp.

PushItemToInventory at 0x49960: **3,840 original placement/stack operations
match all 57 slots after each operation**. Original placement fragment starts
at 0x499a7 after collection bookkeeping and stops before destruction, teleport,
recipe or full-inventory-message effects. Bag is 8x6 (48 slots), belt is nine
slots. Existing belt stacks are checked first unless belt use is disabled,
then bag stacks, then eligible empty belt slots, then empty bag slots.

Both original CanEquipOnAnatomy functions were executed without import stubs:
**684 decisions match**, including invalid types/slots. Typed item loading reads
all **751 supplied item definitions**. The original XML callback uses ASCII
case-insensitive tag/attribute comparisons and last-assignment wins for repeated
primary/secondary buff IDs. The typed reader preserves these rules; unsupported
effects remain in each definition's source tree. Integer syntax is stricter than
original atoi for malformed input; supplied definitions parse successfully.

## Monster scaling, progression and integration session

Monster::RebuildAttributes at 0xe42ac matches **2,048 full original executions /
61,440 fields**, supplying only tweakVal/tweakValf configuration read boundaries.
Tests cover class ranks, definition and live modifiers, percentage scaling and
HP scaling at nonpositive/positive dungeon levels. Floating-point contraction is
disabled so separate original single-precision operations remain separate.
Nested monster definitions clone parent fields before sparse child overrides;
the original callback at 0xdca60 confirms that copy behavior. Typed loading keeps
the source ancestry for effects not yet executed.

The original tweak callback at 0x179870 maintains independent integer and float
maps for each difficulty, copying global entries to all three maps. Missing ival
or fval initializes its corresponding value to zero. Monster HP's flat-per-level
read uses tweakVal even though that supplied setting uses fval; no automatic
float-to-int fallback is applied by the replacement.

FillXPTable at 0x116d10 matches **350 original thresholds** for seven values of
experience n (all 50 entries each). Threshold i is n*(i*(i+1)/2+1), with original
32-bit wrapping. GiveExperience at 0x116c60 uses strict greater-than, resets
current-level XP after allocation, and records score XP at four times the award.
AllocatePoints instructions increment level and unspent points and refill HP/MP.
Only threshold generation has an execution comparison; complete level-up/UI
side effects have not been executed as a reference transition.

Session connects movement, measured combat decisions, provisional cardinal BFS
enemy turns, health/death, pickup, equipment transfers and XP. Tests exercise
encounters, chase order, death, inventory pickup and atomic failed actions. The
session is an integration model, not verified full original turn scheduling/AI.
It currently permits one melee weapon; dual-wield, fumbles, buffs and spell
triggers remain pending. Equipment metadata/modifiers work without artifacts or
encrusting; complete aggregate parity is not yet claimed.

Versioned session XML saves room/player location and turn, actor stats, inventory,
equipment, ground items, progression and both complete RNG states. Tests compare
exact serialized state and subsequent randomized turns after reload, including
dead players, equipped items, escaping, content mismatch and malformed saves.
This is a new port save format, **not original .sav compatibility**. Its content
signature detects changed item definitions; it is not an authentication mechanism.

## Actual arm64 macOS validation

Published source-only implementation branch and ran GitHub Actions on the
standard macos-15 arm64 image. Run 42581482510, commit e20faf7a1e3cccd586d6499533314dbb440441be:
https://github.com/IllyaArtemchuk/dredmormac/actions/runs/42581482510

AppleClang 17.0.0.17000013 compiled the core and AppKit application; file identified
the app as Mach-O 64-bit arm64. **All nine checks passed**, including session,
save/load, real Darwin libc RNG comparison (36,864 outputs), and actual native
window/draw/move/wait smoke. Original assets were not uploaded or used by CI.
This clears the previous native-compilation blocker for the tested foundation,
not complete-game or original-asset visual fidelity validation.

Ghidra initially truncated many functions at a falsely noreturn allocator
destructor import thunk. RepairAllocatorReturns.java clears that annotation and
call-flow overrides before export. Re-running general analysis immediately
rediscovers it, so export directly after repair. Complete Tileset::Load,
Level::Draw and Player::AllocatePoints exports are now available privately.

Run 42583263923, commit c0816a61d8c4b377c6985b409d963f24e51fe135, passed
all twelve tests, including native combat input/drawing. Packaging relocates Expat
into Contents/Frameworks and rewrites its load path, then ad-hoc signs the nested
library and application. Strict signature verification and packaged app smoke
passed. Development artifact contains no original files; this is not a full game.

## SPR asset decoding

All 698 supplied SPR files have a nine-byte header (SPR, little-endian u16 frame
count/width/height), followed by a u16 delay, 256 RGB triples and width*height
indices per frame. No supplied files have trailers. There are 3,633 frames, 542
multiframe files, maximum dimensions 128x160, and maximum decoded image payload
786,432 bytes. All multiframe delays are positive. The C++ decoder bounds file
and decoded sizes and preserves every palette/frame. Independent Python expansion
matches dimensions, frame counts, delays and RGBA fingerprints for every file.
Index zero is assumed transparent; original visual comparison remains pending.
Native rendering selects frames with a presentation clock independent of session
turn/RNG state. This timing policy is provisional, not original engine parity.
Reproduce using scripts/compare-sprites.py DATA_ROOT --report PRIVATE_REPORT.

Run 42584845873 (f750d1b) passed all thirteen checks and relocated package smoke
with synthetic multiframe SPR assets in both movement and combat windows. Native
Save/Load actions now call the session serializer, use atomic file writes, and
validate the room plus all actor image resources before swapping loaded state.
New native smoke saves, advances a turn, reloads, compares full serialized state,
and checks that malformed input leaves the state unchanged. This new check
awaits native execution; original saves and cross-room rendering remain absent.

Run 42585141286 (4fd4747) passed all thirteen checks and relocated app smoke,
including exact native save/reload restoration and malformed-save atomicity.
The macOS 12-deprecated file extension filter is being replaced with UTType.

## Dungeon room placement

Level::ScanRoomForTemplate at 0x192500 runs without imported calls in the original
binary. The C++ numeric-grid kernel matches 69,632 executions: all 65,536 byte
pairs plus 4,096 seeded footprint/boundary cases. Positive template dimensions
are required by the port. Every footprint cell, including void, must lie within
the level's three-cell reserved margin. Void overlaps anything; nonzero cells
require existing void except wall 2 may overlap wall 2 and door 6 may overlap
door 6. Tests include asymmetric footprints and extreme port coordinates.
This establishes the collision kernel only, not template parsing, writing,
selection, RNG consumption or seeded full-level generation. Original report:
room-placement.json, retained privately; scripts/verify_generation.py reproduces.

WriteTemplate cell loop [0x193cc0,0x194022) matches 1,024 original executions,
20,825 template cells and 1,650 appended doors. No imported calls are replaced;
original MT draws run and the next eight draws are compared per fixture. All
cell fields touched by the loop plus untouched bytes are checked. Fixtures vary
branch IDs, incoming/existing types, room IDs, flags, shops/zoos and wrapping
floor counts. This excludes subsequent strings/names, placed-room records and
room counters. scripts/verify_stamp.py reproduces room-stamping.json privately.

Numeric room ingestion now respects declared dimensions and clips rows to width.
It retains separate kind/orientation/wall-marker arrays. Constructor at 0x19cedc
zeroes all arrays. The original row callback records digit locations without
changing that zero cell. PointProperty::SetFrom converts referenced markers to
floor during XML loading; the earlier assumption that this waits for population
was incorrect and is corrected below.
This differs from the conservative developer collision preview. All 346 supplied
base/expansion templates match an independent XML-to-numeric expansion. That is
an ingestion check of statically recovered mapping, not original parser execution
or full script/population parity. Source trees preserve remaining annotations.

Run 42585590201 (fa64f9d) passed all fourteen native/portable checks plus relocated
package smoke. UTType file filtering compiles without the earlier deprecation
warnings. Original assets remain outside GitHub and test artifacts.

Run 42586474951 (6cecbb3) passed all fourteen checks and packaged app smoke.
Direct installation startup now builds the conservative Starting Room scene from
GameContent in process. No arguments shows an AppKit installation picker; .app,
data directory and prepared JSON are accepted. Failed interactive loads show an
error and permit another selection. Original data is read in place, not copied.
The synthetic installation startup fixture is pending native execution.

Run 42586734212 (941ecac) passed all sixteen checks and packaged app smoke,
including direct installation startup from synthetic XML/SPR/PNG content.
Native inventory controller exposes bag/belt/equipped items, target slot and
Equip/Unequip callbacks to the existing core. G picks up at the player position;
I opens the inventory. Explicit ground_items in developer scenes use typed item
definitions and remain separate from original room scripts/population. A new
native synthetic-window check drives pickup/table selection/equip/unequip and
then full save/reload. Its actual Mac execution is pending.

Run 42587414308 (0c082aa) passed all eighteen checks and relocated package smoke,
including synthetic native pickup, inventory table selection, equipment transfer
buttons, restored stats, and save/reload. The prior inventory controller compile
error was a collision with inherited NSWindowController.owner; renamed the weak
callback property inventoryOwner and verified the fix on the actual Mac.

## Starting item selection

GetRandomEdible (0x4e1f0) and GetRandomBooze (0x4ecb0) match 2,048 original
executions with a supplied, separately verified Darwin rand boundary. Compare
chosen entry and complete final libc RNG state. Null current level sets booze
target level one; box-muller target selection and food-table registration remain
separate. The first booze search result is discarded; the second search allows
special items and returns its 101st sample even when no eligibility condition
passes. Tests include 256 target-missing fallback cases, up to 127 total draws.
Edible excludes positive MP, special and debug; booze first excludes zero MP,
special and debug. The second booze loop does not check special. Craft-output
is a different field and is not substituted for debug. Invalid pools with no
eligible first-stage candidate throw before consuming RNG rather than hang.

BuildPlayerFromSkills loadout gate [0x11ba10,0x11ba99) matches 4,096 original
fragment executions, without import replacements. A selected skill contributes
its loadout when its class is tied for largest rank or always is nonzero. Typed
loadouts preserve type/subtype/amount (default one)/always (default false) and
original database ordering. GameContent::starting_loadout returns this filtered
plan; it does not yet instantiate/grant items, auto-equip, add default clothes,
roll four random food entries, or grant abilities. Original scripts reproduce
food-selection.json and starting-loadout.json privately.

## Player passive abilities

Typed ability ingestion preserves the complete source and reads stat/damage/resist
modifiers with case-insensitive tags/attributes and last assignment per ID.
Player::RebuildAttributes (0x1218de) matches 1,024 full-function fixtures with
4,670 ability records and 63,488 fields (primary, secondary, damage, resistance).
Only ASCII std::string::compare is supplied at its imported boundary; original
ability gating and arithmetic execute. Equipment, buffs and polymorph are empty.
Ability::IsMeleeAbility (0x146482) and base passive gating are recovered: unarmed
requires empty weapon hands; Shield Bearer needs hand armour; melee families,
Thrown Weaponry and Archery are excluded from this base pass and handled elsewhere.
Passive primary additions occur after base primary scaling/clamping, then feed
secondary derivation. Secondary/damage/resist additions follow base secondary
scaling/clamping, so negative final secondary bonuses are preserved. Weapon-specific
ability rebuilding, triggers, active spells and polymorph gameplay remain pending.

Session character_profile initializes seven skills and immutable ability content
before any turns/spawning; initial HP/MP use the resulting maxima. Equipment
changes recompute base passive eligibility. Tests switch unarmed bonuses off/on
through weapon transfers. Port save version two stores skills and an ability
content fingerprint, supports version-one port saves, and rejects missing/changed
ability content and duplicate skills. Full serialized state and future equipment
rebuilds agree after reload. Native integration smoke is awaiting the next push.

Run 42589757941 (725e708) passed all eighteen tests and relocated app smoke with
passive ability metadata in the synthetic installation, inventory and version-two
save/reload path. New native skill selection offers eligible content skills,
requires exactly seven, shows class counts, and passes selected IDs into the
session profile. Deprecated entries are retained in content but hidden by the
picker. Prepared JSON keeps its explicit/default developer profile. New synthetic
native checks click skill checkboxes and verify selection count/order plus Begin
button enable/disable. Native execution of that picker check is pending.

Run 42590387752 (261d1fb) passed all eighteen checks and relocated package smoke,
including actual native skill checkbox interaction and Begin enablement.

## Trap/toolkit type correction and port-save migration

Further constructor recovery exposed an earlier metadata error: Toolkit::Toolkit
at 0x1847da sets runtime kind 11; Trap::Trap at 0x108936 sets kind 12. Database
registration and CreateItem dispatch agree. The port enum labels were reversed.
The numeric belt-placement comparison had still passed because it selected kind
12; the correctly named eligibility rule is now trap, not toolkit. Corrected the
metadata mapping and re-ran all 3,840 original inventory operations / 57 slots,
with zero mismatches. Typed tests now check trap-to-belt and toolkit-to-bag.

Port save version three writes corrected type IDs. Version one/two readers use
the old item-content fingerprint and translate legacy 11/12 IDs before validating
against current definitions, retaining instance IDs, quantities and locations.
An entirely synthetic v2 save was generated with the pre-correction serializer
and retained as tests/fixtures/port-v2-item-types.xml; tests load/migrate it and
roundtrip the resulting v3 state. No original save files or original assets are
in that fixture. Actual native validation of this migration awaits the next run.

Run 42591221995 (76381cd) passed all eighteen checks and relocated package smoke,
including old-v2 item type migration, current skill/passive saves and native UI.

## Basic item instances and wand charges

Constructor recovery establishes stackable food/potions/gems/mushrooms/reagents/
toolkits/traps, nonstackable armour/wands, and stackable weapon types five/six.
Trap::Build (0x10854e) sets stackability after Trap's constructor calls it; it must
not be inferred from the constructor's initial base flag alone. Basic factory
creates these instance properties separately from rendering/artifacts/effects.
Misc types zero/one are built-in Lockpick/Spores with known original icon paths.
GameContent appends two engine definitions after external items, preserving IDs;
external-content fingerprint excludes engine builtins so earlier saves still load.
The original 751 supplied definitions remain distinct from those two additions.

Wand charge loop [0x7af5,0x7b43) matches 1,045 original calculations and final
Darwin RNG states with the imported rand boundary supplied. maxcharge is exclusive;
equal bounds consume no draw, while a one-wide interval still consumes a draw.
Normal ordered nonnegative ranges plus boundary cases up to INT_MAX are covered.
Optional per-instance charges persist in port saves, including zero/depleted wands;
older saves without this attribute retain an unspecified charge state. Native
inventory displays known charges. This does not implement wand casting or effects.

Run 42592841136 (ecbb2cb) passed all eighteen native/portable checks and relocated
package smoke with basic instance fields, builtins and optional wand charges.

## Starting character integration

create_character now applies filtered loadouts in database order, basic instance
creation, automatic initial weapon/armour choices, builtins, default class clothes
and two booze/edible pairs. Original code grants an amount>1 weapon stack and then
one additional weapon, so ammunition may receive the extra unit; misc entries each
give one builtin. Starting weapon handling precedes other named/random loadouts.
Armour checks preferred anatomy in order 2,5,0,7,6,3,10,11,9 and leaves occupied
slots intact. Default clothing prioritizes warrior ties, then wizard ties, then
rogue; an already granted head item suppresses the wizard robe. Constructor ends
with attribute rebuild and HP/MP refill. Tutorial behavior and active starting
ability hotbars are not implemented. Weapon-family ability rebuilding remains
pending, so full character stat parity with equipped weapons is not yet claimed.

Source-driven basic random loadouts use ordered mushroom/potion/wand pools and
shared alchemical reagent/component entries. These include the supplied base
loadouts; unsupported empty pools fail explicitly. Full original table registration,
expansion merge semantics and whole BuildPlayerFromSkills execution comparison
remain pending. Existing individual gate/food/wand/inventory comparisons still
apply; do not describe their composition as a complete reference trace.

New synthetic character tests cover exact starting quantities, automatic slots,
class clothing paths, builtins, charges, resource fill, skills, complete save/load
and atomic failed ground creation. A supplied-content batch initialized 256
characters covering all 36 base skill IDs with no failures; this is integration
validation, not original-game comparison. check_starting_content.py reproduces
starting-character-integration.json privately. CLI supports explicit --skills and
--seed. Normal native installation startup enables starting grants and seeds both
streams with elapsed application milliseconds before skill selection; the exact
original external startup timing is not claimed. Developer scenes retain fixed
seed/default-empty loadouts unless explicitly configured.

Run 42594615983 (801b4b8) passed all nineteen checks and relocated app smoke,
including direct installation startup with selected starting inventory/equipment.

## Equipped weapon training

Ghidra missed two computed-jump tables in Weapon::EquipOnPlayer. Explicit computed
jump references alone were insufficient; JumpTable.writeOverride for both switches
restores the visible cases. RepairWeaponSwitch.java validates all destinations,
updates analysis only, and is integrated into analyze-original.sh. Original bytes
remain unchanged. Weapon type to skill-ID mapping is 0,1,2,3,4,5,4,45,46 for
weapon types zero through eight, with fallback skill ID two.

Weapon training primary/secondary modifiers enter flat buffers before base
primary/secondary scaling and clamping. General passive primary modifiers enter
later. Melee weapon-family damage joins the melee skill matrix; bow-family damage
belongs to the ranged matrix and must not leak into melee. Resistances combine
for equipped melee/ranged weapons. Corrected the earlier unconditional addition
of bow item damage bonuses to the melee integration profile.

Actual original Weapon::EquipOnPlayer (0x1397c0), including its virtual call to
actual Player::RebuildAttributes (0x1218de), matches 1,024 cases / 63,488 fields.
This includes 235 melee-plus-bow cases, 4,528 ability records and 1,259 equip calls.
Only ASCII library string comparison is supplied and UpdateSpellsForEquipment is
skipped at its boundary; no buffs/artifacts/dual wield/polymorph. Compare primary,
secondary, effective melee bonus damage and resistance. Tests verify bonus order
and that bow damage cannot augment melee. Session still permits at most one melee
weapon; complete dual-wield transitions and spell/trigger updates remain pending.
Re-run verify_equipped_passives.py for equipped-passive-stats.json privately.
The 256 supplied-content character initializations pass again after this change.

Run 42596610606 (17689fb) passed all nineteen checks and relocated package smoke
with weapon training integrated into normal character creation and save/reload.

## Room rule ingestion correction

Original room parser flags are child <flags> attributes, not room-root attributes.
Corrected numeric_template shop/zoo and added level bounds, room-set, quest,
unique/special, diggle-god and population-suppression rules. Previous independent
numeric-template checking shared the root-attribute assumption; it did not prove
those semantics. The corrected independent parser now compares all rule fields
for all 346 supplied templates. Base has 191 templates, six shops and three zoos,
all room-set zero. Source annotations remain preserved for later population.

A supplied expansion-three loadout contains amount=" 3". Original atoi accepts
it; the new strict skill-loadout reader had rejected it and incidentally blocked
unrelated template checks through GameContent. Added an ASCII whitespace/sign/
decimal-prefix reader for original room/loadout integer attributes. Port save and
CLI integer validation remain strict. Out-of-range source values saturate to
int32 as a stated port policy; original overflow has not been executed. All ten
local groups and corrected 346-template comparison pass.

Run 42597407998 (f76d6d0) passed all nineteen checks and relocated package smoke
with corrected child room flags and whitespace-compatible loadout numbers.

## Room names and generator RNG

Original inline MT range mapping [0x195f1a,0x195f4f) matches 1,080 cases / 9,720
outputs, including the next eight raw draws. No imports are supplied. Mapping is
division/rejection by floor(UINT_MAX/count), not modulo. Counts outside
1..0x7ffffffe return zero without a draw; count one still draws/rejects UINT_MAX.
Room stamping now reuses that helper and still matches all 1,024 original loops.

GenRoomName (0xd3670) matches 1,028 full-function fixtures, comparing name,
subject, libc state and next eight MT outputs. Only string storage/operations and
the separately verified Darwin rand boundary are supplied. Three MT coin flips
are always consumed: optional adjective, singular/plural architecture, and suffix.
A suffix is forced when no adjective was selected, but the third coin still draws.
Word picks consume the separate libc stream, including one-entry pools.

The text XML callback (0xd0a20) matches 1,024 synthetic adjective/architecture/noun
nodes with only ASCII/string library boundaries and preallocated vectors. This
confirms a loader bug: architecturePluralDB receives the singular text despite
reading the plural attribute. The replacement preserves it. Other text node types
and painting/statue/floor side effects are outside that comparison. GameContent
loads the base name pools; room fixed-name overrides are retained and independently
checked with all 346 templates. The fixed-name path will bypass random naming in
generation. Eleven local test groups pass. Native validation awaits the next run.

Run 42599302441 (b56f322) passed twenty native checks and package smoke.

## Complete geometry control flow before door objects

The independent execution harness now runs original GenLevelContent1 (0x195bac)
until BuildDoors entry (0x192d6e), including actual placement, stamp, vector-growth
helpers and name generation. Allocation (controlled zero-filled heap), strings,
Darwin rand, branch configuration and branch discovery are supplied boundaries.
It compares all numeric cells/room IDs/door and wall flags, ordered room records,
adjacency, display names, depth-zero spawn, libc state and eight subsequent MT
outputs. 256 synthetic cases / 339 attempts (43 rerolled cases), and 64 supplied
base-template cases / 68 attempts (four rerolled), have zero mismatches across
depths 0,1,3,6,8,9. Both short and normal generation thresholds are exercised.
The supplied-template run uses independent XML-to-array conversion and the base
word tables, not original XML registration. No assets are emitted or committed.

Two observations materially corrected the first model: reset clears flag bits
0..6 but preserves special-wall bit 0x80 across attempts, and automatic depth>0
starters require at least three numeric door cells. The latter is visible in
instructions 0x196300..0x196316 and real-content fixtures; the current decompiler
output discarded the door count. Named branch starters bypass that requirement.
FIFO candidate selection counts only eligible attempts, searches candidate doors
row-major and appends newly stamped doors x-major. Failed heads become walls.
Room naming follows each stamp's door RNG draws. Shop/unique/zoo/god limits and
fixed-name RNG bypasses are covered. Water/lava/ice conversion depends on depth,
not branch ID; the earlier stamp parameter name has been corrected.

Geometry is a separate numeric model, not yet a Session World. Active template
registration/branch starter resolution remain input boundaries; subsequent door
objects, scripts, stairs, secret doors and population are not implemented here.
Explicit candidate/room/retry budgets are port resource policy and fail without
committing caller RNG state. Twelve local groups include original golden layouts,
retry persistence, reciprocal graph constraints and atomic guard failures.

Run 42601954130 (eda6db8) passed all twenty-one native arm64 checks and relocated
signed package smoke, including the new geometry goldens.

## Door assembly plans

build_door_plan reproduces BuildDoors (0x192d6e) traversal and repair rule before
entity side effects: x ascending, y descending from height-2 through 1. Horizontal
door markers with ordinary floor exactly two cells to each side become floor;
shop/zoo/liquid cells do not satisfy that test. Other markers create three linked
parts, with orientation, lock and initial-open bits from door flags. The plan
retains constructor/placement order and the three part positions.

verify_doors.py matches 1,024 original executions / 32,244 assemblies / 521 repairs.
Allocation, Door constructors, virtual placement, Open and printf are recording
boundaries. Grid mutation, constructor arguments, lock state on each object,
previous/next links, placement coordinates/order and opening calls are checked.
Actual constructor/sprite/place/open side effects and player interactions are
excluded. Malformed port layouts reject invalid part positions atomically; the
original assumes generator margins. Twelve local groups pass with repair, ignored
boundary rows, orientation, lock/open and atomic-failure coverage.

Run 42602496973 (582b200) passed native validation and package smoke.

## Door toggle and remote activation

ToggleDoor (0x109d1e) and RemoteActivation (0x108e7e) match 1,408 full original
executions with supplied occupancy, string, sound and ticker boundaries. Includes
the exhaustive 384 combinations of orientation/open/lock/selected part/remote/
obstruction mask, plus 1,024 random cases and independent flag bytes. Compared
handled returns, all three parts' open/collision state, all cell door flag bits,
effect kind and ordered occupancy queries. Outcomes: 549 opened, 89 closed,
265 obstructed, 155 ignored center requests, 350 locked remote activations.

The raw toggle ignores locks; RemoteActivation checks the lock before toggling.
An open center part returns zero without closing or checking obstructions.
End-cap closing checks only the selected part and its immediate neighbor, not
the far end; preserving this is required to match the original. Obstructed close
returns one and emits a ticker message without changing state. The port exposes
outcome/handled values; their integration with turn scheduling remains pending.
Closed/open transitions clear/set flag 0x04 on all three cells, retaining other
bits and lock state. Animation/render fields and Clicked's lockpick/kick paths
are excluded. Local tests cover these distinctions and atomic invalid queries.

## Content punch-out geometry

PunchOutDungeonContent (0x19263c) matches 2,048 full original executions /
404,862 cells without imports or stubs. Traversal is x-major/y-major and changes
the eight neighbors of each remaining door marker to ordinary floor, even when
the neighbor was another door or special tile. Earlier writes affect which later
markers are processed. A second x-major pass changes void immediately above
ordinary floor to wall. Room IDs, flags and recorded floor count are unchanged.
Boundary door markers would access outside the original allocation; the port
rejects these malformed inputs atomically. Generated geometry has margins.
The function is separate from object placement and does not change Session World.
Local coverage checks overwritten adjacent markers, above-floor walls, retained
metadata/floor count and atomic boundary rejection. Twelve local groups pass.

Door-transition run 42602928512 (dfa1ce4) and punch-out run 42603410256 (e3247e9)
passed native validation and package smoke.

## Point references change templates during loading

PointProperty::SetFrom (0x1ace5e) matches 1,024 original executions on small
controlled attribute trees. Original lookup/comparison/control flow executes;
strings, ASCII tolower/atoi and erase on one/two-node attribute maps are supplied.
Found/missing result, coordinates, consumed attribute count and every template
cell agree. Attributes are case insensitive; point prefixes concatenate directly
with at/x/y (including targetat). An at reference resolves the last row-marker
position and immediately writes numeric floor, without erasing the marker table.
Explicit x/y consume both attributes but do not change numeric geometry. Partial
x/y returns missing without consumption. Malformed errors are covered locally,
not as original error-handler comparisons; overflow retains stated port policy.

This corrects the earlier inference that digits stay void until population.
The constructor/row callback alone did not establish the final loaded template.
numeric_template now resolves common room objects and the supplied at-condition,
move/create-action and spell-target point references during ingestion. Unused
digits stay void. All 346 templates pass updated independent ingestion checks.
Full geometry is rerun with corrected base arrays: 64 cases / 67 attempts, three
rerolled cases, zero mismatches in cells/records/names/spawn and both RNG streams.
This remains a controlled-array execution, not full original XML parser/expansion
registration or room script-action parity. Thirteen local test groups pass.

Run 42604426802 (0eaa020) passed twenty-two native checks and relocated app smoke.

## Room element factory planning

RoomElementRecord::CreateInLevel (0x1a1d30) matches 2,573 executions with recording
entity constructor/finalization boundaries and supplied allocation/strings/rand.
Checks appearance decision, exact constructor/arguments, wrapping coordinates,
bookshelf contents override, Diggle God name and the RNG state after the gate.
All 23 unique recognized type strings plus unknown/case variants are exercised.
Unknown types still accept/finalize with a null object if their appearance gate
passes; type matching is case sensitive. The repeated portal comparison at the
end of the original dispatcher adds no reachable type.

Percent >=100 consumes no gate RNG. Otherwise the original rejects only when
percent < rand bucket 0..99: zero percent has a one-in-100 acceptance chance, and
99 always accepts while still drawing. Dispenser variants are craft4, food1,
bolt0, drink2, thrown3; portal receives0. Constructors/finalization can have
additional RNG and entity effects; the plan intentionally does not perform them
and does not claim complete content-pass RNG parity.

RoomDefinition::elements exposes typed common placement/type/name/contents while
retaining original source metadata. Independent XML ingestion matches all175
element definitions across346 supplied rooms. Common metadata is percent100,
can_push byte1 and empty room-local id by default; points use the verified resolver.
Thirteen local groups pass with reader, off-by-one gates, unknown types, vending
variants and atomic malformed metadata checks. New planning awaits native CI.

Run 42605231486 (34b0aa9) passed native validation and app packaging.

## Custom blocker/engraving construction

SpriteProperty::SetFrom (0x1aae04), in SpriteRecord mode, matches1,024 full
executions with synthetic attribute BST chains and string/ASCII/map-erase library
boundaries. Single images conflict with any series/Animate key; series requires
Sprite/First/Num/Rate and accepts optional Animate. This overload has no Animate
destination, so that key is consumed without changing animation behavior.
Missing partial series leave the attributes untouched. Malformed conflicts are
covered locally, not original error-handler comparisons.

CustomBlockerRecord::CreateInLevel0x1a02e6 and CustomEngravingRecord::CreateInLevel
0x1a04f2 match2,292 fixtures. The actual GameObj and object constructors, Build,
CustomRecord finalizer0x1a0290 and base finalizer0x19f45c run. Supplied boundaries
are allocation/strings/rand, sprite I/O, virtual placement/room-id binding and
standard list insertion. Compared type, appearance/final RNG, coordinates,
movement/visibility flags, push byte, name/description/id, sprite call parameters,
initial frame/animation flags and blocker registration. Constructor paths add no
RNG beyond the appearance gate within these boundaries. Original tile/list/id
binding effects and actual visual output remain excluded.

Blockers default to movement blocking without sight blocking; engravings default
to neither. Explicit passable and visblock override those separately. Basic
custom Build starts at frame0 and sets both animation flags true, including when
the input series says pngAnimate0. The port retains that behavior. Blockers enter
the blocker list even when passable; engravings do not. Low-byte can_push and
room-local id are retained. Custom object construction remains separate from the
owning Session/level and does not yet draw them natively.

RoomDefinition::custom_objects exposes typed definitions. Independent ingestion
matches all992 blockers/engravings across346 supplied rooms. Thirteen local
groups pass, including sprite conflicts/partial keys, defaults/overrides, gate
RNG and atomic reader failures. New custom-object integration awaits native CI.
