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
struct ArtifactWords;
// These use the shared original text word pools, with their own draw order.
std::string generate_monster_name(const ArtifactWords& words,GameRandom& random);
std::string generate_platoon_name(const ArtifactWords& words,GameRandom& random);

}
