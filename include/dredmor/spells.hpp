#pragma once
#include "dredmor/items.hpp"
#include "dredmor/world.hpp"
#include "dredmor/random.hpp"
#include "dredmor/buffs.hpp"
#include <memory>

namespace dredmor {
struct ManeuverTemplate {
    int id=0,width=0,height=0;
    bool affects_player=true;
    std::vector<std::uint8_t> cells;
    XmlNode source;
};
struct ManeuverPlan {bool accepted=false;std::vector<Position> targets;};
ManeuverTemplate read_maneuver_template(const XmlNode& node);
ManeuverPlan plan_maneuver(const std::vector<ManeuverTemplate>& templates,int id,int anchored,int facing,
    Position origin,Position target,DarwinRandom& random);
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
    std::vector<BuffDefinition> buffs;
    XmlNode source;
    std::shared_ptr<const std::vector<ManeuverTemplate>> maneuvers;
};
// Generic effect fields and damage scaling. Unknown properties and child
// effects remain in source; parsing does not assert execution support.
SpellEffectDefinition read_spell_effect(const XmlNode& node);
SpellDefinition read_spell(const XmlNode& node);
// Original heal effect arithmetic; signed wrapping and float conversion can
// produce negative HP for pathological inputs, without a lower clamp.
std::int32_t resolve_healing(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute);
std::int32_t resolve_spellpoints(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute);
}
