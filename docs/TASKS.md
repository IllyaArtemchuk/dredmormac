# Ordered tasks and acceptance criteria

1. [x] Acquire original: readable archive, all ZIP CRCs pass, SHA256 recorded.
2. [x] Inventory: hashes, executable architecture, dependencies, symbols, content formats.
3. [x] Recover one behavior: compiled distance implementation matches 8,200
   executions of the original binary routine. Re-run verify_distance.py to reproduce.
4. [x] Analysis tooling: compiler module launcher repaired; Ghidra imports,
   analyzes and exports selected original functions. Broad suite 424/11; remaining
   failures recorded separately from working project checks.
5. [ ] Finish content scope: embedded startup version 1.1.4 confirmed by formatter
   arguments; verify additional version suffix/mod state and expansion metadata.
6. [x] Foundation: portable core builds; collision, bounds, malformed rooms,
   SPR conversion, inventory and importer tests pass; deterministic replay emits JSON.
7. [ ] Original room rendering: compare original captured Starting Room with
   frontend; recover spawn, atlas rules, decoration placement and sprite alpha.
   Current asset/room preview is provisional.
8. [ ] Original turn sequence: capture movements, wall bumps, waits and doors
   against original gameplay; assert coordinates and turn counts from that trace.
9. [x] RNG and data ingestion: 28,550 original RNG outputs match; all 40 XML
   databases parse with counts independently checked, including monster variants.
10. [x] Character base stats: fragment and full-function controlled fixtures
    match original class-rank/flat/percentage calculations; exclude equipment,
    abilities, buffs and polymorph.
11. [ ] Combat/enemies/inventory/equipment/skills/effects: establish original
   traces and compare deterministic transitions before claiming compatibility.
12. [ ] Generation/progression/death/saves: compare seeded levels and roundtrip
    original-compatible saves; malformed saves fail without partial mutation.
13. [ ] Remaining base-game UI/audio/systems, then separately scoped expansions.
14. [ ] Mac: build arm64 app on an actual Mac, verify architecture, launch scene,
    exercise movement/quit, record OS/toolchain, screenshot and observed results.
    Signing/distribution packaging and complete-game smoke tests follow.
