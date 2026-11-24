# Shop stock generation

`make_shop_stock` implements the stock-selection/quantity section of
WriteDungeonContentFromRoomRecords0x193ab0..0x193c07. `make_shop_inventory`
includes the preceding keeper variant libc draw and constructs ordered stock
with contiguous instance IDs. Entire batches commit RNG and unique-artifact
reservations only on success; identity overflow and resource limits reject.

The first MT bounded4 result selects the full level-set pool when0. That path
uses libc bounded selection and rejects special/debug entries, permitting craft
outputs. Otherwise an MT bounded2 chooses weapons or armour. Weapon selection
allows unique artifacts and uses offset1; armour uses offset0. Direct
CreateItem(entry,0) follows selection. It applies automatic accessory enchantment
but has no additional outer artifact roll merely because a definition is marked
artifact. Weapon selection retains unique reservations, while uniform selection
does not reserve them. Thrown/bolt weapons become stacks of
`2 + (uint64(libc.next()) * 0x1bffffff >> 57)`, capped only by positive maxstack.
The keeper variant is `libc.next() >> 29` and precedes all stock draws, even for
an empty shop.

Evidence uses the binary SHA256 recorded in ARMOUR_CONSTRUCTION.md:
keeper draw0x1939ba and variant store0x1939e7; stock loop0x193ab0; selectors
0x193b6d/0x193b87; actual direct CreateItem0x186c00 called at0x193b97;
for-sale store0x193b9c; ammunition multiply/cap0x193bcf..0x193c01;
pedestal construction0x193c2e and placement0x193c5b. The content writer visits
pedestals in template vector order, adding room origin to each relative point.

    .venv/bin/python scripts/verify_shop_stock.py ORIGINAL build/dredmor-treasure --cases 1024 --out PRIVATE.json

This passed1,024 actual content-writer executions on controlled single shops,
8,166 stock items and complete libc/normal/MT state plus next8 outputs. Actual
PunchOut, selectors, direct CreateItem, Armour automatic tail and CreateArtifact
execute. Keeper, pedestal and base item initialization/placement/library I/O are
recording boundaries. Harness checks keeper fields, pedestal traversal/positions
and the original for-sale write, alongside item types, stacks, artifact text and
modifiers and unique flags. Fixtures include all ten supported direct item kinds,
craft outputs, rejected flags, automatic/predefined accessories and unique weapons.
They exclude undefined original upper-bucket reads and unsupported item kinds.

This is a stock plan, not a complete shop entity or purchase implementation.
Keeper/pedestal lifetime, sprites, pricing, purchase/sale, theft and AI remain
unfinished. Ordinary loot must not be used to bypass their ownership/for-sale
behavior. The surrounding room-content pass and generated-floor integration also
remain incomplete. Local tests cover variant ordering, empty shops, craft/rejected
selection, ammo caps, unique reservations and rollback after earlier stock succeeds.

`item_shop_cost` now implements actual Item/Weapon/Armour::GetShopCost. Base
cost includes quantity for stackable items. Artifact weapon modifiers use wrapping
absolute values; armour keeps signed modifiers. Primary adds150 and factor0.3,
secondary100/0.1, damage100/0.25, resistance50/0.1 for each value in original order.
A spell field adds250 and factor1.0. Float32 stages, wrapping integer arithmetic
and x86 truncation/sentinel behavior are preserved. Non-gear base costs return
integers directly; gear converts through float even without artifacts. The spell
flag is explicit input until that artifact-header field is modeled in item state.

`resolve_shop_pedestal` implements raw Clicked decisions. Choice0 buys,1 declines,
2 takes the item while retaining its for-sale flag and requesting keeper movement.
Empty pedestals do nothing. Buy computes float-scaled virtual cost, truncates it,
then multiplies quantity again for stackable items. This second multiplication
is present in the original even though virtual base cost already includes quantity.
Insufficient funds reports the original dialog; successful payment clears for-sale,
transfers the item and returns handled=false. Decline and taking without payment
return handled=true. Negative quotes use the existing TakeZorkmids/give behavior.
The port rejects INT_MIN payment, whose original give/take virtual pair recurses.
No random state is consumed. Distance, modal UI and inventory placement are outside
this raw decision kernel.

Evidence: Item::GetShopCost0x5a072, Weapon::GetShopCost0x13931e,
Armour::GetShopCost0x387fe, ShopPedestal::Clicked0x3e99e..0x3ebd0.
The purchase quantity multiplication is0x3ea48. The insufficient-funds text is
"You don't have the money, punk!". Actual transfer0x5d2a0 and keeper movement
0x717f6 remain boundaries in the action comparison.

    .venv/bin/python scripts/verify_shop_actions.py ORIGINAL build/dredmor-shop --cases 4096 --out PRIVATE.json

All4,096 price and4,096 action cases passed. Price methods execute completely,
with no supplied calls. The actual Clicked executes with controlled modal choices,
virtual cost/payment, telemetry, pickup and guard dispatch boundaries. Comparison
checks full pricing, handled/transfer/for-sale/guard/funds flags, balance/payment
requests and no RNG draws. Tests include negative modifiers, integer extremes,
float rounding, missing definition fallback, stack quantities and all dialog
branches. This does not implement complete shops or original inventory/theft AI.

Room numeric ingestion now retains shop metadata as well as floor cells:
uppercase S assigns home, lowercase s the guard location, and each i appends a
pedestal in source row order. Repeated S/s markers use the last occurrence;
missing markers retain0,0. Rows truncate to declared width. Actual row callback
stores these at template+0x24/+0x28 and+0x2c/+0x30, and appends pedestal points
to+0x48..+0x4c (0x19b293..0x19b34b).

Generated attached shop rooms now retain absolute guard/home/pedestal positions.
The initial room has no activated shop RoomInfo even if its template shop flag
is set. Connected placements set the RoomInfo shop flag and absolute coordinates
at0x19681c..0x19692f. No extra RNG draws are introduced. Comparison includes
128 full synthetic geometries /137 placed shops and32 supplied-base geometries /
36 shops alongside existing cells, graph, names, spawn and RNG checks.

`move_shop_keepers` implements ForceShopkeeperMove0x717f6. Its int argument is
unused; the routine checks global inShop and PlayerHasStolenGoods. Both true move
all non-guarding keepers to their guard positions and mark guarding. Otherwise
all guarding keepers return home and clear that flag. Already matching flags do
not teleport or correct current coordinates. This affects all keepers on the
level in original list order, not just one shop. Sprite/path/AI behavior and the
inventory stolen-goods query are separate from this positioning kernel.

    .venv/bin/python scripts/verify_shop_layout.py ORIGINAL build/dredmor-shop --cases 1024 --out PRIVATE.json
    .venv/bin/python scripts/verify_shop_geometry.py ORIGINAL build/dredmor-dungeon --cases 128 --out PRIVATE.json
    .venv/bin/python scripts/verify_shop_geometry.py ORIGINAL build/dredmor-dungeon --cases 32 --content PRIVATE_INSTALLATION --out PRIVATE.json

The row callback executes on controlled preallocated template/vector storage
with string boundaries. The real keeper movement function executes with supplied
stolen-goods and teleport boundaries. All1,024 row parses and1,024 guard transitions
match, including marker defaults/order/truncation, ignored int arguments, ordered
teleports, final fields and unchanged random state for movement.

Item instances now retain `for_sale`, matching original Item+0xa0. Shop stock
sets it true. Bag/belt/equipment/ground items keep it across port saves and cached
floor travel. Format19 writes `for-sale="1"` only when needed; older formats still
load, reject that new field if present, and retain their existing canonical bytes.
This is the port save format, not original save compatibility.

`carried_unpaid_goods` checks all48 bag entries,9 belt entries,12 equipped entries
and the optional mouse-held item. Session exposes its owned-container query;
mouse-held ownership is a separate explicit input to the kernel. Quantity and kind
do not gate the original check. Ground items are not carried stolen goods.

    .venv/bin/python scripts/verify_unpaid_goods.py ORIGINAL build/dredmor-shop --cases 2048 --out PRIVATE.json

All2,048 original PlayerHasStolenGoods0x47b20 and CurrentPickedUpItem0x59f20
executions match, including every query location and unchanged random state.
The routines execute completely on controlled pointers with no call replacements.
Local tests cover all query slots, equipped/bag/ground save continuation, malformed
flags/old-format rejection, cached unpaid stock and carrying it through floor travel.
Full shop entity and modal purchase/theft integration remains unfinished.

Further recovered integration constraints (not yet implemented): DoSteppedOn
0x130e60 uses numeric cell kind13, not a room bounding rectangle, to enter/leave
shops. On exiting with stolen goods it calls AngerShopkeepers0x620c6, marks the
level and all keepers angry, clears carried stolen flags, and switches music/dialog.
ForceShopkeeperMove's mode flag is global inShop. Shopkeeper construction includes
MonsterC2/tweak-dependent rank adjustment between the variant draw and stock;
a full shop constructor must account for those draws, rather than treating the
stock-only plan's keeper initialization boundary as a no-op in full gameplay.

`step_shop_presence` now implements DoSteppedOn's shop phase before0x130ffd.
Entering tile13 from outside a calm shop starts shop music and sets inShop/music
override. An angry level suppresses that entry. Leaving tile13 while inShop with
unpaid goods frees music, angers keepers, shows the theft dialog and starts thief
music, retaining music override; leaving without unpaid goods clears override and
rotates the normal tune. Both exit paths clear inShop. Stable states do nothing.
The dialog is "You have stolen merchandise from a shop! You're in trouble now...".

`clear_unpaid_goods` implements ClearStolenGoods0x47cc0. It clears bag, belt,
most equipped items and held items. Original equipped ranged field+0x6f4 is omitted,
even though PlayerHasStolenGoods checks it. The port preserves that omission.
This kernel takes mutable container snapshots; it does not imply native mouse-item
ownership or a complete session shop lifecycle.

    .venv/bin/python scripts/verify_shop_presence.py ORIGINAL build/dredmor-shop --cases 2048 --out PRIVATE.json
    .venv/bin/python scripts/verify_unpaid_goods.py ORIGINAL build/dredmor-shop --cases 2048 --out PRIVATE.json

All2,048 actual shop phases match with real PlayerHasStolenGoods,
AngerShopkeepers and ClearStolenGoods. Music/dialog and unavailable Steam are
boundaries. The harness additionally checks room-byte writes, keeper anger and
cleared flags including ranged omission. The unpaid oracle now also compares
2,048 complete clear operations alongside query cases. Neither phase consumes
random state. Later DoSteppedOn random-colour draws and object callbacks remain
outside this comparison and must not be omitted in a complete movement pipeline.

Session shop ownership is now integrated. `create_shop` rolls keeper variant,
invokes a provisional keeper constructor on the same RNG, then constructs stock
in pedestal order. This callback point preserves MonsterC2 difficulty-rank draws
between variant and stock. Actors and items reserve global session identities;
invalid placement or constructor failure rolls back all state. Session stores
keeper/home/guard/variant/anger, owned pedestal stock and the explicit numeric13
floor positions supplied by its caller. Pedestals and living keepers block motion.
Empty pedestals remain blocking. The keeper is an NPC separate from hostile enemies.

`use_shop_pedestal` connects measured costs/decisions to the wallet and owned item.
The integration selects the original inventory-mode transfer path: belt/bag insert,
or a ground drop at the player if full. It preserves the original destination
stack's existing flag when merging, as the existing inventory kernel does.
The optional original mouse-held-item mode is not yet integrated. Taking unpaid
stock requests the measured guard movement; a paid purchase clears its flag.
Raw action results preserve original handled semantics and do not advance a turn.
Distance/modal UI gates belong to the caller. Session movement applies the shop
presence phase for explicit shop tiles, sets anger and clears the modeled carried
flags on theft exit. Music/dialog requests become typed session events; playback
and full DoSteppedOn callback scheduling remain separate.

Port save20 retains all shop fields and item/actor payloads, accepting prior1..19
formats. Item and actor identities include cached-shop reservations and reject
cross-floor duplicates. Cached floors retain empty/stocked pedestals, keeper
position/guard/anger and presence state across visits and whole-run saves.
This preserves modeled prepared-floor state; original arrival music/global-state
sequencing, hostile keeper AI, sale/debt dialog and the full generated-floor pass
remain unfinished. It is not original save compatibility.

Local integration tests cover RNG callback order, constructor/placement rollback,
blocking, buy/decline/insufficient funds, unpaid taking/guarding/theft exit, full
inventory drops, Session20 round trips, malformed keeper IDs and whole-run cached
shop reservations/travel. Component original comparisons establish the rules;
a complete original shop playthrough has not yet been executed.
