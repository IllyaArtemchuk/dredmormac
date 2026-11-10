#pragma once
#include <cstdint>

namespace dredmor {
struct RegenerationState {
    std::int32_t health_turns=0,mana_turns=0,health_clock=0,mana_clock=0,food=0,booze=0;
    bool vampire=false;
};
// Hittable food/booze counters followed by Player natural regeneration. These
// routines do not choose when the full level object queue updates a player.
void regenerate(RegenerationState& state,std::int32_t& health,std::int32_t maximum_health,
                std::int32_t& mana,std::int32_t maximum_mana,std::int32_t health_bonus,
                std::int32_t mana_bonus,bool time_allowed=true);
}
