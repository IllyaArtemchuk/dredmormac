# Nearby and bounded spawn locations

`find_good_location_near` implements FindGoodLocationNear0xc8a60 with actual
PassableOnAllSides0x5ffc4 semantics. It scans x-major offsets-3..3, then y-3..3,
checks bounds, candidate type1/13/14, empty tile object list and clipped3x3 ordinary
floor. The neighborhood includes its center, so candidate types13/14 ultimately
fail that check. The fast scan consumes no RNG. Failure calls the bounded helper
on center±10 with wrapping32-bit coordinate arithmetic.

`find_good_location_bounded` implements FindGoodLocationBoundedBy0xc8940. Each
of51 attempts draws libc bounded(maxX-minX) and bounded(maxY-minY), adds minima,
then checks nonnegative/level bounds, inclusive caller bounds, ordinary floor,
empty object list and the same neighborhood. Draws exclude the upper edge despite
inclusive validation. Count1 still consumes a libc draw; invalid count ranges do
not. The original bounded function's center arguments are unused. Both helpers
return the final attempted point on failure, matching their output references.
Neither consumes MT, changes layout or constructs/places an entity.

    .venv/bin/python scripts/verify_locations.py ORIGINAL build/dredmor-locations --cases 2048 --out PRIVATE.json

All2,048 complete original nearby/bounded functions and real PassableOnAllSides
executions match. Only libc is supplied. Checks cover result/final references,
libc state and unchanged full MT state, all-floor/shop/blocked/occupied layouts,
clipped edges, invalid/singleton ranges and extreme signed coordinates. Local
checks include no-draw fast order, count1 draws, exact51-attempt failure state,
retained failed point and invalid occupancy rejection before draws. Occupancy is
an explicit post-content object-list input, not a blocking-actor approximation.

These routines are used by hostile keeper collector placement and other original
spawn paths. Their port does not implement keeper/monster AI, SafeTeleport or
connect a complete generated floor by itself. Full callers must retain intervening
Monster::Update and constructor RNG at their original points.
