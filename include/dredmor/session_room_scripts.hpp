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
// A transactional integration phase: live script pass, destruction clear,
// then ordered forced teleports. The caller schedules this at its GameTick
// boundaries. This does not replace Session's provisional outer turn driver.
class SessionRoomScriptExecutor {
public:
    static SessionRoomScriptPass run(Session& session,const SessionRoomScriptProfile& profile,
        SessionRoomObjects& objects,RoomRuntimeState& runtime,const SessionRoomScriptHooks& hooks={});
    static void save(std::ostream& output,const Session& session,const SessionRoomScriptProfile& profile,
        const SessionRoomObjects& objects,const RoomRuntimeState& runtime);
    static SessionRoomScriptSnapshot load(std::istream& input,const SessionRoomScriptProfile& profile,
        std::shared_ptr<const std::vector<ItemDefinition>> definitions={},
        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={},
        std::shared_ptr<const std::vector<SpellDefinition>> spells={});
};
}
