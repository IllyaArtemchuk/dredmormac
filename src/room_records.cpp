#include "dredmor/room_records.hpp"
#include "dredmor/legacy_number.hpp"
#include <stdexcept>
#include <cstring>

namespace dredmor {
namespace {
std::string lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=char(c-'A'+'a');
    return value;
}
std::optional<std::string> attribute(const XmlNode& node,std::string_view key) {
    for (const auto& entry:node.attributes) if (lower(entry.first)==key) return entry.second;
    return std::nullopt;
}
int add(int a,int b) {
    const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;
    std::memcpy(&result,&bits,sizeof(result));return result;
}
auto find_attribute(RoomAttributes& attributes,const std::string& key) {
    const auto wanted=lower(key);
    for (auto it=attributes.begin();it!=attributes.end();++it) if (lower(it->first)==wanted) return it;
    return attributes.end();
}
RoomObjectPlacement placement(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    auto attributes=node.attributes;const auto point=take_room_point(attributes,"",markers,room);
    if (!point) throw std::invalid_argument("room object point required");
    RoomObjectPlacement result;result.relative=*point;
    result.percent=legacy_integer(attribute(node,"percent").value_or("100"));
    result.can_push=std::uint8_t(legacy_integer(attribute(node,"can_push").value_or("1")));
    result.id=attribute(node,"id").value_or("");result.source=node;return result;
}
bool appears(int percent,DarwinRandom& random) { return percent>=100 || int(random.bounded(100))<=percent; }
}
std::optional<RoomSprite> take_room_sprite(RoomAttributes& attributes,std::string_view prefix) {
    const auto key=std::string(prefix);
    const auto image=find_attribute(attributes,key),series=find_attribute(attributes,key+"Sprite"),
        first=find_attribute(attributes,key+"First"),count=find_attribute(attributes,key+"Num"),
        rate=find_attribute(attributes,key+"Rate"),animate=find_attribute(attributes,key+"Animate");
    if (image!=attributes.end()) {
        for (auto part:{series,first,count,rate,animate}) if (part!=attributes.end()) throw std::invalid_argument("room sprite combines image and series attributes");
        RoomSprite result;result.image=image->second;attributes.erase(image);return result;
    }
    for (auto part:{series,first,count,rate}) if (part==attributes.end()) return std::nullopt;
    RoomSprite result;result.series=series->second;result.first=legacy_integer(first->second);
    result.count=legacy_integer(count->second);result.rate=legacy_integer(rate->second);
    for (auto part:{series,first,count,rate}) attributes.erase(part);
    if (animate!=attributes.end()) attributes.erase(animate);
    return result;
}
std::optional<Position> take_room_point(RoomAttributes& attributes,std::string_view prefix,
                                       const RoomMarkers& markers,RoomStamp& room) {
    auto find=[&](std::string_view suffix) {
        const auto key=lower(std::string(prefix)+std::string(suffix));
        for (auto it=attributes.begin();it!=attributes.end();++it) if (lower(it->first)==key) return it;
        return attributes.end();
    };
    const auto at=find("at"),x=find("x"),y=find("y");
    if (at!=attributes.end()) {
        if (x!=attributes.end() || y!=attributes.end()) throw std::invalid_argument("room point combines at and x/y");
        if (at->second.size()!=1) throw std::invalid_argument("room at must be one character");
        const auto marker=markers.find(at->second[0]);
        if (marker==markers.end()) throw std::invalid_argument("room at marker not found");
        const auto position=marker->second;
        if (position.x<0 || position.y<0 || position.x>=room.width || position.y>=room.height ||
            room.cells.size()!=std::size_t(room.width)*room.height) throw std::invalid_argument("room at marker outside template");
        room.cells[std::size_t(position.y)*room.width+position.x].kind=1;
        attributes.erase(at);return position;
    }
    if (x==attributes.end() || y==attributes.end()) return std::nullopt;
    const Position position{legacy_integer(x->second),legacy_integer(y->second)};
    attributes.erase(x);attributes.erase(y);return position;
}
RoomElementDefinition read_room_element(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    if (lower(node.tag)!="element") throw std::invalid_argument("expected room element record");
    const auto type=attribute(node,"type");
    if (!type) throw std::invalid_argument("room element type required");
    auto candidate=room;
    RoomElementDefinition result;result.type=*type;result.name=attribute(node,"name").value_or("");
    result.contents=attribute(node,"contents").value_or("");result.placement=placement(node,markers,candidate);
    room=std::move(candidate);return result;
}
RoomElementPlan plan_room_element(const RoomElementDefinition& definition,Position origin,DarwinRandom& random) {
    auto candidate=random;RoomElementPlan result;
    if (!appears(definition.placement.percent,candidate)) {
        random=candidate;return result;
    }
    result.accepted=true;
    result.position={add(origin.x,definition.placement.relative.x),add(origin.y,definition.placement.relative.y)};
    struct Dispatch {const char* type;RoomElementKind kind;int variant;};
    static constexpr Dispatch types[]={
        {"bookshelf",RoomElementKind::bookshelf,-1},{"lever",RoomElementKind::lever,-1},
        {"lutefiskstatue",RoomElementKind::lutefisk_statue,-1},{"portal",RoomElementKind::portal,0},
        {"anvil",RoomElementKind::anvil,-1},{"statue",RoomElementKind::statue,-1},
        {"dredmorstatue",RoomElementKind::dredmor_statue,-1},{"evilchest",RoomElementKind::evil_chest,-1},
        {"freezer",RoomElementKind::freezer,-1},{"bbq",RoomElementKind::bbq,-1},{"fountain",RoomElementKind::fountain,-1},
        {"digglegod",RoomElementKind::diggle_god,-1},{"craftvending",RoomElementKind::dispenser,4},
        {"foodvending",RoomElementKind::dispenser,1},{"boltvending",RoomElementKind::dispenser,0},
        {"drinkvending",RoomElementKind::dispenser,2},{"thrownvending",RoomElementKind::dispenser,3},
        {"dirt",RoomElementKind::dirt,-1},{"wizardportal",RoomElementKind::wizard_portal,-1},
        {"wizardcontrols",RoomElementKind::wizard_controls,-1},{"wizardpedestal",RoomElementKind::wizard_pedestal,-1},
        {"wizardgraffiti",RoomElementKind::wizard_graffiti,-1},{"wizardkey",RoomElementKind::wizard_key,-1}
    };
    for (const auto& type:types) if (definition.type==type.type) {result.kind=type.kind;result.variant=type.variant;break;}
    if (result.kind==RoomElementKind::diggle_god) result.name=definition.name;
    if (result.kind==RoomElementKind::bookshelf) result.contents=definition.contents;
    random=candidate;return result;
}
CustomRoomDefinition read_custom_room_object(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    const auto tag=lower(node.tag);
    if (tag!="customblocker" && tag!="customengraving") throw std::invalid_argument("expected custom blocker/engraving");
    auto candidate=room;auto attributes=node.attributes;const auto sprite=take_room_sprite(attributes);
    if (!sprite) throw std::invalid_argument("custom room object sprite required");
    CustomRoomDefinition result;result.kind=tag=="customblocker"?CustomRoomKind::blocker:CustomRoomKind::engraving;
    result.sprite=*sprite;result.placement=placement(node,markers,candidate);
    result.name=attribute(node,"name").value_or("");result.description=attribute(node,"description").value_or("");
    if (const auto value=attribute(node,"passable")) result.passable=legacy_integer(*value)!=0;
    if (const auto value=attribute(node,"visblock")) result.vis_block=legacy_integer(*value)!=0;
    room=std::move(candidate);return result;
}
std::optional<CustomRoomObject> create_custom_room_object(const CustomRoomDefinition& definition,Position origin,DarwinRandom& random) {
    auto candidate=random;
    if (!appears(definition.placement.percent,candidate)) {random=candidate;return std::nullopt;}
    CustomRoomObject result;result.kind=definition.kind;
    result.position={add(origin.x,definition.placement.relative.x),add(origin.y,definition.placement.relative.y)};
    if (definition.sprite.series.empty()) result.sprite.image=definition.sprite.image;else {result.sprite=definition.sprite;result.sprite.image.clear();}
    result.name=definition.name;result.description=definition.description;result.room_local_id=definition.placement.id;
    result.can_push=definition.placement.can_push;
    result.blocks_movement=!definition.passable.value_or(definition.kind==CustomRoomKind::engraving);
    result.blocks_vision=definition.vis_block.value_or(false);
    random=candidate;return result;
}
}
