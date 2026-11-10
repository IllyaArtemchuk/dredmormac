#include "dredmor/regeneration.hpp"
#include <cstring>

namespace dredmor {
namespace {
std::int32_t wrapped(std::uint32_t bits) {std::int32_t value;std::memcpy(&value,&bits,4);return value;}
std::int32_t interval(std::int32_t turns,std::int32_t bonus) {
    const auto value=wrapped(std::uint32_t(turns)-std::uint32_t(bonus));return value<2?1:value;
}
}
void regenerate(RegenerationState& s,std::int32_t& hp,std::int32_t max_hp,std::int32_t& mp,std::int32_t max_mp,
                std::int32_t hp_bonus,std::int32_t mp_bonus,bool time_allowed) {
    if (time_allowed) {
        if (s.food>0) {if (hp<max_hp) hp=wrapped(std::uint32_t(hp)+1);--s.food;}
        if (s.booze>0) {if (mp<max_mp) mp=wrapped(std::uint32_t(mp)+1);--s.booze;}
        s.health_clock=wrapped(std::uint32_t(s.health_clock)+1);s.mana_clock=wrapped(std::uint32_t(s.mana_clock)+1);
    }
    if (interval(s.health_turns,hp_bonus)<=s.health_clock) {
        if (hp<max_hp && !s.vampire) hp=wrapped(std::uint32_t(hp)+1);
        s.health_clock=0;
    }
    if (interval(s.mana_turns,mp_bonus)<=s.mana_clock) {
        if (mp<max_mp) mp=wrapped(std::uint32_t(mp)+1);
        s.mana_clock=0;
    }
}
}
