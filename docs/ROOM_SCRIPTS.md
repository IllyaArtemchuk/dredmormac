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

Room condition evaluation now ports AllConditionsPositiveIn (0x1a522c), the
ObjectCondition lookup/missing wrapper (0x1a6012), base Object condition,
Entered/Exited, At, IsDead/IsAlive, ActivatedChanged, Event, Stopped/Started and
Activated methods. Negation belongs to the outer script loop: it stops at the
first raw value equal to that condition's negation flag. An empty list succeeds.
Entered/Exited compare resolved current/previous room identities, not proximity.
At adds the script room origin with signed 32-bit wrapping. Missing objects use
the condition's explicit missing value without invoking its subtype or event
query. Event queries use the resolved object's local ID, not its lookup alias.

ActivatedChanged first requires an activatable, then its two-update change query;
wanted values0/1 filter the final activation state, other uint32 values allow any
change. Activated and clock queries compare their boolean result with the stored
low byte. IsDead/IsAlive require a Monster and use its dead field, not HP. Wrong
object types produce diagnostic requests; their complete original error-handler
behavior is outside the port. In particular the original dead/alive methods
continue to inspect the field if a supplied error handler returns.

`verify_room_conditions.py` executes 4,096 original full condition passes, with
6,213 evaluated conditions and2,095 controlled diagnostic requests, all matching.
It compares ordered evaluation/short circuit, negation, missing values, wrapped
coordinates, lookup/event arguments and diagnostics. Real condition virtuals,
activation and clock queries execute. Object lookup, event queue, strings and a
returning error handler are recording boundaries; diagnostic fixtures do not
establish original fatal-error behavior. All54 portable groups pass. TypeAt,
XML construction, script actions/scheduler and Session entities remain work.
