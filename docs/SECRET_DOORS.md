# Secret pockets and level treasure

`generate_secret_door_plan` implements the numeric layout and placement work of
`Level::GenerateSecretDoors` (`0x63436`). Call it after room content and safe
teleports, before random stair sites. Inputs are the post-content layout, complete
game RNG, placed-room `noblockers` flags, tile object-list occupancy and a treasure
factory returning an optional item value. The factory receives provisional RNG;
it must avoid external side effects. The result retains hidden wall/pocket
coordinates, treasure values and optional lever coordinates. Generated treasure
and levers join occupancy for subsequent searches. Layout and all RNG state,
including the normal cache, commit only on success.

The routine runs three rounds, each with a rejection counter limited to50. It
draws x/y using libc bounded(width/height-6)+3, requires three vertical walls and
rejects a room with `noblockers`. An otherwise eligible wall with no pocket counts
as two rejections; other failures count once. It carves a two-cell pocket on the
void side of the wall and fills surrounding void with walls. Treasure generation
occurs before the lever search and before another candidate round. The lever
search visits x-major offsets-5..4, then y offsets-5..4, requiring ordinary floor
throughout the clipped3x3 neighborhood and an empty object list. It excludes the
top and left edges. No available lever leaves the hidden pocket/treasure in place.

The template flag mapping is established by the original XML callback
`roomDBXMLLoader_StartElement` (`0x199e80`): the `noblockers` attribute comparison
at `0x19add9` reaches its boolean store at `0x19adfe` to template+0x7c.
`notreasure` instead stores to+0x80 at `0x19ae38`. In particular, the secret-pocket
routine tests `noblockers`, regardless of `notreasure`.

The plan omits actor constructors, sprites, global object identities and deferred
destruction. The original constructs an unplaced lever for every pocket, then a
second lever if placement succeeds; if no location exists, the first lever goes
on the destruction list. That lifecycle is checked in the original harness but
is not represented as game entities by the plan. The port requires dimensions of
at least6, correctly sized occupancy and valid placed-room references. Those are
safe input policies, not recovered behavior for malformed original levels.

`make_level_treasure` implements `Level::GenTreasure` (`0x62578`), distinct from
room-record loot dispatch. Resolve `TreasureOptions` from `treasure_options` and
the installation's `Tweaks`. The initial libc category roll occurs even for a
forced artifact. Ranges are inclusive, tested in original order; gaps fall back
to a weapon. Ammo chooses thrown/bolts using MT, then rolls and caps quantity by
the definition's `maxstack`. Lockpicks use the same quantity roll without the cap.
The original accessory filter treats armour types3,4,6 as its ring category,
including the supplied parser's legs type6. Preserve that behavior.

Food dispatch chooses edible/booze with MT. Booze's target-level normal sample
uses current depth+1, then the existing selection/fallback loop. Thrown selection
likewise samples current depth, ignores alternate-depth substitution and retains
its own100-attempt downward search/clamp. Forced artifacts select one gear item,
construct once and use current-level depth for quality, even when the Level
argument differs. The original's second boolean parameter is unused. Malformed
pools or resource-budget exhaustion throw without committing random state.

`Session::create_ground_treasure` assigns a fresh global item identity, performs
the factory call and validates ground placement transactionally. Generated item
state and RNG already fit the existing save format; save/load continuation and
invalid placement rollback are tested.

Original checks (binary SHA256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`):

    .venv/bin/python scripts/verify_secret_doors.py ORIGINAL build/dredmor-secret-doors --out PRIVATE.json
    .venv/bin/python scripts/verify_treasure.py ORIGINAL build/dredmor-treasure --cases 2048 --out PRIVATE.json
    .venv/bin/python scripts/verify_secret_treasure.py ORIGINAL build/dredmor-treasure --out PRIVATE.json

These passed1,024 isolated secret-pocket cases,2,048 actual GenTreasure cases and
1,024 combined original secret-door/GenTreasure cases. Comparisons include plans,
item selections/quantities, all cell fields, libc, normal cache and full MT state
plus next8 outputs. Artifact cases additionally compare full generated text,
quality, modifier arrays and damage. Item/Buff/lever construction, actor placement,
virtual text/refresh, external list/math/string/allocation and libc calls have
explicit boundaries; the combined oracle executes actual GenTreasure and all its
selectors. Existing simple/artifact factory regressions also pass3,124 cases.
Fixtures keep gear targets within allocated original tables: undefined original
upper-bucket reads are not treated as valid gameplay behavior.

Secret lever activation is now integrated with session collision, save format18,
cached floors and native prepared scenes. Its original type1 FlipLever opens only
once; subsequent flips report stuck. The original tutorial path activates before
dispatch and therefore reports stuck even on its first pull; this quirk is modeled
by the low-level API. Click/bump requires exactly one cardinal tile of distance;
the original clicked gate also rejects an item held on the mouse. Remote activation
has no distance gate. `verify_levers.py` passes2,048 actual FlipLever/Clicked/
RemoteActivation/PlayerTriesToMoveInto comparisons, including real Activatable and
adjacency methods. Ticker/sound/tutorial/graphics calls are recorded boundaries;
all cell fields, activation state, messages, sound and graphics rectangle match,
and neither random stream is consumed.

The native scene field `secret_levers` contains x/y/target_x/target_y and optional
activated (0 or1). The lever blocks movement and uses the original lever1.spr
frames0/1. Bump or click opens the target wall and prepares presentation before
committing. Loading a prepared-room save permits wall differences at matching
secret targets, while retaining room-layout/configuration checks. Session format18
stores lever state; formats1..17 still load. Cached floors retain lever state and
opened walls. The interaction uses the session's provisional action scheduler;
it does not establish complete original GameTick, UI or audio parity.

Native play still uses prepared floors. Complete room-content/decorations/shops/
scripts and later full population are needed before this forms a complete generated
floor in the native game. Other lever types and room-script wiring remain pending.
