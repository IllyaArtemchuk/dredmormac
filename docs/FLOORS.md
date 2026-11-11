# Floor travel integration

`FloorRegistry` owns inactive floor snapshots; the existing `Session` owns the
current floor and global character state. Add prepared `SessionFloor` values,
then call `registry.step(session, action)` for movement-triggered travel or
`registry.travel(session, ascending)` to activate the staircase under the player.
Only registered destinations are available. Arrival chooses the matching opposite
stair, in x-major/y-ascending order; ascending can fall back to another downward
stair. Cached floor lookup uses depth, matching the original's branch-ignoring
search. Duplicate depths are rejected by this registry.

A floor holds its World, depth/branch, stair links, actors, loot, objects,
breakables and traps. Global inventory, stats, skills, buffs, player-owned scheduled
spells, cooldowns, progression, RNG and identity reservations stay in Session.
Travel preserves the global turn; inactive floor visit clocks are retained. The
current provisional scheduler updates the departure floor before travel. Inactive
floors freeze. This differs from the original on-demand regeneration path, which
is not implemented. Pending enemy-cast player spells cannot travel yet.

`registry.save` / `FloorRegistry::load` handle complete prepared runs (format1).
Session format17 adds floor metadata and the global minimum fresh item identity.
Earlier session save versions still load. Cached sessions' duplicate character
payloads are ignored; only the active session supplies global character state.
Readers enforce unique floor depths, global actor/item identities and reserved
identity bounds, plus the existing session checks. The native loader validates
active and inactive asset paths before replacing either state or presentation.

## Native scene fields

A scene can supply `stairs` and `floor_branch` for the active floor, plus a
`floors` array for additional prepared floors. Each floor provides `collision`
(ASCII World with one @), `depth`, optional `branch`, `stairs`, and optional named
`monsters` (name/x/y). Stairs use x/y/up/link; up is0 or1 and link is0..255. All
floors currently share the installation, atlas and hero assets. Stair sprites are
resolved through the installation resource resolver. Keyboard movement triggers
travel; arrival itself does not immediately send the player back.

Generate a fully synthetic test installation/scene with:

    python scripts/make-smoke-scene.py /tmp/native-floors --content-source tests/fixtures/content-root --floors

This exercises two prepared layouts, actual SPR rendering, cached monsters and
currency, whole-run save/load and invalid cached-asset rejection. It does not
build a full original dungeon, place random stairs or populate all room records.

## Original routine checks

`verify_stairs.py ORIGINAL build/dredmor-stairs --out PRIVATE_REPORT.json` compares
1024 BuildStairs grids/construction plans,1024 SteppedOn dispatch argument sets,
and1024 cached Ascend/Descend arrival searches. The binary SHA and exact supplied
boundaries are recorded in the private report. Original binaries/assets and
reports stay outside git. Full level generation/load, off-floor regeneration,
new-floor arrival fallback and complete GameTick behavior remain outside these
comparisons.
