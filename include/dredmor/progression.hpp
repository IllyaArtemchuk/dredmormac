#pragma once
#include <array>
#include <cstdint>

namespace dredmor {
std::array<std::int32_t,50> experience_table(std::int32_t n);
struct Progression {
    std::int32_t level=0,experience=0,lifetime_experience=0,score_experience=0,unspent_points=0;
    // Returns true on level-up. Original threshold is strict >; excess XP is
    // discarded when the original resets the current level's experience.
    bool gain(std::int32_t amount,std::int32_t n,bool doubled=false);
};
}
