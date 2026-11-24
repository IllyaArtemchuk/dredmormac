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

`read_room_condition` now parses all11 original factory condition names and their
registered XML properties. Keys are case insensitive; factory type values are
case sensitive. Optional not uses atoi-style boolean parsing. Object conditions
require id, while notfound defaults true for isdead, isalive and activated_changed,
and false for the other object conditions. ActivatedChanged's integer activated
filter defaults -1. Activated/started/stopped boolean filters default true. At
and TypeAt require coordinates/marker; event requires event_type; TypeAt requires
type and defaults subtype to random. Marker references immediately produce
numeric floor. Unused attributes are rejected. Malformed errors and overflow
retain explicit port error/number policies, not original fatal-handler parity.

`verify_room_condition_metadata.py` executes the actual RoomConditionFrom
constructors, virtual property registration and ReadFromXMLAttrs on2,048 fixtures
covering every factory type, defaults, case-insensitive keys, atoi prefixes,
empty strings and point/template effects, with zero mismatches. Controlled maps,
string/formatting/ASCII number helpers and erase boundaries are supplied; full
outer room XML callbacks and duplicate case-alias insertion are separate. All55
portable groups pass. This reader is not yet wired into Session scripts.

`read_room_action` now handles all six factory action schemas after the owning
script removes its on routing. Move/remove/set require id; move requires a point;
ticker requires text; set requires set_type and an integer value. Spell requires
casts, with optional source/target-prefixed points defaulting independently to
(-1,-1). Spell identity/cast sound come from an explicit resolver after property
reading. Create requires create_type, then delegates the remaining canonical
record node to a record reader which owns its schema and point/template effects.
Action type values are case sensitive; attribute keys are case insensitive.

`verify_room_action_metadata.py` compares2,048 actual factories/property readers,
including specialized SpellAction/CreateAction attribute readers, with zero
mismatches. Actual map cloning, point/property flow and all template cells are
compared using supplied standard-library map/string/number boundaries. Spell
lookup and created-record factory/body reading are explicit recording boundaries;
record-body schema/effects and outer on routing are not included. All55 portable
groups pass. Script repetition/level event lifecycle and Session wiring remain.

The complete Level trigger lifecycle is now ported: DoRoomTriggerLogic0x61bd4 and
WantsSuccessActionsPerformed0x61ac6. It visits filled rooms and template scripts
in order. DirectorFillRoom0x152310 checks/sets RoomRecord+0x10 as its content-filled
flag (store0x15232f); the scripting gate does not mean the player currently occupies
the room. Null rooms/templates and unfilled rooms are skipped.

Only repeat=-1 is unlimited. Other limits materialize a zero count if absent and
compare signed count < limit. Successful actions run immediately, then their
count increments with 32-bit wrapping. Exhausted scripts do not evaluate conditions.
Debug flag8 materializes counters before the limit check, including unlimited
scripts whose conditions fail. Failure actions are stored by the schema but
DoRoomTriggerLogic does not invoke them. Previous-room identity remains unchanged
through all callbacks, then resolves the player's room after actions. Every queued
event clears at the end. Shared template definitions keep per-room counters.

`verify_room_triggers.py` compares4,096 complete original fixtures across repeated
passes, with9,494 success callbacks, zero mismatches. Real repeat getters, map
lookup, loop/control flow, prior-room update and full event-tree clearing run.
Counter insertion, library destruction, conditions/actions, debug output and a
returning roomError are boundaries. It compares skip/gate order, debug insertion,
negative/wrapping counts, live event/counter visibility, moved-player room and all
final state. Full condition/action bodies and GameTick/Session timing are separate.

The event queue is now a case-sensitive map (assigned room, object-local ID) to a
set of event names. Set deduplicates; absent queries do not insert. GameObj::SetEvent
0x36cbe uses its own level+0x64, assigned room+0x6c and local ID+0x70, independent
of current position or global current level. Level set/query/clear are0x61712,
0x6150c,0x60ab8. `verify_room_events.py` matches25,573 actual operations across2,048
fixtures, comparing every result and complete queue contents/counts. Container
insertion/destruction and strings are supplied. Actor room assignment/finalization,
constructors, script metadata aggregation and Session/native scheduling/save remain
work. All56 portable groups pass.

Parent script metadata now defaults repeat to1, uses atoi for repeat, and ignores
other script attributes as the original callback does. Child conditions/actions
remain ordered within their respective lists. Actions require exact on="success"
or on="failure" routing; each route retains its source order. The typed parent
reader accepts condition/action children; malformed nesting and other child forms
are not an original fatal-handler comparison.

`verify_room_script_metadata.py` executes actual outer roomDBXMLLoader Start/End
callbacks, factories, property/subclass readers and vector registration on1,024
mixed script fixtures. Repeat defaults/source-order case aliases/ignored script
attributes, interleaved children, routing, resolver order, created-record payloads
and numeric template cells all match. Raw attribute-to-map marshalling, library
helpers, spell lookup and created-record factory/body are supplied. Full Expat/file
loading, created-record body semantics and scheduling/save remain separate.

RoomDefinition::scripts now exposes the typed definitions with caller-provided
spell/record readers. Numeric template ingestion also resolves TypeAt and scripted
spell source markers, in addition to targets. All191 base/346 total supplied
numeric templates match independent XML expansion; focused ingestion tests cover
the previously omitted marker paths. All56 portable groups pass. Session/native
script activation, actor room bindings and full constructor/scheduler integration
are still unfinished. TypeAt caches are mutable condition state; a transactional
Session integration must own/copy those caches rather than sharing mutations with
an older snapshot. Whole template/cache lifetime and original save parity remain
unverified.
