#pragma once
#include "dredmor/room_placement.hpp"
#include <array>
#include <functional>

namespace dredmor {
struct RoomDeferredState {
    RoomPlacementWorld placement;
    std::map<std::uint64_t,RoomPlacementObject> objects;
    std::vector<RoomTeleport> teleports;
    std::vector<std::uint64_t> destruction;
    std::uint64_t current_level=0,picked_item=0,active_item=0;
    std::array<std::uint64_t,9> belt{};
    std::array<std::uint64_t,48> bag{};
};
struct RoomDeferredHooks {
    // A subclass destructor/teleport can replace the base implementation.
    // It owns its mutations and can append requests to the same live queues.
    std::function<void(std::uint64_t)> destroy;
    std::function<void(const RoomTeleport&,bool)> teleport;
};
struct RoomDeferredResult {
    std::vector<std::uint64_t> destroyed,detached;
    std::vector<RoomTeleport> performed;
};
void queue_room_teleport(RoomDeferredState& state,RoomTeleport teleport);
void remove_room_teleports(RoomDeferredState& state,std::uint64_t object);
void queue_room_destruction(RoomDeferredState& state,std::uint64_t object);
// Base GameObj deleting destructor's level/tile/queue lifetime; Bob/resource
// destruction and storage deallocation remain owner boundaries.
void destroy_room_object_base(RoomDeferredState& state,std::uint64_t object);
RoomDeferredResult clear_room_destruction(RoomDeferredState& state,const RoomDeferredHooks& hooks={});
RoomDeferredResult fix_room_teleports(RoomDeferredState& state,const RoomDeferredHooks& hooks={});
}
