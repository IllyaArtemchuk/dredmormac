#pragma once
#include "dredmor/items.hpp"
#include <optional>

namespace dredmor {
struct SkillRank { int id=0,rank=0; };
struct AbilityDefinition {
    std::string name,skill_name,polymorph_tag;
    int skill_id=0,level=0;
    bool starting=false,polymorphed_only=false;
    ItemModifiers modifiers;
    XmlNode source;
    float currency_bonus=0;
};
struct AbilityContext {
    bool weapon_left=false,weapon_right=false,armour_left=false,armour_right=false;
    std::optional<std::string> polymorph;
};
AbilityDefinition read_ability(const XmlNode& node);
bool is_melee_ability(std::string_view skill_name);
int weapon_skill_id(int weapon_type);
ItemModifiers weapon_ability_modifiers(int weapon_type,const std::vector<SkillRank>& skills,
                                      const std::vector<AbilityDefinition>& abilities);
// Base player passive gate. Melee/archery/thrown weapon rebuilding and effect
// triggers are separate; this is not all of an ability's behavior.
bool player_passive_applies(const AbilityDefinition& ability,const std::vector<SkillRank>& skills,
                            const AbilityContext& context={});
struct PassiveStatInput {
    ClassRanks ranks{};
    PrimaryStats primary_flat{},primary_percent=default_primary_percentages();
    SecondaryStats secondary_flat{},secondary_percent=default_percentages();
};
struct PassiveStats { PrimaryStats primary{};SecondaryStats secondary{};Damage damage{},resistance{}; };
PassiveStats derive_passives(const PassiveStatInput& base,const std::vector<SkillRank>& skills,
                             const std::vector<AbilityDefinition>& abilities,const AbilityContext& context={});
}
