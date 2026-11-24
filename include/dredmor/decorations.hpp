#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/xml.hpp"
#include "dredmor/room_records.hpp"

namespace dredmor {
enum class RoomDecorationKind : int {painting=1,statue=2,floor=3};
struct RoomDecorationDefinition {std::string subject;RoomDecorationKind kind;std::string sprite;};
struct RoomDecorationPlacement {RoomDecorationKind kind;Position position;std::string sprite;};
// Noun registration order is painting, statue, floor, independent of XML
// attribute order. Empty sprite values are not registered; subject is plural.
std::vector<RoomDecorationDefinition> read_room_decorations(const XmlNode& text);
// Original per-room conditional decoration traversal, after explicit room
// records. Marker arrays belong to the template, not the punched-out live grid.
// No RNG/constructors/placement side effects; those must execute in plan order.
std::vector<RoomDecorationPlacement> plan_room_decorations(const RoomStamp& room,
    Position origin,std::string_view subject,const std::vector<RoomDecorationDefinition>& table,
    std::size_t max_placements=1000000);
// Named-sprite Blocker/Engraving constructor metadata: stationary frame0,
// movement-blocking paintings/statues, passable floor engravings, no vision
// blocking. Sprite I/O and global object registration belong to the level.
CustomRoomObject make_room_decoration(const RoomDecorationPlacement& placement);
}
