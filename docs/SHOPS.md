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
