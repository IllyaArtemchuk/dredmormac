# Conditional room decorations

The base text database supplies noun decoration records. `read_room_decorations`
preserves the original callback's registration order: painting, statue, floor
for each noun, regardless of attribute order. Empty sprite values are omitted.
The lookup key is the noun's plural value, including an empty plural; it is not
the singular noun. Tags and attribute names compare without ASCII case. When
case variants assign the same field, the last source attribute wins. `XmlNode`
now retains source attribute names in order alongside its existing value map so
this behavior can be reproduced without changing existing content/save hashes.

`plan_room_decorations` takes a numeric template, origin, resolved decoration
subject and ordered table. An empty subject skips the pass; other subjects use
exact case-sensitive equality. Each matching table entry traverses x-major,
y-ascending and checks the template's marker array: painting1 (`P`), statue2
(`@`), floor3 (`^`). It does not rescan the live punched-out level or deduplicate
overlapping records. Constructors and placement must follow this order.

`make_room_decoration` models the named-sprite Blocker/Engraving constructor
metadata: frame0, stopped animation, no vision blocking, movement blocking for
paintings/statues and passable floor engravings. `Session::create_room_decorations`
appends these objects transactionally and preserves the random state. Existing
custom-object saves and cached-floor payloads retain their metadata.

`GameContent::room_decorations` loads the base table. A native prepared-room scene
can supply `room_subject` with `room_objects`; decorations are created after that
room's explicit records. Existing resource lookup/rendering handles the sprites.
The synthetic `--decorations` fixture checks explicit-before-conditional order,
three sprite paths, static frames, collision and native save/load.

Evidence uses the supplied i386 executable SHA256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`:

- Text callback `0xd0a20`, noun branch `0xd1dcf`, registrations `0xd201f` (type1),
  `0xd20d7` (type2), `0xd2187` (type3).
- `WriteDungeonContentFromRoomRecords` `0x193682`: explicit records first,
  conditional decorations second, then shop processing for each placed room.
- Actual named-sprite constructors: Blocker C1/C2 `0xa9860`/`0xa9732`, Build
  `0xa91fa`; Engraving C1/C2 `0xa898c`/`0xa8854`, Build `0xa8744`.

Run:

    .venv/bin/python scripts/verify_decorations.py ORIGINAL build/dredmor-decorations --out PRIVATE.json

This passed1,024 actual XML callback cases (including reversed attribute aliases),
1,024 actual content-writer traversals on controlled rooms without explicit records
or shops, and1,024 actual GameObj/Blocker/Engraving constructors+Build. External
strings/vector append and sprite I/O are supplied boundaries; traversal records
constructor/placement requests separately. Tables, order, positions/resources,
movement/vision/frame/playing fields and unchanged RNG match. Resource-policy
limits and bounds checks are port guards rather than original malformed-data rules.

This implements the conditional decoration portion and its prepared-room runtime
integration. Complete ordered room-content execution, shops, script wiring and
full generated-floor population remain unfinished. Expansion table merging and
the placed-room hint override path also need complete recovery/integration.
