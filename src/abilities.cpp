#include "dredmor/abilities.hpp"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::string lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return value;
}
std::string attr(const XmlNode& node,std::string_view key) {
    for (const auto& pair:node.attributes) if (lower(pair.first)==key) return pair.second;
    return {};
}
int integer(const XmlNode& node,std::string_view key,int fallback=0) {
    const auto text=attr(node,key);if (text.empty()) return fallback;
    int value;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if (parsed.ec!=std::errc() || parsed.ptr!=text.data()+text.size()) throw std::invalid_argument("invalid ability integer");
    return value;
}
std::int32_t add(std::int32_t a,std::int32_t b) {
    const std::uint32_t bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;
    std::memcpy(&result,&bits,4);return result;
}
}
AbilityDefinition read_ability(const XmlNode& node) {
    if (lower(node.tag)!="ability") throw std::invalid_argument("expected ability definition");
    AbilityDefinition ability;ability.source=node;ability.name=attr(node,"name");
    ability.skill_id=integer(node,"skill");ability.level=integer(node,"level");ability.skill_name=attr(node,"skillname");
    ability.starting=integer(node,"startskill")!=0;ability.polymorphed_only=integer(node,"polymorphedonly")!=0;
    ability.polymorph_tag=attr(node,"polymorphtag");
    if (ability.name.empty()) throw std::invalid_argument("ability name missing");
    for (const auto& child:node.children) {
        const auto tag=lower(child.tag);
        if (tag=="primarybuff" || tag=="secondarybuff") {
            const auto id=integer(child,"id"),amount=integer(child,"amount");
            if (id<0 || id>=(tag=="primarybuff"?6:24)) throw std::invalid_argument("ability stat ID out of bounds");
            if (tag=="primarybuff") ability.modifiers.primary[id]=amount;
            else ability.modifiers.secondary[id]=amount;
        } else if (tag=="damagebuff" || tag=="resistbuff") {
            auto& target=tag=="damagebuff"?ability.modifiers.damage:ability.modifiers.resistance;
            for (std::size_t i=0;i<damage_names.size();++i) if (!attr(child,damage_names[i]).empty())
                target[i]=integer(child,damage_names[i]);
        }
    }
    return ability;
}
bool is_melee_ability(std::string_view name) {
    for (auto melee:{"Swords","Daggers","Axes","Maces","Polearms","Staves","Unarmed Combat","Shield Bearer"})
        if (name==melee) return true;
    return false;
}
int weapon_skill_id(int weapon_type) {
    static constexpr int identifiers[]={0,1,2,3,4,5,4,45,46};
    return weapon_type>=0 && weapon_type<9?identifiers[weapon_type]:2;
}
ItemModifiers weapon_ability_modifiers(int weapon_type,const std::vector<SkillRank>& skills,
                                      const std::vector<AbilityDefinition>& abilities) {
    const auto identifier=weapon_skill_id(weapon_type);int rank=0;
    for (auto skill:skills) if (skill.id==identifier) {rank=skill.rank;break;}
    ItemModifiers result;
    for (const auto& ability:abilities) if (ability.skill_id==identifier && ability.level<rank) {
        for (std::size_t i=0;i<6;++i) result.primary[i]=add(result.primary[i],ability.modifiers.primary[i]);
        for (std::size_t i=0;i<24;++i) result.secondary[i]=add(result.secondary[i],ability.modifiers.secondary[i]);
        for (std::size_t i=0;i<16;++i) {
            result.damage[i]=add(result.damage[i],ability.modifiers.damage[i]);
            result.resistance[i]=add(result.resistance[i],ability.modifiers.resistance[i]);
        }
    }
    return result;
}
bool player_passive_applies(const AbilityDefinition& ability,const std::vector<SkillRank>& skills,const AbilityContext& context) {
    if (ability.polymorphed_only) {
        if (!context.polymorph || *context.polymorph!=ability.polymorph_tag) return false;
    } else if (context.polymorph) return false;
    const auto& name=ability.skill_name;
    const bool allowed=(name=="Unarmed Combat" && !context.weapon_left && !context.weapon_right) ||
        (!is_melee_ability(name) && name!="Thrown Weaponry" && name!="Archery") ||
        (name=="Shield Bearer" && (context.armour_left || context.armour_right));
    if (!allowed) return false;
    int rank=0;
    for (auto skill:skills) if (skill.id==ability.skill_id) {rank=skill.rank;break;}
    return ability.level<rank;
}
PassiveStats derive_passives(const PassiveStatInput& base,const std::vector<SkillRank>& skills,
                             const std::vector<AbilityDefinition>& abilities,const AbilityContext& context) {
    PassiveStats result;result.primary=derive_primary(base.ranks,base.primary_flat,base.primary_percent);
    for (const auto& ability:abilities) if (player_passive_applies(ability,skills,context))
        for (std::size_t i=0;i<6;++i) result.primary[i]=add(result.primary[i],ability.modifiers.primary[i]);
    result.secondary=derive_secondary(result.primary,base.secondary_flat,base.secondary_percent);
    // These additions occur after the base clamp/percentage calculation. A
    // negative ability bonus may therefore leave a negative final secondary.
    for (const auto& ability:abilities) if (player_passive_applies(ability,skills,context)) {
        for (std::size_t i=0;i<24;++i) result.secondary[i]=add(result.secondary[i],ability.modifiers.secondary[i]);
        for (std::size_t i=0;i<16;++i) {
            result.damage[i]=add(result.damage[i],ability.modifiers.damage[i]);
            result.resistance[i]=add(result.resistance[i],ability.modifiers.resistance[i]);
        }
    }
    return result;
}
}
