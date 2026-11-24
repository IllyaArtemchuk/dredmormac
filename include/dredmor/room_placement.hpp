#pragma once
#include "dredmor/room_scopes.hpp"
#include <map>
#include <tuple>

namespace dredmor {
struct RoomSquareIdentity {
    std::uint64_t level=0;
    Position position{};
    bool operator<(const RoomSquareIdentity& other) const {
        return std::tie(level,position.x,position.y)<std::tie(other.level,other.position.x,other.position.y);
    }
};
struct RoomLevelRegistry {
    int width=0,height=0;
    std::vector<std::uint64_t> objects,monsters;
};
struct RoomPlacementObject {
    RoomObjectIdentity room;
    bool monster=false;
    std::optional<RoomSquareIdentity> square;
};
struct RoomPlacementWorld {
    std::map<std::uint64_t,RoomLevelRegistry> levels;
    std::map<RoomSquareIdentity,std::vector<std::uint64_t>> squares;
};
// These mirror the base Level registration helpers, not Monster::TeleportTo.
// AddObj appends unconditionally; RemoveObject erases only the first occurrence.
void register_room_object(RoomLevelRegistry& level,std::uint64_t identity);
void unregister_room_object(RoomLevelRegistry& level,std::uint64_t identity,bool monster);
// Base GameObj::TeleportTo preserves the assigned room/name. Cross-level moves
// remove old registration and append new registration; same-level moves repair
// missing registration. Existing tile memberships are removed/attached in order.
// Null/missing destination levels are rejected before changes (port input guard).
void teleport_room_object(RoomPlacementObject& object,RoomPlacementWorld& world,
    Position destination,std::uint64_t level);
}
