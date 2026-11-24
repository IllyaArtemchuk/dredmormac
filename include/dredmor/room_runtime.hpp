#pragma once
#include "dredmor/room_scripts.hpp"
#include <tuple>
#include <iosfwd>

namespace dredmor {
using RoomPrototypeKey=std::tuple<std::uint64_t,std::size_t,std::size_t>;
struct RoomRuntimeState {
    RoomTriggerState triggers;
    // This map belongs to a game snapshot. Shared immutable definitions never
    // acquire mutable TypeAt prototypes while a candidate turn is evaluated.
    std::map<RoomPrototypeKey,ScriptItemType> prototypes;
};
struct RoomRuntimeRoom {
    RoomTriggerRoom trigger;
    Position origin{};
    // Same original template/condition objects share a prototype across rooms.
    // Callers provide stable, nonzero IDs for templates that have scripts.
    std::uint64_t template_identity=0;
};
struct RoomRuntimeHooks {
    std::uint64_t player=1;
    std::function<std::int32_t()> player_room;
    std::function<Position()> player_position;
    std::function<std::optional<ScriptObjectState>(std::int32_t,std::string_view)> find_object;
    std::function<std::optional<ScriptItemType>(std::string_view,std::string_view)> create_item;
    std::function<std::vector<ScriptTileItem>(Position)> tile_items;
    std::function<void(const RoomActionRequest&)> perform;
};
// Compose the recovered level pass, condition loop and action dispatch. Requests
// execute live; the owner must queue safe teleports/destruction at their proper
// GameTick phases and transact state, callbacks and RNG together on failure.
RoomTriggerPass run_room_scripts(const std::vector<RoomRuntimeRoom>& rooms,
    RoomRuntimeState& state,const RoomRuntimeHooks& hooks,bool debug=false);
// Port snapshot payload; immutable room definitions remain caller-owned. Loading
// validates every script/cache reference against that authoritative profile.
void write_room_runtime_state(std::ostream& output,const RoomRuntimeState& state,
    const std::vector<RoomRuntimeRoom>& rooms);
RoomRuntimeState read_room_runtime_state(const XmlNode& node,const std::vector<RoomRuntimeRoom>& rooms);

}
