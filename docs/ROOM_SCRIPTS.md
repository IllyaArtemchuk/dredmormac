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

Action execution now ports the success/failure ordered loops (0x1a5284 and
0x1a52d6) and Create, Move, Remove, Ticker, SetObjectProperty and Spell actions,
including ObjectAction's missing-object dispatch. Requests execute immediately
through a callback so earlier movement can affect a later scripted spell's
player-default coordinates. Constructors, teleport, deferred destruction and
full spell bodies are explicit execution boundaries.

Move wraps room origin plus relative coordinates and requests forced teleport.
Create passes room origin to its record constructor; Remove queues destruction.
Ticker uses style1. The exact lowercase property "passable" sets blocking iff
its integer value is zero. "started" and "stopped" call Clock Start/Stop based
on exact integer equality with1/0; other integer values are not normalized to
booleans. Property names are case sensitive. Unknown properties request an error.
Wrong clock types are diagnosed and guarded in the port; the original roomError
fatal behavior is not replaced with a call into an unrelated object vtable.

Spell actions request Player::SetIdle, set player byte+0x8a4, remove the secondary
Bob from attached squares, optionally play the spell's cast sound, then invoke
DoActualSpellLoop. Origin and target independently default to player position
only when both coordinates are -1. Partial -1 pairs are relative coordinates.
The cast uses the player as caster, "by a spell" as reason, and final flag0.

`verify_room_actions.py` compares4,096 complete original success/failure passes,
46,081 ordered requests, changed live object/player positions, blocking/action
flags, lookup order and actual clock state/events, with zero mismatches. Actual
concrete action/lookup-dispatch/clock methods run; named execution, string and
returning-error boundaries are controlled. Unknown-property diagnostics do not
establish original fatal behavior; wrong-clock-type guards are excluded. All55
portable groups pass. Record XML/constructors, full downstream callees, event
queue, script scheduler and Session/native entities remain unfinished.

TypeAt (0x1a5a36) is now available as the twelfth condition kind. Its first
evaluation constructs an item prototype before inspecting the tile, even for an
empty tile. The original factory gets type/subtype, current level, quantity1 and
artifact=false. Later evaluations reuse the cached native kind/subtype. It scans
items at wrapped room-origin plus relative point, matching kind and subtype;
only the exact subtype string "random" skips the subtype comparison. Non-item
objects cannot satisfy the condition. Prototype construction is an explicit
callback so its RNG and uniqueness effects can be composed with the item factory.

`verify_type_at.py` compares8,192 complete original evaluations on4,096 conditions,
including2,017 lazy prototype constructions and preexisting caches. Native target
linked lists execute; factory/IsItem/string boundaries are controlled. Results,
constructor arguments/count, cache kind/subtype, tile query count and wrapped
coordinates all match. Full factory side effects and cached prototype save
semantics are separate. All55 portable groups pass, with condition regression.
