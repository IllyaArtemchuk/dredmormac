#pragma once
#include "dredmor/items.hpp"
#include "dredmor/random.hpp"

namespace dredmor {
struct ArtifactWords {
    std::vector<std::string> starts,phonemes,titles,nouns,plurals,adjectives,qualities,materials,decor;
};
ArtifactWords read_artifact_words(const XmlNode& root);
std::string generate_artifact_name(const ArtifactWords& words,GameRandom& random);
struct ArtifactRequest {
    ItemKind kind=ItemKind::weapon;
    int quality=1;
    std::optional<std::string> fixed_name;
    std::string base_description;
};
ArtifactRequest artifact_request(const ItemDefinition& definition,int quality);
struct Artifact {
    std::string name,description,buff_description;
    int quality=1;
    ItemModifiers modifiers;
    Damage weapon_damage{};
};
// Names, description and newly allocated stat-buff contents. Item ownership,
// equipment transitions, unique registration and save persistence are separate.
Artifact create_artifact(const ArtifactWords& words,const ArtifactRequest& request,GameRandom& random,
                         std::size_t roll_budget=1000000);
}
