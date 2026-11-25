#pragma once
#include "dredmor/session_game_tick.hpp"
#include "dredmor/session_room_scripts.hpp"

namespace dredmor {
struct SessionRoomGameTickHooks {
    SessionRoomScriptHooks scripts;
    // Called after managed script/destruction/placement work at each boundary.
    // All object IDs in requests/queues use SessionRoomObjects' handle domain.
    // Object Update/Post, visibility, director, respawn, effects and I/O remain
    // explicit bodies. Register immediate creations through supplied objects.
    // Buffer presentation/I/O until run succeeds. Do not mutate committed owners.
    // script_owned is true for the managed room/clear requests and script
    // teleports; other teleports require the supplied placement body.
    std::function<void(Session&,SessionRoomObjects&,RoomRuntimeState&,GameTickState&,const TickRequest&,bool script_owned)> perform;
    std::function<std::uint32_t(Session&,GameTickState&)> ticks;
};
struct SessionRoomGameTickSnapshot {
    Session session;
    SessionRoomObjects objects;
    RoomRuntimeState runtime;
    GameTickState tick;
};
// One prepared level's transaction over Session/RNG, room registry, runtime and
// outer driver. Projects live player/registration/cooldown state and schedules
// script phases at the recovered boundaries. Does not implement virtual AI or
// adopt the native scheduler. Driver effects and counters remain caller-owned.
class SessionRoomGameTickExecutor {
public:
    static SessionRoomScriptPass run(Session& session,const SessionRoomScriptProfile& profile,
        SessionRoomObjects& objects,RoomRuntimeState& runtime,GameTickState& state,const SessionRoomGameTickHooks& hooks);
    static void save(std::ostream& output,const Session& session,const SessionRoomScriptProfile& profile,
        const SessionRoomObjects& objects,const RoomRuntimeState& runtime,const GameTickState& state);
    static SessionRoomGameTickSnapshot load(std::istream& input,const SessionRoomScriptProfile& profile,
        std::shared_ptr<const std::vector<ItemDefinition>> definitions={},
        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={},
        std::shared_ptr<const std::vector<SpellDefinition>> spells={});
private:
    static void validate_snapshot(const Session& session,const SessionRoomObjects& objects,const GameTickState& state);
};
}
