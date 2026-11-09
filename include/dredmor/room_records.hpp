#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/xml.hpp"
#include <variant>
#include <functional>

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
enum class CustomRoomKind { blocker,engraving,breakable,pedestal };
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
struct BreakableRoomDefinition {
    RoomObjectPlacement placement;
    RoomSprite intact,broken;
    std::optional<RoomSprite> breaking;
    std::string name,description;
    int hits_needed=1,hits_taken=0;
    // Applied by CustomRecord's virtual finalizer after construction.
    std::optional<bool> passable,vis_block;
};
struct BreakableFrames {std::uint16_t intact=1,breaking=1,broken=1;};
struct BreakableRoomObject {
    CustomRoomObject object;
    RoomSprite intact,breaking,broken;
    BreakableFrames frames;
    int hits_needed=1,hits_taken=0;
    std::uint16_t single_breaking_delay=0;
};
struct BreakableHit {
    bool handled=false,broken_now=false;
    std::vector<std::string> triggers;
    std::optional<RoomSprite> animation;
};
BreakableRoomDefinition read_breakable_room_object(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
std::optional<BreakableRoomObject> create_breakable_room_object(const BreakableRoomDefinition& definition,
    Position origin,DarwinRandom& random,BreakableFrames frames={});
std::optional<BreakableRoomObject> create_breakable_room_object(const BreakableRoomDefinition& definition,
    Position origin,DarwinRandom& random,const std::function<BreakableFrames(const BreakableRoomDefinition&)>& resources);
// Basic custom breakables contain no item. Trigger/animation dispatch belongs to
// the owner; this applies the original hit count, frame and collision transition.
BreakableHit hit_breakable(BreakableRoomObject& object);
struct RoomPedestalRecord {RoomObjectPlacement placement;std::string name,text;};
RoomPedestalRecord read_room_pedestal(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
std::optional<CustomRoomObject> create_room_pedestal(const RoomPedestalRecord& definition,Position origin,DarwinRandom& random);
struct RoomMonsterRecord {RoomObjectPlacement placement;std::string name;};
struct RoomTrapRecord {RoomObjectPlacement placement;std::string name;};
struct RoomLootRecord {RoomObjectPlacement placement;std::string type,subtype="random";int amount=1;bool artifact=false;};
struct RoomHordeRecord {RoomObjectPlacement placement;std::string name;int minimum=-1,additional=-1;};
using RoomPopulationRecord=std::variant<RoomMonsterRecord,RoomTrapRecord,RoomLootRecord,RoomHordeRecord>;
enum class PopulationRequest {none,named_monster,random_monster,named_trap,random_trap,loot,horde};
struct RoomPopulationPlan {
    PopulationRequest request=PopulationRequest::none;
    Position position{};
    std::string name,type,subtype;
    int amount=1,minimum=-1,additional=-1;
    bool artifact=false;
};
RoomPopulationRecord read_room_population(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
// Ordered request planning only: database resolution, constructors, loot factory,
// random spawn/horde selection, entity flags and finalization remain external.
RoomPopulationPlan plan_room_population(const RoomPopulationRecord& record,Position origin,DarwinRandom& random);
struct UnimplementedRoomRecord {std::string type;XmlNode source;};
using RoomRecord=std::variant<CustomRoomDefinition,BreakableRoomDefinition,RoomPedestalRecord,RoomElementDefinition,
    RoomMonsterRecord,RoomTrapRecord,RoomLootRecord,RoomHordeRecord,UnimplementedRoomRecord>;
}
