#pragma once
#include "dredmor/generation.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace dredmor {
struct RoomObjectIdentity {
    std::uint64_t identity=0,level=0;
    std::int32_t assigned_room=-1;
    std::string id;
    bool global=false;
    Position position{};
};
struct RoomObjectLookup {
    std::optional<std::uint64_t> object;
    std::size_t tested=0;
};
bool matches_room_object(const RoomObjectIdentity& object,std::uint64_t level,std::int32_t room,std::string_view id);
RoomObjectLookup find_room_object(const std::vector<RoomObjectIdentity>& registered,std::uint64_t level,std::int32_t room,std::string_view id);
struct RoomIdAssignment {
    RoomObjectIdentity object;
    RoomObjectLookup lookup;
    bool lookup_performed=false,debug_requested=false;
    std::optional<std::uint64_t> duplicate;
};
// SetRoomID writes the label and derives the assigned room from the owning
// level's tile byte (not RoomInfo identity). Registered references to this same
// object see the new values during the duplicate query. A conflict requests a
// diagnostic; original fatal roomError behavior is an external boundary.
RoomIdAssignment assign_room_object_id(RoomObjectIdentity object,std::string id,const DungeonLayout* owner,
    const std::vector<RoomObjectIdentity>& registered,bool debug=false);
}
