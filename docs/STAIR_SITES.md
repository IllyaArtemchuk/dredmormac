# Random stair sites

`place_stair_sites` implements the random-marker phase of
`Level::GenLevelContent1`, entering the original at `0x196b47` and stopping at
`BuildStairs` (`0x192bd6`). The supplied i386 executable has SHA256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.

Call this phase after room content, safe teleports and secret doors. It takes a
numeric layout, the MT stream, depth/campaign flag, resolved `numstairs`, and two
occupancy callbacks. `obstructed` represents
`SquareContainsObstruction(x,y,0,1)`; `occupied` represents a nonempty tile object
list. These are distinct tests, including for nonblocking objects. Callbacks must
read stable occupancy and must not mutate the layout or random stream.

For each requested stair, the original draws interior x then y using its MT
division/rejection mapping. It requires three walls across the candidate row,
void or wall above all three, and ordinary floor immediately below the center.
It checks obstruction below, then empty object lists at the center, above and
below. Previously selected sites must be at least 20 tiles away. On the 4,001st
otherwise-valid close-site rejection for that stair, it accepts the site despite
the spacing. Geometry/occupancy failures do not advance that counter. The squared
distance uses the original 32-bit wrapping arithmetic and signed comparison.

At depth zero, it places `numstairs` downward markers. Intermediate depths place
that many upward markers followed by that many downward markers. At exactly
depth9 (base) or14 (expanded), only upward markers are selected. Link bytes are
the stair index modulo `numstairs`, truncated to eight bits. Zero stairs does not
consume randomness. Existing stair markers do not join the spacing list. The
function returns selection order and writes only tile kind and link; the
subsequent `build_stair_plan` performs construction traversal and wall repair.

The port limits count to0..4096, requires dimensions of at least3, and imposes a
configurable candidate budget (default one million). These are explicit resource
policies, not original game rules. Input errors, budget exhaustion and callback
exceptions leave both layout and RNG unchanged. Callback side effects themselves
cannot be rolled back.

Run the private original comparison with:

    .venv/bin/python scripts/verify_stair_sites.py ORIGINAL build/dredmor-stairs --cases 1024 --out PRIVATE_REPORT.json

The harness reconstructs the function frame inputs from its prologue and executes
the original random-site phase, including vector operations and object-list
checks. Allocation, `numstairs` and obstruction responses are supplied. Fixtures
cover campaign/depth policy, zero counts, minimum dimensions, occupied tiles,
metadata preservation, existing markers and forced spacing relaxation. It checks
selection order, every cell field, all624 MT words and index, eight following
draws, and the full ordered obstruction query sequence.

This adds a verified generation phase to the core library. Native gameplay still
uses prepared floors. Full room content/decorations/shops/scripts, secret-door
generation and later population are not connected into a complete floor builder.
