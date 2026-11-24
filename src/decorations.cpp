#include "dredmor/decorations.hpp"
#include <stdexcept>
#include <cstring>

namespace dredmor {
namespace {
std::string lower(std::string s) {for (auto& c:s) if (c>='A' && c<='Z') c+=('a'-'A');return s;}
std::string attribute(const XmlNode& node,std::string_view key) {
    std::string result;
    if (node.attribute_order.empty()) {for (const auto& a:node.attributes) if (lower(a.first)==key) result=a.second;}
    else for (const auto& name:node.attribute_order) if (lower(name)==key) {
        const auto found=node.attributes.find(name);if (found!=node.attributes.end()) result=found->second;
    }
    return result;
}
int offset(int origin,int coordinate) {const auto bits=std::uint32_t(origin)+std::uint32_t(coordinate);std::int32_t result;std::memcpy(&result,&bits,4);return result;}
}
std::vector<RoomDecorationDefinition> read_room_decorations(const XmlNode& text) {
    std::vector<RoomDecorationDefinition> result;
    for (const auto& node:text.children) if (lower(node.tag)=="noun") {
        const auto subject=attribute(node,"plural");
        for (const auto& pair:{std::pair{"painting",RoomDecorationKind::painting},std::pair{"statue",RoomDecorationKind::statue},std::pair{"floor",RoomDecorationKind::floor}}) {
            const auto sprite=attribute(node,pair.first);if (!sprite.empty()) result.push_back({subject,pair.second,sprite});
        }
    }
    return result;
}
std::vector<RoomDecorationPlacement> plan_room_decorations(const RoomStamp& room,
    Position origin,std::string_view subject,const std::vector<RoomDecorationDefinition>& table,std::size_t max_placements) {
    if (room.width<1 || room.height<1 || room.cells.size()!=std::size_t(room.width)*room.height || !max_placements) throw std::invalid_argument("invalid decoration room or budget");
    std::vector<RoomDecorationPlacement> result;if (subject.empty()) return result;
    for (const auto& decoration:table) {
        if (decoration.subject!=subject) continue;
        const auto marker=int(decoration.kind);if (marker<1 || marker>3) continue;
        for (int x=0;x<room.width;++x) for (int y=0;y<room.height;++y) if (room.cells[std::size_t(y)*room.width+x].wall_marker==marker) {
            if (result.size()>=max_placements) throw std::runtime_error("decoration placement budget exceeded");
            result.push_back({decoration.kind,{offset(origin.x,x),offset(origin.y,y)},decoration.sprite});
        }
    }
    return result;
}
CustomRoomObject make_room_decoration(const RoomDecorationPlacement& placement) {
    if (placement.kind!=RoomDecorationKind::painting && placement.kind!=RoomDecorationKind::statue && placement.kind!=RoomDecorationKind::floor) throw std::invalid_argument("invalid room decoration kind");
    CustomRoomObject result;result.kind=placement.kind==RoomDecorationKind::floor?CustomRoomKind::engraving:CustomRoomKind::blocker;
    result.position=placement.position;result.sprite.image=placement.sprite;result.blocks_movement=result.kind==CustomRoomKind::blocker;
    result.blocks_vision=false;result.frame=0;result.playing=false;return result;
}
}
