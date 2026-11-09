# Known deviations

- No complete game exists yet. Combat decisions, inventory placement, equipment
  eligibility, character/monster stat calculations and XP thresholds have original
  comparisons. An integration session includes enemy turns, HP/death, pickup,
  equipment and saves. Seeded numeric geometry and door kernels are compared;
  their full-level session/native integration remains absent. Full combat triggers,
  skills/effects, dungeon population,
  original save compatibility, audio and original UI remain incomplete.
- Movement is a harness policy: four cardinal directions; successful movement
  and waiting consume a turn, blocked moves do not. No original comparison yet.
- Preview imports only the first Starting Room and a small symbol subset.
  Vending machines and custom blockers block movement; doors are closed borders.
  Custom blockers/engravings now populate and render from typed metadata, with
  their appearance gate and basic state compared. Other entities/scripts remain
  unfinished. Preview draws occur without preceding full-level generator/RNG
  consumption or intervening entity constructors; no whole-level parity claim.
- Preview spawn is the passable cell nearest the room center, not a recovered
  spawn policy. Wall and floor atlas crops are fixed, without adjacency rules.
- Native SPR loading decodes all frames and assumes transparent palette index 0.
  The renderer loops frame delays on a separate presentation clock; original
  scheduling and alpha still need visual comparison. The PNG exporter retains
  its first-frame fallback and rejects unsupported layouts/trailers.
- AppKit frontend compiles and runs on actual arm64 macOS CI; its synthetic
  window/input smoke passes. Original-asset visual fidelity and full gameplay
  validation remain pending. The UI remains a developer room/encounter view with
  native inventory controls, not original UI. Ground item markers are provisional;
  explicit developer item placement does not implement original population.
- Distance routine matches sampled original i386 execution, including wraparound.
  Unusual floating-point rounding modes have not been compared.
- RNG interfaces match sampled original outputs and double bits. Separate Darwin
  libc rand() stream matches 36,864 actual Mac outputs. DoRandomize startup seed
  flow is identified; full-game seed/consumption parity remains pending.
- Class-rank primary and secondary stats match original controlled fixtures.
  Base passive abilities now have controlled original comparisons and session
  integration. Weapon-family abilities and active effects remain incomplete.
  Measured artifact construction/equipment and unique weapon selection are integrated;
  complete constructor side effects, encrusting,
  dual-wield, buffs and polymorph aggregation remain incomplete/unverified.
- Catalog loading preserves sources and monster ancestry. It does not implement
  expansion override rules. Known monster numeric/sprite fields inherit parent
  values, while full variant spell/drop/effect semantics remain pending.
- Integration-session scheduling and cardinal BFS AI are provisional. Counter
  chains beyond 128 fail atomically; XP levels beyond the original 50-entry table
  are not supported. Malformed item integer strings are rejected more strictly
  than original atoi. These edge policies are not original parity claims.
- Session XML saves are port-specific and not compatible with original saves.
  Version7 preserves custom objects, artifact instances, currency and RNG caches and accepts earlier port versions.
  Native Save/Load controls operate on the current prepared room. Cross-room
  rendering and original load/menu UI remain incomplete.
- Embedded startup patch version is 1.1.4; additional suffix/mod state and
  expansion parity are unresolved. No parity claim.

- Trap runtime primitives now have1024 original sequence comparisons, including
  placement origin, detection, avoidance, charge/trigger/disarm state and spell
  requests. Session/native trap ownership, origin rendering/collision and automatic basic
  damage casts now exist. Other spell effects, disarm UI, full destruction queues,
  actual VFX/audio and original scheduling remain unfinished/unverified.
