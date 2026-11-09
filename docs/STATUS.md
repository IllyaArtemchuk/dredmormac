# Status — 2026-11-09

Branch: implementation/native-foundation. Initial repository had only Readme.md.
Overall implementation is incomplete. Milestone 1 has a working Ghidra/Unicorn
analysis toolchain, inventory and original-routine comparisons. Milestone 2 has
original asset loading, provisional room preparation and movement/replay code;
original gameplay comparison and original-asset visual fidelity remain pending.
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
- Room-placement collision: 69,632 original executions match.
- Room-writing cell loop: 1,024 original executions / 20,825 cells match.
- Numeric templates: all 346 supplied rooms match independent ingestion.
- Twenty-one local CTest groups pass, including deterministic session and save/load tests.
- Actual macos-15 arm64 CI run 42637405068 passed all thirty-eight checks, including
  native combat-window input, real libc RNG agreement, and relocated, ad-hoc
  signed app packaging/smoke. Original files remain private and outside GitHub.
- Native SPR decoder: all 698 files / 3,633 frames agree with independent palette
  expansion. Index-zero transparency and animation scheduling need original
  visual comparison. Renderer uses all frames; synthetic animation CI passes.
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

Current work: native scene renderer is connected to the turn session and original
content definitions. Combat-window CI and portable development app packaging pass.
Direct SPR animation passes Mac CI. Native save/load controls and atomic reload
checks pass on actual Mac CI. Room-placement collision now matches 69,632
original executions. Cell stamping matches 1,024 original loop executions; all
346 numeric room templates match independent ingestion. Seeded numeric geometry,
room records and naming now have controlled original comparisons. Full-level
session integration and content/script population remain pending.
DoRandomize and seed flow are identified; decision/arithmetic combat rules are
compared, while complete DoCombat/triggers and original HP/death side-effect
comparisons remain unfinished. Session HP/death behavior exists but is not yet
a full original transition comparison.
Obtain original gameplay captures for remaining parity comparisons. See RESUME.md.

Native startup now accepts the original .app or data directory directly, decoding
SPR in process. No-argument launch opens an installation picker. The prepared-scene
path remains supported. Synthetic installation startup CI passes. Native inventory/pickup/equip/unequip
controls and their synthetic-window check pass on actual Mac CI.

Starting-item kernels: 2,048 original food selections/final RNG states and 4,096
original class/always loadout decisions match. Typed filtered loadout plans exist;
complete starting item instantiation, auto-equip and abilities remain pending.

Player passive abilities: 1,024 original full-function fixtures / 63,488 fields
match with no equipment/buffs/polymorph and supplied string comparison boundary.
Typed abilities and character_profile now feed sessions/equipment. Save v3 stores
seven skills and ability content identity; v1/v2 port saves still load. Local tests
verify passive eligibility across equip/unequip and exact save/reload. Native
integration CI passes. Weapon-specific abilities, active spells and effects
remain incomplete.

Native installation startup now offers an exactly-seven-skill picker, preserving
database order and hiding deprecated entries. The selected IDs flow to class
ranks, passive stats and saves. Prepared scenes retain explicit/default profiles
for developer replay. Synthetic picker interaction passes on actual native CI.

Corrected swapped trap/toolkit metadata IDs from original constructors: toolkit
11, trap 12. Numeric inventory kernel still matches all 3,840 original operations.
Save v3 migrates v1/v2 IDs and fingerprints; a synthetic old-v2 fixture roundtrips
through migration locally. Native migration verification passes.

Basic item instance factory now retains original stackability and wand charges;
1,045 original wand calculations/RNG states match. GameContent appends Lockpick
and Spores engine definitions without changing external content fingerprints or
existing IDs. Optional charges roundtrip, including zero. Native validation of
these instance additions passes.

Starting character integration now grants base loadouts, default clothing and
four random consumables, including automatic gear and wand charges. 256 supplied-
content initializations cover all 36 skill IDs with no failures; ten local groups
pass. This is not full original BuildPlayerFromSkills or equipped weapon-ability
parity. Normal native startup is wired to these grants and passes Mac CI.

Equipped weapon training now enters the correct pre-scaling flat-stat stage.
1,024 original equip+rebuild executions / 63,488 fields match, including 235
melee-plus-bow cases. Bow damage bonuses no longer leak into melee damage.
Updated local checks and 256 original-content initializations pass; this new
weapon-training integration passes native CI. Dual wield/spells/triggers remain
incomplete.

Room child-flag ingestion is corrected and covers level bounds, sets, quest/
unique/god/population flags. Independent comparison now checks all those fields
across 346 templates. Whitespace in the supplied expansion loadout amount parses
with original atoi-style prefix semantics. Full level generation remains pending.

Generator prerequisites: 1,080 original MT range selections / 9,720 outputs,
1,028 original names/both RNG streams, and 1,024 original name-word callbacks
match. Room-name pools and fixed overrides are available; singular/plural loader
bug is preserved. Eleven local groups pass. Full generator integration remains
pending; these new prerequisites await native CI.

Room-name/RNG run 42599302441 passed all twenty native checks and packaging.
Level geometry now matches 256 synthetic fixtures / 339 attempts and 64 supplied-
base-template fixtures / 68 attempts across six depths. Comparison stops at the
original BuildDoors entry and checks cells, flags, graph, names, spawn and both
RNG streams. Retry wall flags and the three-door automatic starter requirement
are preserved. Twelve local groups pass; this new geometry awaits native CI.
Branch/template registration is an explicit input boundary. Door objects, room
scripts, stairs, population and native full-level integration remain unfinished.

Geometry run 42601954130 passed all twenty-one native checks and packaging.
Door assembly plans now match 1,024 original executions / 32,244 assemblies / 521
repairs. Constructors, placement and Open are recording boundaries; actual
entities, sprites and interactions remain pending. Twelve local groups pass.

Door-plan native run 42602496973 passed validation/package smoke. Door toggle and
remote activation now match 1,408 original executions with controlled occupancy
and UI boundaries. Cell bits, part state, handled result and occupancy order match.
Lockpick/kicking, animation state and session/native door integration remain
unfinished. Twelve local groups pass; new transition code awaits native CI.

Content-pass punch-out now matches 2,048 original executions / 404,862 cells with
no imported routines supplied. Ordered neighbor clearing and above-floor walls
are preserved. Twelve local groups pass. Object placement and full-level native
integration remain unfinished.

Door-transition run 42602928512 and punch-out run 42603410256 passed native CI.
Point lookup now matches 1,024 original parser executions with library boundaries.
Corrected numeric ingestion: referenced digit markers become floor during XML
loading, including supplied script point references. All 346 templates match
updated independent checks; corrected base arrays match 64 original geometries /
67 attempts. Thirteen local groups pass; this ingestion correction awaits Mac CI.

Point-ingestion run 42604426802 passed all twenty-two native checks and packaging.
Element creation plans now match 2,573 original executions with constructor/
finalization boundaries. Typed ingestion matches all175 supplied element records.
Thirteen local groups pass. Entity effects/extra RNG, other record classes,
scripts/population and full-level session/native integration remain unfinished.

Element run42605231486 passed native CI and packaging. Custom blocker/engraving
construction now matches2,292 executions through actual constructors/Build and
finalizers, with sprite/placement/id/list boundaries. Sprite property parsing
matches1,024 executions; all992 supplied custom definitions pass ingestion.
Thirteen local groups pass. Owning-level/session/render integration is next;
other record classes, scripted interactions and full population remain incomplete.

Custom-object run42607026309 passed native validation/package smoke. Session now
owns custom objects, applies their movement collision to players/enemy routes and
persists all modeled fields/RNG in save v4 (v1..3 still read). Native startup and
prepared scenes can populate/render original SPR/PNG-series objects. Thirteen
local groups, 2,292 original object states (also unchanged MT stream), 992 metadata
definitions and256 starting characters pass. New native object fixture and atomic
asset-load checks await Mac CI. Preview construction still omits full generator
RNG and intervening object constructors; complete content parity is not claimed.

Custom session/render run42608683842 passed all24 native checks and package smoke.
Population request planning now matches2,132 original record executions with
constructor/spawn/finalizer boundaries. Independent ingestion matches830 monster,
trap, loot and horde records across346 rooms. Thirteen local groups pass. Actual
population factories/ordering and complete gameplay remain unfinished.

Population-record run42610022836 passed native validation and packaging. Cached
normal sampling now matches8,384 original calls/cache states and mixed MT draws
with a host log boundary. Savev5 retains the Gaussian cache; v1..4 migrate. All
14 local groups and28,550 existing RNG outputs pass. New sampler/save Mac CI is
pending. Random content selection and complete gameplay remain unfinished.

Normal/save run42611259389 passed all25 native checks and packaging. Random trap
selection matches1,024 original calls/states (529 fallbacks); session save/reload
retains future selections. Trap metadata matches1,024 original callback fixtures /
1,366 nodes and all53 supplied definitions. Fixed child trap level overrides and
retained trigger/spell/charge/mount/facing fields. Fourteen local groups pass;
new trap selection/metadata source awaits Mac CI. Activation/full gameplay remain
unfinished.

Trap run42612861333 failed in the new test's aggregate initialization on AppleClang.
GameRandom libc now initializes explicitly to1; numeric behavior is unchanged.
Native revalidation is pending. This does not affect the passing original-routine
comparisons, but the new trap code is not yet a successful native checkpoint.

Currency core construction/rolls/transfers now match5,166 original cases with
sprite/UI/config/library boundaries and unchanged MT draws. Typed zorkmidbuff
metadata and kind10 are added. Currency value and stack quantity remain separate.
Fifteen local groups pass; session wallet, pickup and persistence are next.

Currency core run42615446594 passed native arm64 validation and relocated package
smoke, also validating the Clang aggregate-initialization fix. Session wallet and
separate currency_value payload are now integrated: movement/G pickup credits
money independently of bag space or Item quantity, and give/take retain measured
wrapping/debt behavior. Savev6 persists wallets/payloads and currency-bonus rule
fingerprints; v1..5 migrate. A synthetic fixture emitted by the actual previous
v5 serializer checks nonempty skills, objects and normal cache migration. Fifteen
local groups pass. Native wallet display and the inventory smoke now check17
zorkmids on pickup; this new integration awaits Mac CI. Scheduling/UI/audio parity
and complete gameplay remain unfinished.

Wallet run42617482250 passed all26 native checks and relocated packaged-app smoke.
Simple loot factory/session creation now supports ordered four-pool named lookup,
misc/currency, reagent/component, potion/wand/bolts and random traps. A2,100-case
original comparison checks full outer/named dispatch, actual trap selection,
separate currency values, count override and libc/normal/MT state. Simple item
constructors are recording boundaries; currency constructors/Build execute with
sprite boundaries. Sixteen local groups pass, including mixed resumed factories
and atomic failed placement. Registration tables remain explicit inputs. Random
weapon/armour/artifact generation and complete room population are unfinished;
those unsupported generators throw atomically. New factory source awaits Mac CI.

Simple-factory run42618377334 passed native validation/package. Factory table
registration now matches1,024 full original end-callback fixtures /32,759 completed
items, including conversion skips, all eleven four-set tables and the global
reagent list. Independent ingestion matches565 supplied base items /66 reagents.
Base mask7 is traced at original startup0xb7c82/0xb7c8a; base registration populates
sets0..2, leaving3 empty. GameContent exposes base tables; expansion tables remain
explicitly unavailable until conversion/merging is implemented. Native developer
named ground items now use the measured factory and tables. Sixteen local groups
pass; this registration/native integration awaits MacCI.

Registration/native run42619110918 passed27 native checks/package. Weapon/armour
selection now matches2,048 full original executions, including cached normals,
alternate-depth branches, offset wrapping, weapon rejection/downshift and artifact
spawn flags. splitUpItemLists bucketing matches256 original fixtures /8,228 entries
and independent supplied-base ingestion. Non-artifact gear is connected to factory
creation; expanded2,100-case full-factory comparison passes with actual gear/trap
selectors. Seventeen local groups pass. Artifact generation still throws atomically.
Upper weapon target buckets outside the original20 populated levels are empty in
the port, and exhausted/invalid tables or retry-budget overflow fail atomically;
these safety policies are not original behavior claims. New gear source awaits MacCI.

Gear run42620199153 passed all28 native checks and packaged-app smoke. Artifact
name/description/stat construction now matches1,024 complete original executions
with Buff constructor and virtual item-text boundaries. Actual artifact name,
CreateArtifact control flow and both RNG streams execute; generated/fixed names,
all text/modifier/weapon-damage outputs and subsequent RNG agree. Artifact text
loading matches2,048 original callbacks including missing/empty/case variations;
all900 supplied base word entries agree independently. Eighteen local groups pass.
This standalone artifact core awaits MacCI; item ownership, factory integration,
equipment aggregation, uniqueness and save persistence are still pending.

Artifact core run42621347955 passed native validation/package. Session now creates
and owns artifact payloads through ground/inventory/equipment; native inventory
shows generated names. Savev7 retains all generated text/modifiers/weapon damage
and accepts v1..6; an actual previous-v6 synthetic wallet fixture verifies migration.
Artifact-equipped rebuild matches1,024 original full equip/ApplyBuff/Rebuild runs /
65,536 fields, including HP/MP. Weapon-owned buff damage is excluded from generic
player damage and added once from weapon damage; armour adds its buff damage.
Direct HP/MP bonuses update on equip; removal clamps HP to1 as original DeapplyBuff.
Eighteen local groups pass. New native artifact pickup/equip/save smoke awaits MacCI.
Complete artifact factory selection/quality and unique-state persistence are next;
ordered full room population and full gameplay remain incomplete.

Artifact factory routes now match1,024 full original executions, including134
discarded first weapons, named quality overrides, separate Level/curLevel depths,
1..3 explicit artifact quality, unique flags and all generated payload/RNG state.
Gear/Buff constructors/destruction and virtual item text remain boundaries. Session
persists unique weapon flags in optional savev7 metadata (earlier v7 files lacked
selection support and default empty). Failure and resumed mixed-artifact tests
pass; all18 local groups and existing2,100 simple factory comparisons pass. Native
artifact fixture now uses the real named factory. Ownership run42623376554 is
queued for a Mac runner; this expanded factory source awaits its own CI push.
Next: ordered room content/population and complete constructor/interaction effects.

Artifact integration run42624802836 passed all31 Mac checks, including native
artifact pickup/equip/save and relocated packaging. Earlier ownership run was
cancelled while queued; this successful descendant validates those changes.
Ordered RoomDefinition::records now preserves all2,123 recognized records across
346 supplied rooms, including116 breakables and10 pedestals. Breakable creation/
AttackSomehow matches1,024 original cases /4,580 requested hits; pedestal creation
matches1,024 original cases. Sprite/place/id/trigger/transient-animation boundaries
are explicit. Eighteen local groups pass. Session/native ownership and interaction
for these new record types are next; full content execution remains unfinished.

Breakables/pedestals now have Session ownership/collision and optional savev7
state; older v7 files omit the new data. Bump/explicit adjacent strike uses the
provisional turn policy, with ordered hit/broken events and enemy routes updating
afterbreak. Lazy sprite callbacks run after appearance gates and fail atomically.
Native rendering selects saved frames, plays transient breaking animations, and
validates all three sprite counts before loading state. New native fixture covers
partial hits/save/reload, resource/count failure rollback, break/passability and
pedestal collision. Twenty local groups and256 supplied starting characters pass.

Original fixed startup mount requests match26 recorded calls. Directory lookup
uses expansion3→expansion2→expansion→loose precedence (core ZIP wouldprepend, but
is absent in supplied installation). All13 breakable sprite files resolve, five
through expansion2. Native room/monster sprites and installation hero/atlas now
use this lookup; native fixture proves differing overlay framecounts/fallback.
Packed core ZIP and mod mounts remain unsupported. New integration awaits MacCI.

Breakable/resource integration run42630170521 passed all35 Mac checks, including
native interaction and packaging. Room loot now executes its appearance gate and
actual item factory in Session/native supported-record order. All1,024 original
record→factory comparisons match accepted/null results, coordinates, push/roomID,
full artifact payload, unique flags and both RNG streams. Constructor, place/ID
and library boundaries remain explicit; other room constructors are still skipped.
Item room IDs/push survive ground, inventory, equipment and optional savev7
metadata. Local rollback, container migration, malformed push and resumed mixed
record/factory tests pass; all20 groups pass. Native inventory/artifact fixtures
now use room-defined loot; this source awaits its own Mac CI run.

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
