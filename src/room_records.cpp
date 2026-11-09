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
    auto candidate=room;auto attributes=node.attributes;
    const auto position=take_room_point(attributes,"",markers,candidate);
    if (!position) throw std::invalid_argument("room element point required");
    RoomElementDefinition result;result.type=*type;result.name=attribute(node,"name").value_or("");
    result.contents=attribute(node,"contents").value_or("");result.placement.relative=*position;
    result.placement.percent=legacy_integer(attribute(node,"percent").value_or("100"));
    result.placement.can_push=std::uint8_t(legacy_integer(attribute(node,"can_push").value_or("1")));
    result.placement.id=attribute(node,"id").value_or("");result.placement.source=node;
    room=std::move(candidate);return result;
}
RoomElementPlan plan_room_element(const RoomElementDefinition& definition,Position origin,DarwinRandom& random) {
    auto candidate=random;RoomElementPlan result;
    if (definition.placement.percent<100 && int(candidate.bounded(100))>definition.placement.percent) {
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
}
