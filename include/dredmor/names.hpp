#pragma once
#include "dredmor/random.hpp"
#include "dredmor/xml.hpp"

namespace dredmor {
struct RoomNameWords {
    std::vector<std::string> adjectives,architecture,architecture_plural,subjects;
};
struct RoomName { std::string display,subject; };
RoomNameWords read_room_name_words(const XmlNode& root);
RoomName generate_room_name(const RoomNameWords& words,GameRandom& random);
}
