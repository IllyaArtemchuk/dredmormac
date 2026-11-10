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

Run42607026309 (4c1ad45) passed native validation and packaging.

## Session ownership, persistence and custom-object rendering

Session now owns custom objects and uses their measured movement flag for player
steps, new enemy spawns and BFS routes. Appearance consumes the session's libc
stream atomically; invalid positions/budgets do not commit state/RNG. Core world
tiles remain separate from dynamic object collision. Preview room preparation
can leave custom collision to Session while retaining its existing preview spawn.
The portable CLI exposes this path with --room-objects. Full generation, object
ordering/RNG across other record classes, original scheduling and pushing remain
separate/unverified.

Save v4 serializes custom type, position, labels/id, push/collision flags, sprite
metadata and modeled animation fields, plus existing full RNG/session state.
Reads v1/v2/v3; old swapped item-type migration applies only to v1/v2. Tests cover
exact object roundtrip, future appearance draws, v3 normalization and malformed
type/flag/position/number rejection. Thirteen portable groups pass; 256 supplied
starting-character initializations pass again. Custom-object original comparison
now also confirms the next eight MT outputs are unchanged in all2,292 fixtures.

Native startup populates the starting preview's custom objects after character
creation and draws SPR or zero-padded PNG sequences, resolving all resources
within the selected data root. Fourteen original starting-room files/frames exist
under the supplied root. Save reload prepares all object images before committing
either session or image caches. The synthetic native object fixture checks SPR
and PNG animation, blocked movement, exact save/reload and atomic rejection of
invalid object resource paths. Actual native execution is pending this push.
PNG sequence timing and drawing order remain presentation policy, not original
visual parity. Preview still omits preceding full-level RNG consumption and other
record constructors, so appearance sequences are not full-game verified.

Run42608683842 (01a3e5e) passed all twenty-four native checks and relocated signed
package smoke, including SPR/PNG-series custom objects and atomic resource reload.

## Monster, trap, loot and horde record requests

Typed population records preserve original common placement and source metadata,
optional monster/trap/horde name, loot type/subtype/quantity/artifact and horde
minimum/additional counts. Loot defaults to subtype random, quantity1, artifact0;
horde bounds default -1/-1, deferring to spawn tweaks. RoomDefinition::population
retains XML order among these four classes; it does not yet interleave every
custom/element/script constructor in one content executor.

verify_population_records.py matches2,132 original record executions at monster
0x1a1342, trap0x1a0ffe, loot0x1a2c64 and horde0x1a331c. The compared normalized
requests include appearance, wrapping position, named/random path, loot factory
arguments, horde arguments and appearance RNG. Allocation/strings/rand, monster
lookup, constructors, random monster/trap spawning, item factory, horde spawning
and finalization are supplied boundaries. Original trap-name searching runs over
controlled four-pool tables and finds the first match; selected database identity
is not yet represented/resolved by the port plan. Constructor/spawn effects and
their additional RNG are excluded. This is not a full population comparison.

Independent ingestion matches all830 supplied population records in346 rooms:
165 monster,102 trap,514 loot and49 horde. Thirteen local groups pass with defaults,
coordinate wrapping, appearance draws, malformed metadata and cross-class order
coverage. Factory implementations/session integration remain next.

Population-record run42610022836 (d56c8c4) passed native validation/package smoke.

## Cached normal sampling and save v5

NormalRandom reproduces box_muller(float,float), original0x85d70. It uses two MT
closed-interval doubles per polar candidate, float-rounded x/y/squared radius,
double log/sqrt, float factor/products and separate float multiply/add at output.
The normalized second value is cached as float; subsequent mean/deviation changes
apply to that cached value. MT seeding does not reset the cache. GameRandom now
owns it alongside MT/libc, keeping independent sessions reproducible.

verify_normal.py matches8,384 original result/cache states (4,192 cached calls)
and11,790 mixed output lines across varied finite float parameters, interleaved
raw draws and reseeding. Original rejection, MT, sqrt and float instructions run;
the imported double log is supplied by the host library. This does not compare
Darwin libSystem's log implementation or nondefault rounding modes. Original
28,550 mixed RNG outputs still match after extending the harness. The harness
adds an immutable post-call float-store thunk for cdecl float arguments/returns.

Savev5 persists cache validity and exact spare float bits, including when not
currently available. v1..v4 migrate with the untouched/empty cache. v4 custom
objects still load; legacy item-ID translation remains v1/v2 only. Nonfinite
cache values and invalid validity flags reject before replacing session state.
Tests cover exact cache roundtrip, old-v4 normalization, malformed fields,
original sampler goldens and reseeding/restore/atomic resource guard behavior.
Fourteen local groups pass. Random content selectors will consume this state;
the normal implementation alone does not populate a level.

Normal/save run42611259389 (116e773) passed all twenty-five native checks and
relocated signed package smoke, including original normal-sample goldens.

## Random trap selection and item metadata

select_trap matches1,024 full GetRandomTrap0x4e410 executions, including529 forced
101st-attempt fallbacks. Actual original MT/box_muller execute; tweak deviation,
double log and Darwin rand are supplied. Compares selected entry, clamped target,
attempt count, libc/cache state and next eight MT outputs. Synthetic ordered pools
exercise both depth branches, initial cached samples, special/debug flags and
missing target levels. Pool registration and trap construction remain external.

The function always samples normal at float(depth)+1 first. Alternate negative
depth discards that target and uses libc bucket0..19; alternate nonnegative depth
samples again at that mean without+1. Target clamps1..20. A libc pool draw is
discarded before up to101 attempts, accepting nonspecial exact-level entries or
the unconditional final attempt. Debug is not filtered. Session exposes this
selection on its owned RNG; save/reload tests compare twenty subsequent selections
and complete states, covering both cache transitions and alternate-depth branches.

Trap levels were incorrectly left at the item-root value. The original child
callback sets shared level0x238, and typed parsing now preserves that override.
Trap trigger/charges, mounting/facing, spell/origin and target-is-caster are also
retained. Actual entry constructor0x55a4c and full trap-child callback0x4f7d0 match
1,024 fixtures /1,366 nodes with string/ASCII/atoi boundaries and a controlled
parent level. Tests cover missing/repeated attributes, case-sensitive enum values,
case-insensitive keys and prefix integers; overflow remains the stated port policy.
All53 supplied traps pass independent metadata ingestion. Fourteen local groups
pass. Trap activation, disarming, spell execution and full population are pending.

Trap run42612861333 failed AppleClang at streams={} in the new normal/selection
test. GameRandom.libc now has default member initializer{1}, preserving behavior
and making aggregate reset portable; follow-up native validation is pending.

## Currency primitives

CurrencyObject models the separate money value, Item quantity/stack flag and six
sprite thresholds. Positive eligible zorkmidbuff modifiers apply sequentially in
ability-table order with float multiplication/addition and integer truncation
after each. Eligibility uses the first matching of seven selected skills and
level<rank; it does not use weapon or polymorph passive gates. Both percent and
percentage attributes populate the typed ability field; source metadata remains.
Currency kind10 is now named explicitly. This core is not yet a session wallet.

verify_currency.py matches5,166 cases:1,040 actual currency constructors/Build,
1,024 type-based rolls,1,024 named rolls and2,078 wallet transfers. Actual original
Item/Zorkmids construction, Build, currency branches of factories18ac20/185bb0 and
Give/TakeZorkmids11c446/11c402 execute. Allocation/strings/rand/strcmp, tweak,
sprite I/O, sound and floating text are supplied boundaries. Amount, sprite key/
path, Item quantity/stackability, wallet balance/delta/sound, roll libc state and
unchanged next eight MT outputs match. Constructor bonus fixtures include varied
skill ranks, sequential modifiers, signs, overflow/float limits and ignored NaNs.

Currency value lives separately from Item count: a quantity>1 factory override
sets count/stackability but does not change the money credited. Type zorkmids rolls
(effectiveDepth+1)*libcBucket15Plus2, or1 for negative ordinary depth without a
valid alternate depth. Named zorkmids rolls from wrapping (depth+1)*goldDropPerLevel
then adds5; invalid bounds return5 without a draw. The type factory uses curLevel,
not its Level argument. Wallet transfers wrap and allow negative balances. Only
INT_MIN transfer is guarded, since the original virtual negative-amount delegation
recurses indefinitely. Fifteen local groups pass; native currency validation and
session wallet/pickup/save integration are next.

Currency wallet integration uses the measured Give/Take arithmetic and constructor
bonuses; pickup removes the money object and credits its value independently of
Item quantity and bag capacity. Session actions retain provisional scheduling;
these integration tests do not establish original turn-order parity. All mutations
that can fail after pickup begins commit through a copied Session. The native
inventory fixture now walks onto17 coins, checks the wallet and continues equipment
and save/load smoke. Rendering still uses provisional ground markers and no coin
sound/floating-text implementation. Savev6 preserves wallets/value payloads and
adds typed currency-bonus bits to ability signatures. Earlier hashes remain valid
for v1..5; a fixture produced by the actual df8cac9 v5 serializer exercises cached
normal state, custom objects, nonempty skills and subsequent bonused pickup.
Currency core run42615446594 passed actual arm64 Mac validation/package checks.

## Simple loot factory integration

factory.cpp and Session::create_ground_loot connect ordered item selection to
inventory instance construction and session ground ownership. Four named pools
are searched in order with exact case, retaining first-match behavior even when
that entry has no supported constructor. Type artifact/zorkmids/misc dispatches
before subtype handling. Other non-random subtypes bypass the type and use named
lookup. Reagent/component share a global uniform pool; potion/wand/bolts use the
selected level-set pool. Trap uses the measured cached-normal selector. Quantity
above1 forces stack/count; currency value is independent. Named artifact and
random artifact/weapon/armour construction remain unimplemented and throw before
committing session/RNG changes. Registration tables are explicit input boundaries.

verify_factory.py matches2,100 original full outer/named factory calls, including
actual random-trap/normal/MT execution and currency constructors/Build. Simple
constructors are recorded; allocation/string/rand/strcmp, currency sprite and
trap log/tweak are supplied. Selected constructor/definition, forced quantity,
separate money value, libc/normal cache and next8 MT draws match. Default simple
constructor stack flags are excluded; factory forced-stack flags are compared.
Synthetic tables exercise every named type in each pool, duplicates, unknown
first matches, case, unknown types, ignored artifact flag on named items and both
currency routes. Sixteen local groups pass; session tests cover failure rollback,
full-state save/reload of mixed future factories and currency autocollection.
Wallet run42617482250 passed all26 native checks/package; factory MacCI pending.

## Item registration tables

Original startup0xb7c79 loads game//itemDB.xml,0xb7c82 sets argument7 and0xb7c8a
calls itemDBLoad4f6a0. ESIbase0xb69c1 +16798c points to string21e34d. Subsequent
expansion/expansion2/expansion3 calls pass5/1/7, respectively. This is instruction
and string evidence, not a whole startup execution claim. Base items therefore
register in sets0..2. The initial curDBLevelSets global value1 alone would have
led to an incorrect base table; the startup caller overrides it.

register_factory_item mirrors end callback544f0: global alchemical registration
precedes masking, converted entries append nowhere, every selected set gets the
named entry, traps/toolkits have separate tables, weapons always enter weapon
tables and only non-special weapon type6 enters bolt tables. Armour/potion/wand/
food/gem/mushroom tables retain order. Unrecognized types still enter named pools.
verify_item_tables.py matches1,024 complete callback fixtures /32,759 item records
across all eleven four-set tables plus reagent list. Controlled completed entries,
mask/conversion stack and empty enclosing-item stack are inputs; vectors reserve
capacity, weapon spell strings are empty, ASCII/string imports supplied. Parsing,
conversion mutation and spell binding are excluded. All565 supplied base entries
and66 reagents agree with independent XML ingestion. GameContent exposes these
base tables; expanded catalogs reject table access until conversion is recovered.
Native named developer ground items now use this path. Sixteen local groups pass;
registration/native changes await MacCI. Factory run42618377334 passed27 checks
and relocated packaged-app smoke on actual arm64 macOS.

## Weapon and armour selection

select_weapon/select_armour match2,048 full original function executions with
actual MT/box_muller, supplied tweak/log/libc boundaries, explicit ordered item-
level buckets and varied cache states, flags, offsets and depths. Selected entry,
artifact-spawned flags, libc/cache and next8 MT draws match. Weapon starts with
truncated normal at mean depth plus wrapping offset. Alternate negative uses
libc bucket0..19; alternate nonnegative consumes a new normal but discards its
value. Armour instead uses that alternate sample plus offset. Targets clamp to
0..20 and0..18, respectively, then lookup starts at bucket target+1. Weapon tries
100 draws before decrementing target/resetting counter0; an immediately lower
nonempty bucket then permits101 draws. Empty buckets decrement/reset via the
outer loop. Armour descends only empty buckets and otherwise retries indefinitely.
Port budgets/exhaustion guards fail atomically instead of reading unrelated globals
or looping forever; these are deliberate edge policies.

Weapon eligibility excludes craftoutput, debug, special and types5/6. Artifacts
require allow-artifact and not-yet-spawned; accepted artifacts mark that bit.
Non-artifact entries ignore that bit. Armour excludes craftoutput/special/debug
and ignores artifact status/uniqueness. Full selector tests cover these differences.
Factory currently invokes only non-artifact gear paths; artifact generation remains
explicitly unsupported, and no session artifact-uniqueness persistence is claimed.

build_gear_buckets reproduces full splitUpItemLists4f400 in256 fixtures /8,228
entries, with actual bucketing/vector growth and supplied allocation/string
boundaries. All20 levels/four sets/both gear types compare. Entries match kind/
level and !special, preserving named table order; other eligibility happens later.
Supplied-base weapon/armour buckets agree with independent ingestion. Two extra
empty port buckets allow high weapon targets to descend safely; original upper
out-of-array reads are excluded. Expanded full factory comparison still matches
2,100 cases with actual gear/trap selectors and simple-constructor boundaries.
Seventeen local groups pass. Registration/native run42619110918 passed27 Mac checks
and packaged-app smoke; gear integration MacCI pending.

## Artifact construction core

artifact.cpp models GenArtifactName3c290 and CreateArtifact3c540. Name assembly uses
libc word picks with two MT binary decisions. It always runs before a named-item
name replacement. Descriptions select qualities/material/decor by quality, always
consume another MT binary choice, and conditionally append extra decor above6.
Buff description is base item description plus space plus generated description.

Stat categories use libc bounded82. Damage0..15 and resistance16..31, duplicate
secondary ranges32..50/51..69 (index15 ignored), and primary70..75 each try up to
three MT binary additions while quality remains. Categories76..81 add a primary
point without consuming quality or MT; they can add free points. Weapon damage
also writes the weapon's damage array; armour writes only the Buff damage array.
Named definition quality overrides the passed quality after name generation.
Port roll budgets and invalid/missing word guards fail atomically.

verify_artifact.py matches1,024 full original calls. Actual name/description/roll
code and MT execute; zero-modifier Buff constructor, item virtual text and string/
allocation/libc boundaries are supplied. Compares names, descriptions, quality,
6primary/24secondary/16damage/16resistance/16weapon values, libc and next8 MT draws.
Fixed-name cases pass a different caller quality to prove the definition override.
This does not compare complete Buff construction, item factory, equipment or saves.
verify_artifact_words.py matches2,048 actual text-loader callbacks with reserved
vectors and ASCII/string boundaries, including case/empty/missing attributes.
All900 base word entries independently agree:49 starts,40 phonemes,58 titles,
192 nouns/192 plurals,180 adjectives,65 qualities,63 materials,61 decor. GameContent
retains these words. Eighteen local groups pass; artifact core MacCI pending.
Gear run42620199153 passed all28 native checks and relocated packaged-app smoke.

## Artifact item ownership and equipment

InventoryItem carries the generated Artifact, and explicit Session artifact
construction commits RNG/ID/ground changes atomically. Pickup, bag transfer and
equipment retain text and all modifiers. Native inventory displays the instance
name. Savev7 uses nested artifact arrays/text and rejects invalid types, dimensions,
integer overflow and weapon damage on armour. Earlier1..6 port saves still read;
synthetic port-v6-wallet.xml is actual prior7aa0860 serializer output with wallet,
ground money, objects, skills and normal cache. It contains no original game data.

verify_artifact_equip.py matches1,024 original weapon/armour EquipOnPlayer/FixBuffs,
Player/HittableApplyBuff and PlayerRebuild paths /65,536 fields with actual buff
registration. Controlled single item/base metadata and artifact Buff are inputs;
string/allocation boundaries supplied, spell updates skipped. Primary/secondary,
damage/resist and current HP/MP compare, including bow artifact modifiers. Original
Rebuild excludes a weapon-owned Buff's damage; its weapon damage array is added
for melee attack calculations. Armour-owned Buff damage applies generically.
Artifact direct flat HP/MP updates occur at ApplyBuff; DeapplyBuff44dc6 subtracts
those and clamps HP>=1/MP>=0. Session models these direct changes; original full
unequip/replacement/trigger scheduling is not claimed. Synthetic core tests cover
injured removal at1HP, save/reload/future rolls, equipment restoration and atomic
failure. Eighteen local groups pass; native artifact fixture awaits MacCI.

Artifact-type factory machine code18ac9d..18ad06 confirms the first weapon is
constructed, tested for type5 (infinite loop if reached), destroyed and selected
again. Default selector excludes5/6, so the pathological branch is normally
unreachable. This duplicate creation must be preserved when connecting factory.
Constant231888 is0.25; depth-based artifact quality uses floor(depth*0.25) and libc
bounded+1, or1 without a draw when range invalid. Explicit random weapon/armour
artifact flag uses(rand*6>>32)+1, yielding1..3 over Darwin rand's positive range.
Named artifact construction consumes a depth-based quality roll before definition
quality overrides it inside CreateArtifact. Full factory integration remains next.
Artifact core run42621347955 passed actual Mac validation and packaged-app smoke.

## Artifact factories and uniqueness

make_loot_item now connects all modeled artifact routes. Generic artifact type
uses its Level argument for quarter-depth quality and preserves discarded first
weapon construction. Named items use curLevel depth and consume quality RNG even
when definition quality overrides it. Explicit weapon/armour artifact flag uses
1..3 quality. Source artifact flag alone does not cause generation on entry-based
reagent/component or unflagged random armour paths. Named subtype does cause it;
the passed artifact flag is ignored on named paths. This distinction replaces the
earlier overbroad guard that rejected any selected named-artifact definition.

verify_artifact_factory.py matches1,024 original full factory calls, including134
discarded first weapons. Actual selectors, name generation/CreateArtifact and both
RNG streams execute. Gear/Buff construction/destruction, virtual item text, floor/
log/tweak and allocation/string/libc are boundaries. Compares selected definition,
quantity, payload presence/text/quality/all modifiers/weapon damage, unique flags,
libc/normal cache and next8 MT. Full constructors and global object IDs are excluded.

Session owns ItemFactoryState and persists selected unique weapon indices in an
optional savev7 container. Earlier v7 saves could not select unique artifacts, so
missing container means empty. Loader rejects invalid/duplicate/non-weapon/non-
artifact indices. Failed placement rolls back selection flags and RNG. Resumed
sessions produce identical future mixed artifacts. Named construction does not
mark the weapon flag, while random weapon selection does; armour ignores it.
All18 local groups and existing2,100 simple factory comparisons pass. Native
artifact fixture now uses named factory. Ownership Mac run42623376554 is queued;
expanded factory native validation is pending. Full room population remains next.

## Ordered room records, breakables and pedestals

RoomObjectRecordFrom1a3ef0 recognizes12 exact lower-case types: customblocker,
customengraving, custombreakable, trap, monster, pedestal, element, loot, horde,
lever, clock, magicblocker. RoomDefinition::records preserves their source order;
last three retain raw pending definitions. Scripts have a separate original list.
Independent XML checks match all2,123 records across346 supplied rooms, including
116 breakable and10 pedestal metadata definitions. This is not an original whole
XML-parser execution comparison. Unknown metadata remains on RoomDefinition.source.

Breakable construction uses normal/breaking/broken SpriteRecord; missing breaking
falls back to normal. Build changes one-frame breaking delay to10. Intact state
usesframe0, initially broken useslastbrokenframe; both animation flags are0. The
constructor still blocks movement when initially broken. Passable/vis overrides
apply through CustomRecord's actual virtual finalizer (2653a8+10 points1a0290),
after construction. AttackSomehow increments hits only if not alreadybroken,
emits hit then optionally broken, usesmin(hits,intactframes-1) when intact, and
onbreak records a transient breaking animation before selecting static lastbroken
frame, clears movement/vision and stops animation. Alreadybroken returns0 unchanged.

verify_breakable.py matches1,024 original full record/constructor/GameObj/Build/
finalizer/AttackSomehow cases /4,580 requested hits. SpriteRecord resources, virtual
placement/id, script events and PlayOnBob are boundaries; compares appearance,
libc/unchangedMT, hitcounts, coords/labels/id/push, overrides, frames/flags/delay,
trigger order and animation request. No custom contained item is modeled (original
custom constructor sets it0); trigger execution/rendering/scheduling are excluded.

Pedestal defaults PNGdungeon/podium_scroll.png/keyPedestal_ext, movementblocked,
visionclear, stoppedframe0. Name/text and commonfinalizer metadata survive.
verify_pedestal.py matches1,024 full original record/constructor/finalizer calls
with sprite/place/id boundaries and unchanged MT. Eighteen local groups pass;
session/save/native integration of these new types remains next. Artifact factory
run42624802836 passed all31 native checks/package; older ownership run cancelled
whilequeued, successful descendant validates all prior artifact changes.

## Owning breakables and directory resources

Session now owns breakable objects and pedestal visuals, applies collision in
player/enemy routing, and emits ordered hit/broken events. The cardinal bump/
adjacent strike scheduling is provisional, like existing combat turns; original
movement queue/AI/script execution is not claimed. Resource callbacks occur after
appearance gates; callback/placement failures leave session/RNG unchanged. Savev7
adds optional breakable objects with three sprite records, framecounts/hits/delay
and strict validation; absent data migrates toempty. Pedestals usecustomkind3.
Native previews build supported visual records in relative XML order, omitting
intervening unsupported constructors/RNG, so whole-room parity is not claimed.
Native draws saved static frames and records transient breaking animations using
the existing presentation clock. Original visual scheduling remains unverified.
Loads validate all sprite counts/paths before session/cache replacement. New smoke
fixture tests partial hit/save/reload, both badpath and framecount rollback, final
break/animation/passability/movement, and pedestal collision. Twenty local groups
and256 supplied starting characters pass; native validation pending push.

The supplied breakable XML references13 unique sprite files; five are expansion2
assets without a direct root path. Actual startup fragmentb6b6d..b6e8f (ESI=b69c1)
executes26 fixed PHYSFS_mount1c03d0 calls. Expansion3, expansion2, expansion game
folders mount at/layer/, their dungeon/items/skills/sprites/sfx/tilesets/ui folders
at corresponding root namespaces with append1. Then dredmorcore.zip mounts root
with prepend0, and. appends root. Thus with no coreZIP (the supplied installation)
directory precedence is expansion3,2,1,loose. ResourceMount plans matchall26 actual
requests with a recording/success PHYSFS boundary; this excludes fullstartup/I/O.
Directory resolver independently finds all13 source breakable files, five through
expansion2. Native object/monster/installation hero+atlas loading uses this lookup.
Synthetic native breakable fixture has conflicting framecounts in three layers
and one asset only in expansion2, proving lookup on Mac. Core ZIP/mod mounts remain
unsupported; requesting packed installations fails explicitly instead of using
incorrect loose precedence. Original resources and mount disassembly stay private.

RoomLootRecord::CreateInLevel1a2c64 executes the appearance gate, full item factory
18ac20 and base finalizer19f45c. A factory null still returns accepted1; appearance
rejection returns0. Finalizer only touches nonnull: virtualplace+98, ID+c0, then
zero-extends record byte14 into object push68. New verify_room_loot.py executes
these actual functions around previously measured gear selectors/CreateArtifact,
with gear/Buff constructors/destruction, place/ID, virtual text and library inputs
supplied. All1024 fixtures match accepted/null, translated placement, full-byte
push, ID, selected definition/quantity/artifact payload, unique flags, libc/cache
and next8MT. Full constructors/global object IDs/level registration/scheduling are
excluded. Existing1024 artifact factory regression cases still match.
Session integration commits gate/factory/RNG/metadata together and rolls back on
placement/factory exceptions. Item metadata travels with pickup/equip/unequip and
optional savev7 attributes, defaulting empty/1 for previous files. Native supported
record order now includes loot; skipped constructors still prevent whole-room
parity. Inventory/artifact fixture now uses a source room loot record and checks
identity/push through equipment. Twenty local groups pass. Prior breakable/resource
run42630170521 passed all35 actual Mac checks including packaging.

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

## Scheduled spells — 2026-11-10

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

Reference: supplied 1.1.4 i386 executable SHA256 a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d. Trigger default ctor9c32c/copy46002; payload remaining0, repeat4, spell8, percentc, caster10, gate fields14..20, parent string24. Original routines execute under Unicorn; C++ string/list/allocation, actor virtual identity, UI/audio boundaries are supplied. No supplied bytes or private reports are tracked.

## Buff metadata and actor update order — 2026-11-10

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
