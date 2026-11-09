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
- Eleven local CTest groups pass, including deterministic session and save/load tests.
- Actual macos-15 arm64 CI run 42597407998 passed all nineteen checks, including
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
346 numeric room templates match independent ingestion. Full seeded generation,
room records, naming and content/script population remain pending.
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
