#include "dredmor/combat.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <limits>

namespace dredmor {
namespace {
std::int32_t bits(std::uint32_t value) { std::int32_t result;std::memcpy(&result,&value,4);return result; }
std::int32_t add(std::int32_t a,std::int32_t b) { return bits(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t subtract(std::int32_t a,std::int32_t b) { return bits(std::uint32_t(a)-std::uint32_t(b)); }
}
CombatResult resolve_combat(const CombatInput& input,const std::function<std::int32_t()>& draw) {
    CombatResult result;
    auto chance=[&](std::int32_t value) {
        const auto random=draw();
        if (random<0) throw std::invalid_argument("combat draw must be a nonnegative libc rand result");
        ++result.draws;
        return random/21474837 < value;
    };
    result.critical=chance(input.critical);
    const bool dodge=chance(input.dodge);
    if (dodge && !result.critical) { result.outcome=CombatOutcome::dodged;return result; }
    const bool counter=chance(input.counter);
    if (counter && !result.critical) { result.outcome=CombatOutcome::countered;return result; }
    const bool block=chance(input.block);
    result.blocked=block && !result.critical;
    auto physical=bits(std::uint32_t(input.melee_power) << (result.critical?1:0));
    for (std::size_t i=0;i<3;++i)
        physical=add(physical,std::max(0,subtract(input.damage[i],input.resistance[i])));
    result.physical=std::max(0,subtract(physical,input.armour));
    for (std::size_t i=3;i<16;++i) {
        const auto reduced=input.damage[i]>0 ? subtract(input.damage[i],input.resistance[i]) : input.damage[i];
        result.elemental=add(result.elemental,std::max(0,reduced));
    }
    if (result.blocked) { result.physical/=4;result.elemental/=2; }
    result.total=add(result.physical,result.elemental);
    if (result.total<1) result.outcome=CombatOutcome::no_effect;
    return result;
}
SpellDamageResult resolve_spell_damage(const SpellDamageInput& input,const std::function<std::int32_t()>& draw) {
    SpellDamageResult result;
    auto chance=[&](std::int32_t value) {
        const auto random=draw();if (random<0) throw std::invalid_argument("spell draw must be a nonnegative libc rand result");
        ++result.draws;return random/21474837<value;
    };
    result.haywire=chance(input.haywire) && input.allow_haywire;
    const bool resist=chance(input.resist);result.resisted=!input.force_unresisted && resist && !result.haywire;
    auto truncate=[](float value) {
        if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
        return static_cast<std::int32_t>(value);
    };
    const float attribute=static_cast<float>(input.scaling_attribute);
    for (std::size_t i=0;i<16;++i) {
        const float scaled=input.scaling[i]*attribute;
        auto value=i<3?truncate(static_cast<float>(input.damage[i])+scaled):add(input.damage[i],truncate(std::floor(scaled)));
        if (value>0) value=subtract(value,input.resistance[i]);
        if (i<3) result.physical=add(result.physical,value);
        else result.elemental=add(result.elemental,std::max(0,value));
    }
    result.physical=std::max(0,subtract(result.physical,input.armour));
    if (result.resisted) {result.physical/=4;result.elemental/=2;}
    result.total=add(result.physical,result.elemental);return result;
}
}
