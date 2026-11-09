#include "dredmor/spells.hpp"
#include "dredmor/legacy_number.hpp"
#include <cstdlib>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>

namespace dredmor {
namespace {
std::int32_t scaled_resource(std::int32_t current,std::int32_t amount,float scale,std::int32_t attribute) {
    const auto bits=std::uint32_t(current)+std::uint32_t(amount);std::int32_t value;std::memcpy(&value,&bits,4);
    if (scale>0) {
        const float bonus=scale*static_cast<float>(attribute);const float result=static_cast<float>(value)+bonus;
        value=!std::isfinite(result) || result>=2147483648.0f || result< -2147483648.0f?std::numeric_limits<std::int32_t>::min():static_cast<std::int32_t>(result);
    }
    return value;
}
}
std::int32_t resolve_healing(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute) {
    if (current<1) return current;
    return std::min(scaled_resource(current,amount,scale,attribute),maximum);
}
std::int32_t resolve_spellpoints(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute) {
    return std::min(std::max(0,scaled_resource(current,amount,scale,attribute)),maximum);
}
namespace {
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+=32;return value;}
}
SpellEffectDefinition read_spell_effect(const XmlNode& node) {
    if (lower(node.tag)!="effect") throw std::invalid_argument("expected spell effect");
    SpellEffectDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);const auto& value=attribute.second;
        const auto integer=legacy_integer(value);const auto real=[&]{return static_cast<float>(std::strtod(value.c_str(),nullptr));};
        if (key=="type") result.type=value;
        else if (key=="spell") result.spell=value;
        else if (key=="taxa") result.taxa=value;
        else if (key=="bufftag") result.buff_tag=value;
        else if (key=="amount") result.amount=integer;
        else if (key=="amountf") result.amount_scale=real();
        else if (key=="floorscalef") result.floor_scale=real();
        else if (key=="percent" || key=="percentage") result.percent=integer;
        else if (key=="turns") result.turns=integer;
        else if (key=="primaryscale") result.primary_scale=integer;
        else if (key=="secondaryscale") result.secondary_scale=integer;
        else if (key=="self") result.self=integer;
        else if (key=="affectscaster") result.affects_caster=integer;
        else if (key=="unblockable") result.unblockable=integer;
        else if (key=="permanent") result.permanent=integer;
        else if (key=="kickback") result.kickback=integer;
        else if (key=="resistable") result.resistable=integer!=0;
        else if (key=="affectscorpses") result.affects_corpses=integer!=0;
        else if (key=="issleeping") result.is_sleeping=integer!=0;
        else if (key=="midas") result.midas=integer!=0;
        else if (key=="burn") result.burn=integer!=0;
        else if (key=="regengfx") result.regeneration_graphic=integer!=0;
        else for (std::size_t i=0;i<damage_names.size();++i) {
            // Generic property names are case insensitive; the original
            // ConvertEffectType/ConvertEffectTypeF fallback is case sensitive.
            if (attribute.first==damage_names[i]) result.damage[i]=integer;
            else if (attribute.first==std::string(damage_names[i])+"F") result.scaling[i]=real();
        }
    }
    return result;
}
SpellDefinition read_spell(const XmlNode& node) {
    if (lower(node.tag)!="spell") throw std::invalid_argument("expected spell definition");
    SpellDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);
        if (key=="name") result.name=attribute.second;else if (key=="type") result.type=attribute.second;
    }
    if (result.name.empty()) throw std::invalid_argument("spell name missing");
    for (const auto& child:node.children) if (lower(child.tag)=="effect") result.effects.push_back(read_spell_effect(child));
    return result;
}
}
