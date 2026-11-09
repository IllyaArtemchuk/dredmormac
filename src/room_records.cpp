#include "dredmor/room_records.hpp"
#include "dredmor/legacy_number.hpp"
#include <stdexcept>
#include <cstring>
#include <type_traits>
#include <algorithm>

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
    if (definition.kind!=CustomRoomKind::blocker && definition.kind!=CustomRoomKind::engraving) throw std::invalid_argument("invalid custom room object kind");
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
BreakableRoomDefinition read_breakable_room_object(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    if (lower(node.tag)!="custombreakable") throw std::invalid_argument("expected custom breakable");
    auto candidate=room;auto attributes=node.attributes;const auto intact=take_room_sprite(attributes),breaking=take_room_sprite(attributes,"breaking"),broken=take_room_sprite(attributes,"broken");
    if (!intact || !broken) throw std::invalid_argument("breakable requires intact and broken sprites");
    BreakableRoomDefinition result;result.placement=placement(node,markers,candidate);result.intact=*intact;result.breaking=breaking;result.broken=*broken;
    result.name=attribute(node,"name").value_or("");result.description=attribute(node,"description").value_or("");
    result.hits_needed=legacy_integer(attribute(node,"hitsneededtobreak").value_or("1"));result.hits_taken=legacy_integer(attribute(node,"hitstaken").value_or("0"));
    if (const auto value=attribute(node,"passable")) result.passable=legacy_integer(*value)!=0;
    if (const auto value=attribute(node,"visblock")) result.vis_block=legacy_integer(*value)!=0;
    room=std::move(candidate);return result;
}
std::optional<BreakableRoomObject> create_breakable_room_object(const BreakableRoomDefinition& definition,
    Position origin,DarwinRandom& random,BreakableFrames frames) {
    auto candidate=random;if (!appears(definition.placement.percent,candidate)) {random=candidate;return std::nullopt;}
    if (!definition.breaking) frames.breaking=frames.intact;
    if (!frames.intact || !frames.breaking || !frames.broken) throw std::invalid_argument("breakable sprites require frames");
    auto normalize=[](RoomSprite sprite){if (!sprite.series.empty()) sprite.image.clear();return sprite;};
    BreakableRoomObject result;result.intact=normalize(definition.intact);result.breaking=normalize(definition.breaking.value_or(definition.intact));result.broken=normalize(definition.broken);
    result.hits_needed=definition.hits_needed;result.hits_taken=definition.hits_taken;result.frames=frames;
    result.single_breaking_delay=frames.breaking==1?10:0;
    auto& object=result.object;object.kind=CustomRoomKind::breakable;object.position={add(origin.x,definition.placement.relative.x),add(origin.y,definition.placement.relative.y)};
    object.name=definition.name;object.description=definition.description;object.room_local_id=definition.placement.id;object.can_push=definition.placement.can_push;
    object.blocks_movement=!definition.passable.value_or(false);object.blocks_vision=definition.vis_block.value_or(false);object.playing=object.looping=false;
    const bool broken=result.hits_taken>=result.hits_needed;object.sprite=broken?result.broken:result.intact;object.frame=broken?frames.broken-1:0;
    random=candidate;return result;
}
BreakableHit hit_breakable(BreakableRoomObject& object) {
    BreakableHit result;if (object.hits_taken>=object.hits_needed) return result;
    if (!object.frames.intact || !object.frames.breaking || !object.frames.broken) throw std::invalid_argument("breakable sprites require frames");
    auto next=object;result.handled=true;next.hits_taken=add(next.hits_taken,1);result.triggers.push_back("hit");
    result.broken_now=next.hits_taken>=next.hits_needed;
    if (result.broken_now) {
        result.triggers.push_back("broken");result.animation=next.breaking;next.object.sprite=next.broken;
        next.object.frame=next.frames.broken-1;next.object.blocks_movement=next.object.blocks_vision=false;
        next.object.playing=next.object.looping=false;
    } else next.object.frame=std::uint32_t(std::min(next.hits_taken,int(next.frames.intact)-1));
    object=std::move(next);return result;
}
RoomPedestalRecord read_room_pedestal(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    if (lower(node.tag)!="pedestal") throw std::invalid_argument("expected pedestal");
    auto candidate=room;RoomPedestalRecord result;result.placement=placement(node,markers,candidate);
    result.name=attribute(node,"name").value_or("");result.text=attribute(node,"text").value_or("");room=std::move(candidate);return result;
}
std::optional<CustomRoomObject> create_room_pedestal(const RoomPedestalRecord& definition,Position origin,DarwinRandom& random) {
    auto candidate=random;if (!appears(definition.placement.percent,candidate)) {random=candidate;return std::nullopt;}
    CustomRoomObject result;result.kind=CustomRoomKind::pedestal;
    result.position={add(origin.x,definition.placement.relative.x),add(origin.y,definition.placement.relative.y)};
    result.name=definition.name;result.description=definition.text;result.room_local_id=definition.placement.id;result.can_push=definition.placement.can_push;
    result.sprite.image="dungeon/podium_scroll.png";result.blocks_movement=true;result.blocks_vision=false;result.playing=result.looping=false;
    random=candidate;return result;
}
RoomPopulationRecord read_room_population(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    const auto tag=lower(node.tag);auto candidate=room;RoomPopulationRecord result;
    if (tag=="monster") {RoomMonsterRecord record;record.name=attribute(node,"name").value_or("");result=record;}
    else if (tag=="trap") {RoomTrapRecord record;record.name=attribute(node,"name").value_or("");result=record;}
    else if (tag=="loot") {
        RoomLootRecord record;record.type=attribute(node,"type").value_or("");record.subtype=attribute(node,"subtype").value_or("random");
        record.amount=legacy_integer(attribute(node,"amount").value_or("1"));
        record.artifact=legacy_integer(attribute(node,"artifact").value_or("0"))!=0;result=record;
    } else if (tag=="horde") {
        RoomHordeRecord record;record.name=attribute(node,"name").value_or("");
        record.minimum=legacy_integer(attribute(node,"min").value_or("-1"));
        record.additional=legacy_integer(attribute(node,"additional").value_or("-1"));result=record;
    } else throw std::invalid_argument("expected room population record");
    auto common=placement(node,markers,candidate);
    std::visit([&](auto& record){record.placement=common;},result);room=std::move(candidate);return result;
}
RoomPopulationPlan plan_room_population(const RoomPopulationRecord& definition,Position origin,DarwinRandom& random) {
    auto candidate=random;RoomPopulationPlan result;
    std::visit([&](const auto& record) {
        if (!appears(record.placement.percent,candidate)) return;
        result.position={add(origin.x,record.placement.relative.x),add(origin.y,record.placement.relative.y)};
        using Record=std::decay_t<decltype(record)>;
        if constexpr (std::is_same_v<Record,RoomMonsterRecord>) {
            result.name=record.name;result.request=record.name.empty()?PopulationRequest::random_monster:PopulationRequest::named_monster;
        } else if constexpr (std::is_same_v<Record,RoomTrapRecord>) {
            result.name=record.name;result.request=record.name.empty()?PopulationRequest::random_trap:PopulationRequest::named_trap;
        } else if constexpr (std::is_same_v<Record,RoomLootRecord>) {
            result.request=PopulationRequest::loot;result.type=record.type;result.subtype=record.subtype;
            result.amount=record.amount;result.artifact=record.artifact;
        } else {
            result.request=PopulationRequest::horde;result.name=record.name;
            result.minimum=record.minimum;result.additional=record.additional;
        }
    },definition);
    random=candidate;return result;
}
}
