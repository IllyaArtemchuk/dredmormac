#include "dredmor/artifact.hpp"
#include <stdexcept>
#include "dredmor/legacy_number.hpp"

namespace dredmor {
namespace {
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+='a'-'A';return value;}
std::string attribute(const XmlNode& node,std::string_view name) {
    for (const auto& field:node.attributes) if (lower(field.first)==name) return field.second;
    return {};
}
const std::string& pick(const std::vector<std::string>& words,GameRandom& random) {
    if (words.empty() || words.size()>0x7ffffffeU) throw std::invalid_argument("invalid artifact word pool");
    return words[random.libc.bounded(static_cast<std::uint32_t>(words.size()))];
}
}
ArtifactWords read_artifact_words(const XmlNode& root) {
    ArtifactWords words;
    for (const auto& node:root.children) {
        const auto tag=lower(node.tag);auto text=attribute(node,"text");
        if (tag=="startphoneme") words.starts.push_back(text);
        else if (tag=="phoneme") words.phonemes.push_back(text);
        else if (tag=="title") words.titles.push_back(text);
        else if (tag=="noun") {words.nouns.push_back(text);words.plurals.push_back(attribute(node,"plural"));}
        else if (tag=="adjective") words.adjectives.push_back(text);
        else if (tag=="quality") words.qualities.push_back(text);
        else if (tag=="material") words.materials.push_back(text);
        else if (tag=="decor") words.decor.push_back(text);
    }
    return words;
}
ArtifactRequest artifact_request(const ItemDefinition& definition,int quality) {
    ArtifactRequest request;request.kind=definition.kind;request.quality=quality;request.base_description=definition.description;
    const auto flag=attribute(definition.source,"artifact");
    if (!flag.empty() && legacy_integer(flag)!=0) {
        request.fixed_name=definition.name;request.quality=0;
        for (const auto& node:definition.source.children) if (lower(node.tag)=="artifact") {
            const auto value=attribute(node,"quality");if (!value.empty()) request.quality=legacy_integer(value);
        }
    }
    return request;
}
std::string generate_artifact_name(const ArtifactWords& words,GameRandom& random) {
    if (words.nouns.size()!=words.plurals.size()) throw std::invalid_argument("artifact noun/plural pools differ");
    auto candidate=random;
    auto name=pick(words.starts,candidate);name+=pick(words.phonemes,candidate);
    if (candidate.mt.bounded(2)==0) name+=pick(words.phonemes,candidate);
    name+=", the ";
    if (candidate.mt.bounded(2)==0) {
        name+=pick(words.titles,candidate);name+=' ';name+=pick(words.plurals,candidate);
    } else {
        name+=pick(words.adjectives,candidate);name+=' ';name+=pick(words.nouns,candidate);
    }
    random=std::move(candidate);return name;
}
Artifact create_artifact(const ArtifactWords& words,const ArtifactRequest& request,GameRandom& random,std::size_t roll_budget) {
    if (request.kind!=ItemKind::weapon && request.kind!=ItemKind::armour) throw std::invalid_argument("invalid artifact item kind");
    auto candidate=random;Artifact result;result.name=generate_artifact_name(words,candidate);
    if (request.fixed_name) result.name=*request.fixed_name;
    result.quality=request.quality;result.description="It is studded with ";
    if (request.quality>2) {result.description+=pick(words.qualities,candidate);result.description+=' ';}
    result.description+=pick(words.materials,candidate);
    if (request.quality>4) {
        result.description+=" and is decorated with ";result.description+=pick(words.qualities,candidate);
        result.description+=' ';result.description+=pick(words.decor,candidate);
    }
    const bool extra=candidate.mt.bounded(2)==0;
    if (extra && request.quality>6) {
        result.description+=" and ";result.description+=pick(words.qualities,candidate);
        result.description+=' ';result.description+=pick(words.decor,candidate);
    }
    result.description+=".\n\n";result.buff_description=request.base_description+" "+result.description;
    int remaining=request.quality;std::size_t rolls=0;
    while (remaining>0) {
        if (rolls++>=roll_budget) throw std::runtime_error("artifact roll budget exhausted");
        const auto category=candidate.libc.bounded(82);
        if (category>=76) {++result.modifiers.primary[category-76];continue;}
        std::int32_t* target=nullptr;std::int32_t* weapon=nullptr;
        if (category<16) {
            target=&result.modifiers.damage[category];
            if (request.kind==ItemKind::weapon) weapon=&result.weapon_damage[category];
        } else if (category<32) target=&result.modifiers.resistance[category-16];
        else if (category<70) {
            const auto index=category<51?category-32:category-51;
            if (index==15) continue;
            target=&result.modifiers.secondary[index];
        } else target=&result.modifiers.primary[category-70];
        for (int trial=0;trial<3 && remaining>0;++trial) if (candidate.mt.bounded(2)==0) {
            ++*target;if (weapon) ++*weapon;--remaining;
        }
    }
    random=std::move(candidate);return result;
}
}
