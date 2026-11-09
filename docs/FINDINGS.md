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
