# Live turn-driver integration

The outer GameTick control flow is recovered from the supplied i386 executable
at0x2b480 and compared separately with original executions. Its virtual object,
AI, director, visibility and platform callbacks are explicit subsystem boundaries.
The native view still uses the provisional Session action scheduler. These APIs
are integration work toward replacing it, not a claim of complete gameplay.

`SessionGameTickExecutor::run` transacts Session and GameTickState together, using
Session's candidate-owned libc/MT/normal-cache state for driver and callbacks.
Callbacks must mutate their supplied candidates and buffer I/O until success.
Nested entry and unfinished staged scripts/horde placements reject. The caller
owns synchronization with external registries and any driver/player projections.

`SessionRoomGameTickExecutor::run` additionally owns the room binding registry and
RoomRuntimeState candidates for one prepared level. It projects the registered
object order and live player state before/after callbacks. Its cooldown vector
uses every Session spell definition's index, preserving zero and negative values;
positive counters are decremented by the outer driver after PostGameTick and
published back to Session before teleports/clock/autosave access. Changing levels
inside this prepared-level adapter rejects.

The room adapter evaluates scripts at room_triggers, clears their destruction
queue at clear_destruction, and merges their forced moves after pre-existing
teleports. Destroyed script objects also lose pre-existing queued moves. Managed
moves run through the existing staged placement API. The phase finishes before
tutorial/clock/autosave access even when there are no moves. The returned script
presentation pass is available only after all candidate owners commit.

Every request is exposed to the subsystem hook. Its script_owned argument is true
for the room/clear phases and managed script teleports; other teleports still
need the supplied placement body. Immediate creations must be registered through
supplied SessionRoomObjects so later Update/Post loops observe source order.
The registry, player and cooldown projections belong to the adapter; callbacks
should mutate Session/registries rather than overwrite those driver projections.
Action-owner breakable events must be forwarded to room events before evaluation,
as in the current native script adapter. No provisional enemy AI is silently
substituted for an unimplemented object Update/Post body.

GameTickState's standalone version1 XML retains all driver fields, including
ordered duplicate effect memberships and retained duration-map entries. The
SessionGameTickExecutor save/load wrapper stores Session/RNG and driver state
atomically and checks their geometry/player agreement. Independent room binding,
runtime and resource registries remain owner-managed. These port formats do not
claim compatibility with original saves. Structural, numeric, text, size and
effect-reference validation completes before bytes are exposed to a writer.

Portable checks cover64 shared-RNG constructor cases, complete late-failure
rollback,110tick save/reload continuation, registered objects appended during
Update, new objects receiving Post, source script/destruction/cooldown/movement
order, pending teleport cancellation, unsigned autosaves and updates-player gates.
Virtual subsystem bodies in the composed fixtures are controlled boundaries;
full Monster::Update/AI, clock/effect lifetimes, dungeon generation and native
scheduler adoption remain work in progress.
