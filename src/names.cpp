#include "dredmor/names.hpp"
#include <stdexcept>

namespace dredmor {
namespace {
std::string lower(std::string text) {
    for (auto& c:text) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return text;
}
std::string attr(const XmlNode& node,std::string_view key) {
    for (const auto& field:node.attributes) if (lower(field.first)==key) return field.second;
    return {};
}
}
RoomNameWords read_room_name_words(const XmlNode& root) {
    RoomNameWords result;
    for (const auto& node:root.children) {
        const auto tag=lower(node.tag);
        if (tag=="adjective") result.adjectives.push_back(attr(node,"text"));
        else if (tag=="architecture") {
            result.architecture.push_back(attr(node,"text"));
            // Original loader copies the singular local into both vectors,
            // despite reading a separate plural attribute. Preserve that bug.
            result.architecture_plural.push_back(attr(node,"text"));
        } else if (tag=="noun") result.subjects.push_back(attr(node,"plural"));
    }
    return result;
}
RoomName generate_room_name(const RoomNameWords& words,GameRandom& random) {
    for (const auto* pool:{&words.adjectives,&words.architecture,&words.architecture_plural,&words.subjects})
        if (pool->empty() || pool->size()>0x7ffffffeU) throw std::invalid_argument("room-name word pools must be nonempty and bounded");
    RoomName name;name.display="The ";
    const bool adjective=random.mt.bounded(2)==0;
    auto pick=[&](const auto& pool)->const std::string& {return pool[random.libc.bounded(static_cast<std::uint32_t>(pool.size()))];};
    if (adjective) name.display+=pick(words.adjectives)+" ";
    const bool singular=random.mt.bounded(2)==0;
    name.display+=pick(singular?words.architecture:words.architecture_plural);
    const bool suffix=random.mt.bounded(2)==0;
    if (suffix || !adjective) {name.subject=pick(words.subjects);name.display+=" of "+name.subject;}
    return name;
}
}
