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

Room object lookup uses the ordered Level object vector at+0xb0, shared with
GameTick updates and base teleport/destructor registration. The earlier note
describing a separate main object list was incorrect. Level::ObjectMatchingRoomID
0x609ac calls MatchesRoomID0x36aee:
owner-level equality, then either IsGlobalRoomObject or assigned-room equality,
then exact label equality. Player's global override allows cross-room lookup
within its own level. No alive/deferred-destruction predicate runs here.

GameObj::SetRoomID0x36d4a first writes its new label and resets assigned room to-1.
An in-bounds owner grid supplies tile byte+1, independently of RoomInfo identity.
A nonempty name queries the registry after this write, so a registered self can
match before a later duplicate. An earlier other object requests roomError;
this port reports the duplicate and does not claim original fatal-handler parity.
Debug flag8 logs nonempty names even without an owner level.

verify_room_scopes.py executes those three full original routines on4,096 lookup
and4,096 assignment fixtures, comparing tested counts, resolved identities,
assigned scope/name, self aliasing, diagnostic requests and debug payloads.
String/allocation helpers, global-kind query and returning error logger are
controlled boundaries. Registration lifetime, teleport and scheduler integration
are separate. Original binary SHA256 remains
`a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
All57 portable groups pass.

Base placement now preserves the original lifetime distinction: GameObj::TeleportTo
0x36e80 changes coordinates/owner/tile attachment, but never recalculates assigned
room or name. Same-level movement registers an absent object. A cross-level move
calls old Level::RemoveObject0x610e0, new Level::AddObj0x6108e, then assigns owner.
AddObj always appends, including duplicates; RemoveObject removes only the first
registry occurrence and, for IsMonster, the first monster-vector occurrence.
AddObj itself does not append to the monster vector. Tile removal erases the first
matching membership; attachment avoids appending if already present. Outside-grid
coordinates clear the square pointer while retaining owning-level registration.

verify_room_placement.py matches8,192 complete actual base teleports, including
actual registration helpers and all tile/registry contents. List insertion/unlink,
allocation, strings and IsMonster are boundaries; spare capacity avoids vector
reallocation. Null/missing destination levels are rejected as a port guard.
SafeTeleport0x29990 merely queues requests; FixSafeTeleports0x29bc0 clears pending
destruction before executing teleports with final flag1. This base implementation
does not claim immediate script movement, subclass override or full scheduler
parity. All58 portable groups pass.

Clock record metadata is now typed in RoomDefinition::records. Actual
RoomClockRecord::UpdateReadWriter0x1a070e extends CustomRecord fields with required
integer initial, optional integer remaining(default-1, with a distinct presence
flag), and optional started(defaultfalse). The common sprite property is required
in addition to the object point. Name/description, ID, percent/can_push and optional
passable/visblock are retained. ASCII case-insensitive attribute aliases follow
source order. Invalid unused attributes/child bodies use explicit port guards;
room marker mutation commits only after a successful parse.

verify_room_clock_metadata.py executes the actual record factory, constructors,
virtual property registration and complete attribute reader on2,048 fixtures;
all typed fields/default/presence flags and every numeric template cell match.
Map/string/ASCII conversion/library operations are boundaries. This covers record
metadata, not full Clock object construction or finalization. Original CreateInLevel
0x1a0ba0 has no percent roll; finalizer0x1a0dae applies base placement/ID, collision
overrides, then calls SetTimeRemaining again when the optional remaining attribute
was present. Constructor default-1 resolution therefore differs from a finalizer
assignment of explicit-1. Constructor event timing and preinitialized storage need
separate evidence before full construction parity can be claimed. All58 portable
groups and ingestion of2,123 supplied base/expansion records pass.

The complete outer GameTick0x2b480 driver is now portable. It first clears the
inventory announcement flag and calls FixSafeTeleports, advances the slow-time
counter (reset only when incremented value equals2), clears gameMessage, and
returns early if no current level. Otherwise it awards zero XP, calculates
visibility, iterates live registered objects' Update virtuals while player dead
field+0x704 remains zero, and conditionally updates player buffs/triggers using
Level byte+0x60. Room triggers precede destruction clearing and DirectorTick.

After those phases it decrements finite tile-effect durations and removes every
matching membership when a duration reaches zero. Shared/duplicate memberships
observe live duration changes. Conditional player respawn counter wraps/increments;
at signed>=101 it subtracts100 and requests SpawnMonster(-1,-1,0,0,1,1).
Visibility recalculates after these effects. An unconditional MT rejection draw
(up to0xfffffed7) gates rare colour output at<=0x83126d, then a libc selection
when tutorial is off and the queue is nonempty. The colour queue always clears.

Synchronized Bobs play/clear, every live registered object gets PostGameTick,
positive spell cooldowns conditionally decrement, and nonzero player delay wraps
down even if negative. Every queued teleport then invokes virtual TeleportTo with
final flag1; the queue clears. Tutorial logic precedes the unsigned300001ms
autosave check. A living player requests SaveGame("autosave.dredmorsave",1), a
second GetTicks value becomes the last save timestamp, and ticker reports
"Game autosaved.". SteamAPI_RunCallbacks runs last. Full subsystem implementations
and actual disk/platform actions are external callbacks, not performed by this
portable driver on their own.

verify_game_tick.py executes the entire original wrapper on8,192 fixtures with
recorded subsystem/virtual boundaries. Every request, live append/move/death,
tile effect, spell cooldown/delay, respawn and autosave gate, random state and
next8 MT values match. The fixtures include684 respawn,3533 autosave,14947
end-of-tick teleport,11863 Update and20483 PostGameTick requests. Stable current
level and preallocated registry storage are fixture assumptions; replacing a
level during registry iteration is not claimed. Focused C++ tests compose clock
expiry with trigger observation and delayed movement. All59 portable groups pass.
Session/native adoption and full subsystem behavior remain unfinished.

Ordinary Lever type0 now toggles ActivationState through actual-derived FlipLever
0x7b86e behavior. It emits pull text and sound"lever"; an initially inactive
lever in tutorial mode is activated before dispatch and then toggles off. Previous
and older history words stay untouched by a flip. Actual Clicked0x7b77c,
RemoteActivation0x7c36a and bump0x7c20c dispatch/cardinal/held-item gates match on
8,192 full original fixtures via verify_script_levers.py. Ticker/sound/tutorial
are controlled boundaries; no libc or MT draw occurs. Other lever types retain
their separate evidence (secret type1 already implemented).

Session ordinary levers use RoomElement chance planning, constructor0x7c00e/
0x7c0e8 constants (blocking, no vision block, Lever name, dungeon/lever1.spr), and
Draw0x7b74e activation frame0/1. Resource factory construction is not yet a full
original-routine comparison. The Session owns collision, click/bump interaction,
ID/can_push metadata, history advancement and cached-floor transfer. Save22
preserves full current/previous/older state and object data; malformed history,
appearance or missing state is rejected. Save1..21 remains readable and untouched
when ordinary levers are absent. Session tests cover canonical roundtrip,
continuation, held-item rejection, floor cache and creation failure rollback.
All60 portable groups pass. Full object ordering/room script/tick-driver adoption
and native controls remain work.

Native RoomView now instantiates ordinary RoomElement typelever records in source
order using Session creation, prepares dungeon/lever1.spr with at least two frames,
and draws current activation. Mouse clicks and movement bumps use the ordinary
lever Session path. Loaded ordinary lever count/position/ID/can_push must match the
room before resource preparation and state/presentation commit. Save22 restores
activation/history while preserving the earlier floor/load rollback architecture.
The --script-levers synthetic Mac fixture covers click, bump, history settling,
save/restore, distant click and malformed-history rollback. Local portable checks
and fixture generation pass; arm64 native validation is pending for this source.
No original VFX/audio/UI or full script/tick integration claim is made.

The composed RoomRuntime now runs the recovered level trigger pass with live
object/player/action hooks. Conditions see current queued events and prior-room
state; actions execute in source order through the owning world's request handler.
Template IDs preserve prototype sharing across room instances. TypeAt conditions
are copied for evaluation and populate a snapshot-owned cache; immutable shared
definitions never acquire a prototype. A prototype constructed before a false
condition remains cached in that candidate. Stable nonzero template IDs and
consistent shared definition references are explicit profile guards.

Runtime snapshot XML preserves previous-room state, success counters, queued
events and prototype kind/subtype pairs. Loading validates every script/TypeAt
reference, rejects duplicate entries and checks a deterministic profile hash of
immutable room origins/identity/template mappings and typed conditions/actions.
Current room fill/presence and owner world objects remain caller-owned. XML strings
preserve escaped whitespace. This component format is not an original-game save.

Composition tests cover activation/event visibility across scripts, per-room
repeat counts, cross-room shared prototypes, false-condition cache retention,
immutable source state, saved continuation, changed-profile rejection and an
injected action failure followed by a retry with the same constructor RNG draw.
All61 portable groups pass. This is composition/ownership verification of already
compared kernels, not a new full original-world or Session integration oracle.
The owner must transact world/runtime/RNG together and process safe teleport and
destruction requests in their verified GameTick phases. Session/native script
binding, created records, full scheduling and original save compatibility remain
unfinished.

Deferred object lifetime now follows SafeTeleport0x29990,
RemoveFromSafeTeleportList0x299f0, AddToDestructionList0x2ba60,
ClearDestructionList0x29a50 and FixSafeTeleports0x29bc0. SafeTeleport preserves
ordered duplicate requests. Explicit removal deletes all matching requests.
Destruction enqueue clears a matching picked item even for a duplicate enqueue,
but adds each object only once. Clearing first removes one pending teleport,
removes square membership, calls Level::RemoveObject on the global current level,
clears all9 belt/48 bag references plus active/picked pointers, then invokes the
virtual deleting destructor. The live destruction vector can grow during callbacks.

The base deleting GameObj destructor0x37510 removes every remaining pending
teleport, clears its square pointer, erases only the first main-registry entry
from its own owning level, and clears that owner. It does not remove a monster
vector entry in this base phase. Bob/resource destruction and storage release
remain boundaries; subclass lifetime is an explicit replaceable hook.

FixSafeTeleports clears destruction first. It skips null object requests, invokes
nonnull-level TeleportTo with final flag1, or detaches for a null destination
level. A null-level request skips detachment when either requested coordinate is-1
and either existing coordinate is-1. Otherwise it removes square membership and
owning-level registration and clears square/owner while retaining coordinates.
The queue clears after all requests, including callback-appended entries. Dangling
objects and invalid null attachment contracts use explicit port guards.

verify_room_deferred.py executes all those complete original routines, actual
base deleting destructor, Square removal, Level registration helpers, base
TeleportTo and picked-item accessors on8,192 fixtures. It compares11232 destructor,
3918 teleport and1565 detach traces, every queue/inventory/object/registry/tile state,
and no libc/MT use. Duplicate registrations/memberships and mismatched current vs
owner levels are included. Allocation/list/string helpers, IsMonster, Bob/resource
destruction and base storage release are supplied. Callback-appended work is also
covered by focused portable tests. All62 portable groups pass. Full subclass,
Session/native script/tick and save integration remain unfinished.

RoomScriptProgram now owns typed script lists and the ordered records requested
by Create actions. A shared read_room_record schema serves placed and created
records. The created-record reader enforces case-sensitive create_type values,
as RoomObjectRecordFrom0x1a3ef0 compares exact strings; placed record tag handling
retains its existing policy. Each Create gets a one-based record handle. GameContent
links spells to one-based catalog handles and their cast sound. Every action
request also carries its source room index so owners can route local handles.
Unknown spell/record references fail explicitly; pending constructor kinds retain
their source rather than being silently executed.

spell_cast_sound reads top-level anim/sfx updates in source order. Original
spell-start initialization0x7ca8b sets Spell+0x138 to an empty string; the anim
branch0x7fb44 assigns sfx at0x7fcf6. verify_spell_cast_sound.py matches2,048 actual
spellDBXMLLoader_StartElement callback fixtures for repeated/absent/empty/case-
aliased sound assignments, with string/ASCII helpers and a preallocated Spell.
Other animation fields, non-anim/nested parsing, full spell registration/build
and audio playback remain outside this comparison.

verify_room_programs.py independently parses supplied XML and checks every room's
script/condition/route/spell/Create counts against linked programs. All346 rooms
agree:62 scripts,100 conditions,235 success actions,zero failure actions,76 spell
references and55 created records. All linked handles are range-checked. This is
content/linkage verification, not original world/constructor parity.

The linked-room-script-puzzle test composes real XML program parsing, ordinary
activation history, ordered object scope lookup, live runtime conditions/actions,
base custom object creation, the full outer tick driver and deferred lifetime.
It confirms pending-removal lookup in a later script of the same pass, new object
PostGameTick before its first Update, destruction before post-tick, queued player
movement last, per-room repeat exhaustion and removal visibility on the next tick.
All63 portable groups pass. Native Session adoption, complete record constructors,
full actor/effect behavior, generation and original save compatibility remain work.

## Forced actor placement and initial room entry

Forced Monster::TeleportTo0xe5fb0 and Player::TeleportTo0x11c49a now match4,096
complete original cases each. Monster maintains the +0xd4 monster vector as well
as the shared +0xb0 object vector: cross-level moves append even if a duplicate
exists; same-level moves repair missing membership. Assigned room/name remain
unchanged. Player delegates base placement with flag0 and always calls
Level::CalcVisibility0x114824 afterwards; its caller's nonzero forced flag then
skips movement-only effect removal and adjacent money pickup. Both accept outside
coordinates at this wrapper boundary. Visibility, ordinary movement's effects/
pickup/contact paths and Session integration are separate.

Compared every ordered registry/monster vector and tile list, owner/coordinates,
assigned room/name, unchanged libc/full MT and player visibility arguments plus
state at that callback. Allocator/string/list/IsMonster/visibility bodies are
boundaries. See scripts/verify_actor_placement.py. All68 portable groups pass.

Full Level constructor0x67186 against dirty storage confirms previous room at
+0xf4 initializes to-1 (write0x67373). RoomTriggerState/RoomScriptContext defaults
now retain that value, so Entered on room0 fires on the first pass; tests cover
staying, leaving and re-entry. Player constructor's assignment0x125dd2 installs
room ID "player" atGameObj+0x70; this is a source observation for the future live
Session binding, not a full Player constructor comparison. Native horde source
f42a50e passed arm64 Mac run45389291770 and relocated packaged-app smoke.

Room runtime snapshots now optionally validate against a game-wide immutable
RoomRuntimeCatalogue. TypeAt prototypes for templates absent from the active
floor remain valid and contribute to the profile signature; counters/history
still validate only against the current floor. Tests travel between disjoint
room sets, save/reload both, reuse the first prototype without another factory
call and reject changed absent templates, unknown references, missing/null/zero
catalogue entries and conflicting template pointers. Existing single-floor
payload signatures remain unchanged. All68 portable groups pass. Latest native
actor-placement source a6578f5 passed Mac run45393289621. This is runtime ownership
infrastructure; Session/native scripts and full game remain incomplete.

SessionRoomObjects now binds measured room lookup/finalization to live typed
Session storage while retaining explicit cross-type registration order and
opaque object identities independent of actor/item IDs. Positions, monster
liveness and lever history resolve live; assigned tile-room bytes and labels
remain binding-owned. Source player ID defaults to "player" and is global.
Non-finalized horde registrations retain empty IDs/assigned room -1. Value
snapshots and a validated standalone binding payload preserve order, sequence,
scopes and references. Missing typed objects cannot match lookup; saving stale
bindings fails until the lifetime owner unregisters them. Index-based static
object references require adjustment by the future deletion owner; this layer
does not yet run native scripts or automate Session construction/lifetime.
All69 portable groups pass, including live Session activation through the full
RoomRuntime, cross-type duplicate lookup, room-byte capture, movement, snapshot
isolation, session+binding reload and malformed reference guards. Full game is
still incomplete; next connect action/deferred phases and immutable profiles.

SessionRoomScriptExecutor now runs the recovered RoomRuntime against actual
Session storage in one candidate transaction. Default Create supports custom
objects, pedestals, ordinary levers and resource-backed breakables. It preserves
same-pass visibility of queued removals, de-duplicates destruction, removes all
teleports targeting destroyed objects, adjusts remaining static typed indices
without changing opaque IDs, and applies surviving forced moves in queue order.
Passable flags mutate live objects. Scripted spell actor/effect preparation and
other constructors remain explicit hooks, not ignored side effects. Unsupported
clock/horde/trap/keeper lifetime paths and out-of-grid or unpersistable wall
placements fail atomically. Ticker/audio requests are returned after success.
All70 portable groups pass. The real-Session XML puzzle covers pending deletion,
immediate Create visibility, repeated moves, final index-shift correctness,
pre-teleport previous-room capture, saved component continuation, late failure
rollback including RNG and resource failure rollback. This phase is not yet
scheduled by the native/provisional Session turn loop, and component saves are
still caller-owned. Next add authoritative profile/save and native puzzle wiring.
Mac source2da1b85 passed run45396650040; catalogue75bf473 passed45396105118.
Full outer GameTick, native scripts, effects/AI/generation/UI/audio/game unfinished.

Prepared native rooms can now opt into room_scripts. Source-order registration
binds supported placed records, then player; keyboard movement/wait and ordinary
lever mouse/bump interactions run the transactional Session script phase and
preflight all changed sprites before committing Session/bindings/runtime together.
Ticker requests appear in the room HUD. New combined scripted-room save wrapper
persists Session, bindings and runtime and validates immutable XML sources,
linked scripts/actions and numeric room geometry. Changed Create bodies (whose
local record handles do not change) are covered by the authoritative source hash.
Core save versions remain unchanged; ordinary sessions keep their prior format.
Native scripted-room load permits saved script-driven lever changes and rejects
bad resources before replacing live state. The new synthetic native puzzle tests
mouse/bump equivalence, same-pass pending gate visibility, Create/player teleport,
initial/solved reload, following-pass destruction visibility, failed-constructor
sprite preparation rollback and malformed saved sprite rollback.
All70 portable groups pass. The synthetic native fixture generates successfully
outside git; Mac build/window validation pending this commit. Previous executor
11519f2 passed Mac run45397687724. Prepared scripting remains opt-in and single
floor; native action scheduling still uses provisional Session turns (including
its existing cooldown phase), not the full verified GameTick driver. Other action
sources, carried-object ownership, breakable events, clocks, all constructors,
horde/trap/keeper lifetime, visibility body, full AI/generation/effects/UI/audio
and the complete game remain unfinished.
