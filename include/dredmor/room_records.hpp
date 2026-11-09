#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/xml.hpp"

namespace dredmor {
using RoomAttributes=std::map<std::string,std::string,std::less<>>;
using RoomMarkers=std::map<char,Position>;
// PointProperty::SetFrom behavior for valid ASCII metadata. Consumes matching
// attributes; at resolves a marker and immediately writes ordinary floor.
// Explicit x/y do not change the template. Missing coordinates return nullopt.
std::optional<Position> take_room_point(RoomAttributes& attributes,std::string_view prefix,
                                       const RoomMarkers& markers,RoomStamp& room);
}
