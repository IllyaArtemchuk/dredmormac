#pragma once
#include "dredmor/items.hpp"

namespace dredmor {
struct SpellEffectDefinition {
    std::string type="damage",spell,taxa,buff_tag;
    Damage damage{};
    std::array<float,16> scaling{};
    std::int32_t amount=0,percent=100,turns=0,primary_scale=-1,secondary_scale=3;
    std::int32_t self=0,affects_caster=0,unblockable=0,permanent=0,kickback=0;
    float amount_scale=0,floor_scale=1;
    bool resistable=true,affects_corpses=true,is_sleeping=false,midas=false,burn=false,regeneration_graphic=false;
    XmlNode source;
};
struct SpellDefinition {
    std::string name,type,layer;
    std::vector<SpellEffectDefinition> effects;
    XmlNode source;
};
// Generic effect fields and damage scaling. Unknown properties and child
// effects remain in source; parsing does not assert execution support.
SpellEffectDefinition read_spell_effect(const XmlNode& node);
SpellDefinition read_spell(const XmlNode& node);
// Original heal effect arithmetic; signed wrapping and float conversion can
// produce negative HP for pathological inputs, without a lower clamp.
std::int32_t resolve_healing(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute);
}
