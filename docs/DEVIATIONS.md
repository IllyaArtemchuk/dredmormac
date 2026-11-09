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

- Immediate spell triggers, spellpoints and template AoE are now integrated with
  controlled original comparisons. Deferred triggers, full buffs and remaining
  spell families are pending. Missing maneuver-row cells normalize empty; effects
  outside map bounds are skipped because the original can read invalid memory.
  These policies are explicit deviations. Original template VFX and playback are
  not yet reproduced. Facing on successful moves uses recovered direction values;
  other original facing transitions and animation remain pending.

- Scheduled spell counter/chance/creation kernels now have2048 original sequence
  comparisons. Their placement before the provisional enemy-turn loop is a port
  scheduling policy, not verified original whole-turn timing. Actor tile-object
  overlap ordering, buff gates/taxa and Steam achievements remain unsupported.
  A4096-operation budget rolls back cycles; original unbounded looping is not
  reproduced. Missing dot child rejects rather than retaining a null spell.
  Save format8 is emitted when pending spells require persistence; otherwise7
  remains sufficient. Both are port formats, not original save compatibility.

- Buff metadata and timer decisions are compared with the original (1024 synthetic
  definitions,268 supplied definitions,2048 timer passes). This does not implement
  active buff application, stacking, upkeep or stat-removal side effects; session
  buff casts still fail atomically. Timed expiry occurs below zero in the original.
- Scheduled session ordering now follows the identified GameTick/Monster::Update
  relative order: monster triggers before its AI; player triggers after monster
  updates. Complete object-queue order and speed/time flags remain unverified.

- Active player stat buffs and mana upkeep now match1024 original sequences with
  full attribute rebuilding and save/resume. Monster buffs, lethal apply paths,
  other upkeep/status callbacks, icon resource aliases and large equal-name sort
  ties remain outside the verified scope. Relative turn ordering is measured;
  the full action queue remains provisional. Save9 is port-specific.

- Immediate/deferred trigger buff-name conditions now have2048 original sequence
  comparisons. Immediate gates inspect the caster; scheduled gates inspect the
  owner after chance. Generic requirebuff9c remains unsupported. Monster active
  buff state is not implemented. Save10 retains gated queue state, separately
  from the original game's save format.
