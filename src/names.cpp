#include "dredmor/names.hpp"
#include "dredmor/artifact.hpp"
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
namespace {
std::uint32_t name_index(const std::vector<std::string>& pool,GameRandom& random) {
    if (pool.size()>0x7ffffffeU) throw std::invalid_argument("name word pool is too large");
    return random.libc.bounded(std::uint32_t(pool.size()));
}
const std::string& name_word(const std::vector<std::string>& pool,std::size_t index) {
    if (index>=pool.size()) throw std::invalid_argument("name word pool has no selected entry");
    return pool[index];
}
std::string pick_name_word(const std::vector<std::string>& pool,GameRandom& random) {return name_word(pool,name_index(pool,random));}
}
std::string generate_monster_name(const ArtifactWords& words,GameRandom& random) {
    auto candidate=random;
    auto name=pick_name_word(words.starts,candidate);name+=pick_name_word(words.phonemes,candidate);name+=pick_name_word(words.phonemes,candidate);
    name+=" the ";name+=pick_name_word(words.titles,candidate);name+=' ';name+=pick_name_word(words.plurals,candidate);
    random=std::move(candidate);return name;
}
std::string generate_platoon_name(const ArtifactWords& words,GameRandom& random) {
    auto candidate=random;
    const auto number=int((std::uint64_t(std::uint32_t(candidate.libc.next()))*0x0c7ffffbULL)>>52)+1;
    std::string name="Lord Dredmor's "+std::to_string(number);
    if (number>=10 && number<=20) name+="th";
    else switch (number%10) {case 1:name+="st";break;case 2:name+="nd";break;case 3:name+="rd";break;default:name+="th";}
    const auto style=(std::uint64_t(std::uint32_t(candidate.libc.next()))*0xbffffffbULL)>>60;
    static constexpr const char* types[]={" platoon"," company"," regiment"," cohort"," flank"," brigade"};
    if (style>=6) throw std::runtime_error("platoon unit roll outside original libc range");
    name+=types[style];name+=", the ";
    const bool titled=candidate.mt.bounded(2)==0;
    // Both branches draw from the title count first and the noun count last.
    // The alternate branch uses those indices in adjective/plural pools.
    const auto first=name_index(words.titles,candidate);
    name+=name_word(titled?words.titles:words.adjectives,first);name+=' ';
    const auto second=name_index(words.nouns,candidate);name+=name_word(titled?words.nouns:words.plurals,second);
    random=std::move(candidate);return name;
}

}
