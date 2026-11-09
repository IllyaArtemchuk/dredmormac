# Status — 2026-11-10

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
- Thirty local CTest groups pass, including buff metadata, scheduled spells, deterministic session and save/load tests.
- Actual macos-15 arm64 CI run 42722079171 passed all forty-eight checks, including
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
