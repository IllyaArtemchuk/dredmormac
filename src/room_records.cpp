#include "dredmor/room_records.hpp"
#include "dredmor/legacy_number.hpp"
#include <stdexcept>

namespace dredmor {
namespace {
std::string lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=char(c-'A'+'a');
    return value;
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
}
