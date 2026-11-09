# DredmorMac

Native Apple Silicon macOS reimplementation in progress. This checkpoint is an
analysis foundation and provisional room preview, **not a complete game**.
Original game files are required and are never distributed with the source.

The portable C++17 core runs deterministic room replays. A native AppKit target
loads an original tileset and animated SPR hero/monster sprites directly. The distance
routine has been checked against 8,200 executions of the supplied i386 binary.
The RNG matches 28,550 original outputs, including exact double bits. Base stat
derivation matches 2,568 original code-fragment runs; the entire character
attribute function also matches 1,027 fixtures with abilities/equipment/buffs
absent. Movement, room rendering and the macOS frontend have not been compared
with original gameplay. See [status](docs/STATUS.md) and [deviations](docs/DEVIATIONS.md).

## Build and verify

Requires a C++17 compiler, Python 3.11+ and the Expat development library.
On macOS use the SDK's Expat or install Expat with your package manager; on
Debian use libexpat1-dev. Setup installs project-local CMake and Unicorn;
no paid service or original game download is involved.

```sh
scripts/setup.sh
export PATH="$PWD/.venv/bin:$PATH"
scripts/build.sh
build/dredmor-replay tests/room.txt 'EES.WW'
```

The room text format is an internal harness format: `#` blocks, `.` is floor,
and exactly one `@` marks the initial player. Actions are `N S W E .` without
spaces. Each line of output is a state after the corresponding action, preceded
by the initial state. Blocked moves consume no turn under the provisional policy.

## Use your original installation

Keep data outside the repository. For the supplied Mac app, DATA_ROOT is its
`Contents/MacOS` directory, where game/, tilesets/ and sprites/ reside.

```sh
python3 scripts/inventory.py '/private/path/Dungeons of Dredmor.app' \
  --out '/private/analysis/inventory.json'
python3 scripts/prepare_scene.py '/private/path/Dungeons of Dredmor.app/Contents/MacOS' \
  --out '/private/derived/starting-room'
build/dredmor-replay '/private/derived/starting-room/room.txt' 'NNEES.WW'
.venv/bin/python scripts/verify_distance.py \
  '/private/path/Dungeons of Dredmor.app/Contents/MacOS/dredmor' \
  build/dredmor-distance --out '/private/analysis/distance.json'
```

The comparison runs the original function in an i386 emulator, not the entire
game. It requires the named i386 Mach-O executable from the supplied edition.
Reports include paths and original metadata; keep them private and outside git.
Generated scenes contain machine-local absolute paths; regenerate them on a Mac.

To run all original comparisons, including RNG, primary/secondary character
stats and independent native/Python database ingestion checks:

```sh
scripts/verify-original.sh '/private/path/Dungeons of Dredmor.app/Contents/MacOS' \
  '/private/analysis'
```

`dredmor-data DATA_ROOT` loads only base content; `--expansions` explicitly adds
the three supplied expansion database folders. Duplicate definitions, unknown
attributes, record order and nested monster ancestry are retained. Loading data
does not establish expansion override semantics or playable expansion support.

Combat analysis shows calls to macOS libc rand(), separately from MT19937.
The verified MT stream alone is not sufficient to claim deterministic combat.

## Native macOS handoff

On an actual Mac, install Xcode command line tools, Python 3.11+ and CMake,
then launch the app, choose your original installation and select seven skills.
Prepared JSON scenes
remain available for explicit developer encounters.

```sh
scripts/build-macos.sh
build/macos-arm64/DredmorMac.app/Contents/MacOS/DredmorMac
# Or supply an original .app / Contents/MacOS folder / prepared scene as an argument.
```

Arrow keys move or attack; `.` waits; G picks up and I opens inventory.
Select an item and target slot, then Equip; select equipped gear to Unequip.
Command-S saves the current port session;
Command-O loads a port save for the same prepared room. Selected skills and
passive abilities, custom room objects, artifact instances, currency and RNG caches persist in version-seven port saves; earlier port saves still load with item-type migration. Malformed saves fail
before replacing the current state. These saves are not original .sav files.
Closing the window quits. Normal installation startup grants the selected skills’ starting items and
class clothes. Prepared developer scenes keep empty loadouts unless configured.
Custom blockers and engravings now load their SPR/PNG-series graphics and affect
movement. The UI remains a developer room/encounter view. Full seeded-level
population and original visual/animation parity remain unfinished.

The portable session harness accepts `--room-objects` to populate the starting
preview's custom objects using the session RNG. This integration does not include
the preceding full-level generator consumption or intervening object constructors.

Direct installation startup passes its synthetic native fixture check. The current preview still omits original room scripts, population
and decoration.

Actual standard macos-15 arm64 runs compile with AppleClang and test the native
window, movement, combat and animated SPR loading. Run 42590387752 passed all
18 checks and relocated, ad-hoc signed app packaging smoke:
https://github.com/IllyaArtemchuk/dredmormac/actions/runs/42590387752

CI uses only synthetic assets. Its development app artifact contains no game
files and is not a complete game release. Original-asset visual fidelity,
notarization and full-game compatibility remain pending. The original i386 app
needs legacy macOS for full reference gameplay.

## GameFrame

`dredmor-dungeon` is a developer fixture harness for seeded numeric geometry.
The original comparison is `scripts/verify_dungeon.py ORIGINAL_BINARY
build/dredmor-dungeon --out PRIVATE_REPORT.json`; add `--content DATA_ROOT` to
exercise supplied base templates and word pools. It stops before door objects
and population and does not create a playable level. Original reference files
and reports must remain outside tracked source.

Use one separate checkout of https://github.com/tanepiper/game-frame. Read its
setup instructions, inspect scripts before running them, and keep its home and
analysis project outside this repository. `scripts/gameframe.sh` uses the
`GAMEFRAME_ROOT` environment variable; `GAMEFRAME_HOME` selects external storage.
The existing source repository remains authoritative; do not replace it with
GameFrame's generated project. Tool versions, failures and evidence are in
[findings](docs/FINDINGS.md); continuation instructions are in [resume](docs/RESUME.md).

For a Java installation that contains `jdk.compiler` but lacks the javac
launcher, `scripts/java-compiler-home.py JAVA_RUNTIME NEW_LOCAL_HOME` exposes
the real compiler module without changing the system installation. Set
JAVA_HOME to that new directory, GHIDRA_INSTALL_DIR to Ghidra, and GAMEFRAME_ROOT
to the external checkout. Then `scripts/analyze-original.sh ORIGINAL_BINARY
PRIVATE_ANALYSIS_DIR` imports once and reuses the saved project for targeted
exports. This workflow has successfully analyzed the supplied Mach-O.

`dredmor-factory` exercises simple loot creation using explicit ordered item
registration tables. Compare with `scripts/verify_factory.py ORIGINAL_BINARY
build/dredmor-factory --out PRIVATE_REPORT.json`. It covers named/misc/currency,
reagent/component, potion, wand, bolts, trap and weapon/armour paths. Artifact
routes use the recovered name/stat generator and persistent unique weapon flags. Session loot creation
retains constructor results and RNG across saves; full room population is pending.

`dredmor-item-tables DATA_ROOT` emits ordered base registration tables. Compare
synthetic original loader callbacks and supplied base ingestion with
`scripts/verify_item_tables.py ORIGINAL_BINARY build/dredmor-item-tables
--content DATA_ROOT --out PRIVATE_REPORT.json`. Base table registration is
implemented; expansion conversion/merge behavior remains unfinished.

`verify_gear_selection.py ORIGINAL_BINARY build/dredmor-gear-selection --out
PRIVATE_REPORT.json` compares full weapon/armour selection. `verify_gear_buckets.py
ORIGINAL_BINARY build/dredmor-gear-buckets --content DATA_ROOT --out
PRIVATE_REPORT.json` checks original bucketing and independent base ingestion.
These preserve the selectors' different alternate-depth and retry rules; exhausted
or invalid tables fail atomically under a port resource guard.

Artifact construction primitives are checked with `verify_artifact.py ORIGINAL_BINARY
build/dredmor-artifact --out PRIVATE_REPORT.json`; metadata uses
`verify_artifact_words.py ORIGINAL_BINARY build/dredmor-artifact --content DATA_ROOT
--out PRIVATE_REPORT.json`. These compare names, descriptions, stat rolls and RNG
with explicit Buff-constructor/item-text boundaries. Artifact instances support ground/inventory/equipment ownership and save persistence;
artifact factory selection/quality and unique weapon flags are integrated. Complete
constructor side effects and full room population remain pending.

Ordered room records include typed breakables and pedestals. Compare primitives
with `verify_breakable.py ORIGINAL_BINARY build/dredmor-breakable --content DATA_ROOT
--out PRIVATE_REPORT.json` and `verify_pedestal.py ORIGINAL_BINARY build/dredmor-pedestal
--out PRIVATE_REPORT.json`. Constructor/hit state matches explicit sprite/event
boundaries. Session ownership, collision, saving and native drawing are integrated;
the provisional cardinal movement policy strikes breakables on bump. Original
scheduling and script execution remain unfinished.

Directory resource lookup preserves the original expansion3, expansion2,
expansion, loose-file precedence. `verify_resource_mounts.py ORIGINAL_BINARY
build/dredmor-resources --content DATA_ROOT --out PRIVATE_REPORT.json` compares
startup mount requests and supplied sprite lookup. Packed core ZIP installations
and runtime mod mounts are not implemented; the supplied installation uses loose
files. Native breakable smoke also verifies layer precedence and atomic resource
validation during save loading.

Spell execution now supports basic damage, healing, player mana effects,
immediate and delayed trigger chains, damage over time and template area effects.
Active player stat buffs support refresh, stack caps, expiry and mana upkeep.
Trigger chains support required and forbidden buff-name conditions.
Pending spells, buffs, template rules and facing persist through saves. Generic buff gates,
remaining spell families and the complete original action queue are still
incomplete; this does not establish full-game parity.

The following original-executable comparisons require your private installation
and the analysis Python environment. Keep the JSON reports outside the repository.

| Scope | Command arguments after `python scripts/` |
| --- | --- |
| Immediate trigger chains | `verify_triggers.py ORIGINAL_BINARY build/dredmor-trigger --out PRIVATE_REPORT.json` |
| Scheduled spell creation and updates | `verify_scheduled.py ORIGINAL_BINARY build/dredmor-scheduled --out PRIVATE_REPORT.json` |
| Trigger buff conditions | `verify_trigger_gates.py ORIGINAL_BINARY build/dredmor-trigger-gates --out PRIVATE_REPORT.json` |
| Buff ordering and tie behavior | `verify_buff_order.py ORIGINAL_BINARY build/dredmor-buffs --out PRIVATE_REPORT.json` |
| Player stat buffs and mana upkeep | `verify_buff_session.py ORIGINAL_BINARY build/dredmor-buff-session --out PRIVATE_REPORT.json` |
| Buff metadata and timer decisions | `verify_buffs.py ORIGINAL_BINARY build/dredmor-buffs --content DATA_ROOT --out PRIVATE_REPORT.json` |
| Mana arithmetic and mixed casts | `verify_spellpoints.py ORIGINAL_BINARY build/dredmor-spells build/dredmor-spell-cast --out PRIVATE_REPORT.json` |
| Template targeting and base ingestion | `verify_maneuvers.py ORIGINAL_BINARY build/dredmor-maneuver --content DATA_ROOT --out PRIVATE_REPORT.json` |
| Template casts with nested effects | `verify_maneuver_cast.py ORIGINAL_BINARY build/dredmor-maneuver-cast --out PRIVATE_REPORT.json` |

The template importer normalizes three missing source cells to empty and skips
effect coordinates outside the map, where the original can perform invalid reads.
The comparison scopes and other remaining limits are recorded in
[findings](docs/FINDINGS.md) and [deviations](docs/DEVIATIONS.md).
