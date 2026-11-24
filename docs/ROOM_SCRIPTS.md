# Room scripting recovery

Evidence: supplied i386 executable SHA-256
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
Private disassembly/fixtures and supplied data remain outside tracked source.

The scripting object prerequisites now include measured activation history and
clock transitions (`clocks.hpp`). Activatable defaults all three history fields
to zero. Activate writes the current value. After base GameObj::Update, Update
copies previous to older, then current to previous. ActivatedChangedThisTick
compares current with older, so a change can survive two updates. Comparing only
the immediately previous value would change room script trigger eligibility.

Clock state is duration, remaining and step. Zero step means stopped; Start
writes -1 and always emits started, while Stop writes zero and always emits
stopped. Reset sets remaining to duration without starting the clock. Setting a
different remaining value emits updated before writing it. If the clock is
running and the new remaining value is exactly zero, it stops and then emits
expired. Setting an unchanged zero on a running clock still stops/expires.

A running Update adds step with 32-bit wrapping, calls SetTimeRemaining, then
compares the clock/player RoomInfo identities. Only the same room receives the
timer ticker. Positive values use "The timer ticks down to N..."; nonpositive
values use "The timer ticks down to 0!". Starting a zero clock and updating gives
-1, so it keeps running rather than expiring. Stopped updates do no room query.
The raw state supports positive/nonstandard steps observed in controlled inputs;
this does not imply the room XML creates those configurations.

Addresses: Activatable change query 0x7afb0, Update 0x7b0fe, constructor 0x7b372,
IsActivated 0x7b41a, Activate 0x7b42c; Clock stopped/started/reset 0x1aebe0,
0x1aebf6,0x1aec18; set remaining/start/stop/update 0x1aedbe,0x1aeeea,0x1aef82,
0x1af0e4. Clock's base event virtual slot is +0xd0.

`verify_clocks.py` compares 4,096 actual clock operations and 4,096 actual
activation operations with zero mismatches. Actual virtual clock calls run;
base GameObj update, event delivery, string functions and ticker are controlled
boundaries. It compares final fields, ordered event-time states, room-query
eligibility, ticker text, activation queries and base-update count. Focused tests
cover two-update history, expiry ordering, equal-zero sets, reset/start/stop,
negative and wrapping counters. All53 portable groups pass.

These primitives are not yet installed as Session entities or a room script
runtime. Clock constructors/record XML, Level event queue, condition/action
execution, lever/clock population and full original scheduling remain work.
