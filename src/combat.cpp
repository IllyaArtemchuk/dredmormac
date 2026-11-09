#include "dredmor/combat.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

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
}
