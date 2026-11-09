#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/xml.hpp"

namespace dredmor {
using RoomAttributes=std::map<std::string,std::string,std::less<>>;
using RoomMarkers=std::map<char,Position>;
struct RoomSprite {
    std::string image,series;
    int first=0,count=0,rate=0;
};
// SpriteRecord-style property: series Animate is consumed but has no destination
// in the original custom-object records. Single image conflicts with any series key.
std::optional<RoomSprite> take_room_sprite(RoomAttributes& attributes,std::string_view prefix="png");
// PointProperty::SetFrom behavior for valid ASCII metadata. Consumes matching
// attributes; at resolves a marker and immediately writes ordinary floor.
// Explicit x/y do not change the template. Missing coordinates return nullopt.
std::optional<Position> take_room_point(RoomAttributes& attributes,std::string_view prefix,
                                       const RoomMarkers& markers,RoomStamp& room);
struct RoomObjectPlacement {
    Position relative{};
    int percent=100;
    std::uint8_t can_push=1;
    std::string id;
    XmlNode source;
};
struct RoomElementDefinition {
    RoomObjectPlacement placement;
    std::string type,name,contents;
};
enum class RoomElementKind {
    none,bookshelf,lever,lutefisk_statue,portal,anvil,statue,dredmor_statue,
    evil_chest,freezer,bbq,fountain,diggle_god,dispenser,dirt,wizard_portal,
    wizard_controls,wizard_pedestal,wizard_graffiti,wizard_key
};
struct RoomElementPlan {
    bool accepted=false;
    RoomElementKind kind=RoomElementKind::none;
    int variant=-1;
    Position position{};
    std::string name,contents;
};
RoomElementDefinition read_room_element(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
// Creation dispatch only. Constructors/finalization can consume additional RNG
// and have entity effects that are not performed by this planning function.
RoomElementPlan plan_room_element(const RoomElementDefinition& definition,Position origin,DarwinRandom& random);
enum class CustomRoomKind { blocker,engraving };
struct CustomRoomDefinition {
    RoomObjectPlacement placement;
    CustomRoomKind kind=CustomRoomKind::blocker;
    RoomSprite sprite;
    std::string name,description;
    std::optional<bool> passable,vis_block;
};
struct CustomRoomObject {
    CustomRoomKind kind=CustomRoomKind::blocker;
    Position position{};
    RoomSprite sprite;
    std::string name,description,room_local_id;
    std::uint8_t can_push=1;
    bool blocks_movement=false,blocks_vision=false;
    std::uint32_t frame=0;
    bool playing=true,looping=true;
};
CustomRoomDefinition read_custom_room_object(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
// Basic constructor/metadata/collision state. Sprite I/O, tile/list registration,
// room-id binding and subsequent interactions belong to the owning level.
std::optional<CustomRoomObject> create_custom_room_object(const CustomRoomDefinition& definition,Position origin,DarwinRandom& random);
}
