#pragma once
#include "dredmor/session_room_objects.hpp"
#include "dredmor/session.hpp"

namespace dredmor {
struct SessionRoomScriptProfile {
    explicit SessionRoomScriptProfile(DungeonLayout grid):layout(std::move(grid)) {}
    DungeonLayout layout;
    std::vector<RoomRuntimeRoom> rooms;
    // Room index -> program. Runtime script vectors must alias these immutable
    // programs, retaining the one-based local Create record handles.
    std::vector<std::shared_ptr<const RoomScriptProgram>> programs;
    // Authoritative source nodes, including Create record bodies. Each program
    // is linked from its corresponding source; do not edit linked definitions
    // independently. Sources contribute to the saved profile signature.
    std::vector<XmlNode> sources;
};
struct SessionRoomScriptSnapshot {
    Session session;
    SessionRoomObjects objects;
    RoomRuntimeState runtime;
};
struct SessionRoomScriptHooks {
    // Optional constructor boundary for NPCs, loot, traps and other single
    // records not handled by the default custom/lever/pedestal constructors.
    // Mutations must stay in the supplied candidate Session; definitions and
    // resource callbacks are immutable. Returned objects are already placed.
    std::function<std::optional<SessionRoomObjectRef>(Session&,const RoomRecord&,Position)> create_record;
    // Queue through Session::queue_room_horde; do not flush placements here.
    // The executor merges each group's forced placements with script moves.
    std::function<void(Session&,const RoomHordeRecord&,Position)> queue_horde;
    std::function<std::optional<ScriptItemType>(Session&,std::string_view,std::string_view)> create_prototype;
    std::function<std::vector<ScriptTileItem>(const Session&,Position)> tile_items;
    std::function<BreakableFrames(const BreakableRoomDefinition&)> breakable_frames;
    // Scripted spell setup mutates actor/effect state; it is not merely a
    // presentation request and cannot be silently skipped by the adapter.
    std::function<void(Session&,const RoomActionRequest&)> prepare_spell_actor;
};
struct SessionRoomScriptPass {
    RoomTriggerPass triggers;
    // Ticker/audio presentation requests are delivered only after the
    // candidate succeeds. Their actual native rendering is caller-owned.
    std::vector<RoomActionRequest> presentation;
    bool player_teleported=false;
};
struct SessionRoomScriptMove {std::uint64_t subject=0;Position point{};bool reserved=false;};
enum class SessionRoomScriptStage {idle,evaluated,cleared,finished};
struct SessionRoomScriptQueue {
    SessionRoomScriptPass pass;
    std::vector<std::uint64_t> destruction;
    std::vector<SessionRoomScriptMove> moves;
    std::uint64_t level=0;
    std::size_t next_move=0;
    SessionRoomScriptStage stage=SessionRoomScriptStage::idle;
};
// A transactional integration phase: live script pass, destruction clear,
// then ordered forced teleports. The caller schedules this at its GameTick
// boundaries. This does not replace Session's provisional outer turn driver.
class SessionRoomScriptExecutor {
public:
    static SessionRoomScriptPass run(Session& session,const SessionRoomScriptProfile& profile,
        SessionRoomObjects& objects,RoomRuntimeState& runtime,const SessionRoomScriptHooks& hooks={});
    // Staged driver interface. The caller owns one candidate containing Session,
    // bindings, runtime and queue, and discards all four on any failure. Scripts
    // queue mutations; the driver clears destruction, runs Post/cooldowns, then
    // places each move and finishes. Saves/floor snapshots reject an active phase.
    static SessionRoomScriptQueue evaluate(Session& candidate,const SessionRoomScriptProfile& profile,
        SessionRoomObjects& objects,RoomRuntimeState& runtime,const SessionRoomScriptHooks& hooks={});
    static void clear_destruction(Session& candidate,SessionRoomObjects& objects,SessionRoomScriptQueue& queue);
    static void place_next(Session& candidate,SessionRoomObjects& objects,SessionRoomScriptQueue& queue);
    static SessionRoomScriptPass finish(Session& candidate,const SessionRoomObjects& objects,SessionRoomScriptQueue& queue);
    static void save(std::ostream& output,const Session& session,const SessionRoomScriptProfile& profile,
        const SessionRoomObjects& objects,const RoomRuntimeState& runtime);
    static SessionRoomScriptSnapshot load(std::istream& input,const SessionRoomScriptProfile& profile,
        std::shared_ptr<const std::vector<ItemDefinition>> definitions={},
        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={},
        std::shared_ptr<const std::vector<SpellDefinition>> spells={});
private:
    static SessionRoomObjectRef reference(const SessionRoomObjects& objects,std::uint64_t identity);
    static CustomRoomObject& custom(Session& session,SessionRoomObjectRef reference);
};
}
