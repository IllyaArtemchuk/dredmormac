# Known deviations

- No complete game exists yet: combat, enemies, inventory, equipment, skills,
  effects, generation, progression, death, saves, audio and original UI are absent.
- Movement is a harness policy: four cardinal directions; successful movement
  and waiting consume a turn, blocked moves do not. No original comparison yet.
- Preview imports only the first Starting Room and a small symbol subset.
  Vending machines and custom blockers block movement; doors are closed borders.
  Conditional decorations, entities, scripts and their graphics are absent.
- Preview spawn is the passable cell nearest the room center, not a recovered
  spawn policy. Wall and floor atlas crops are fixed, without adjacency rules.
- SPR exporter uses only the first frame, assumes transparent palette index 0,
  and rejects transparency trailers. Animation is not implemented.
- Native AppKit frontend is written but has not been compiled or run on macOS.
  The prepared synthetic window test and manual Mac CI workflow have not been run.
- Distance routine matches sampled original i386 execution, including wraparound.
  Unusual floating-point rounding modes have not been compared.
- RNG interfaces match sampled original outputs and double bits. Seed flow across
  the full game and the separate macOS libc rand() stream are not yet implemented.
- Class-rank primary and secondary stats match original controlled fixtures;
  abilities, equipment, buffs and polymorph aggregation remain absent.
- Catalog loading preserves sources and monster ancestry. It does not implement
  expansion override rules, monster variant inheritance or gameplay execution.
- Embedded startup patch version is 1.1.4; additional suffix/mod state and
  expansion parity are unresolved. No parity claim.
