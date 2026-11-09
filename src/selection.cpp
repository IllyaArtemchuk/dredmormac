#include "dredmor/selection.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cmath>

namespace dredmor {
bool loadout_allowed(const ClassRanks& ranks,int category,bool always) {
    if (category<0 || category>2) throw std::invalid_argument("invalid loadout class category");
    return always || ranks[category]>=*std::max_element(ranks.begin(),ranks.end());
}
TrapSelection select_trap(const std::vector<TrapCandidate>& pool,GameRandom& random,SpawnDepthContext depth,float deviation) {
    if (pool.empty() || pool.size()>0x7ffffffeU) throw std::invalid_argument("invalid trap pool size");
    auto candidate=random;
    auto truncate=[](float value) {
        if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<int>::min();
        return static_cast<int>(value);
    };
    const float mean=float(depth.depth)+1.0f;
    int target=truncate(candidate.normal.sample(mean,deviation,candidate.mt));
    if (depth.alternate) {
        if (depth.alternate_depth<0) target=int(candidate.libc.bounded(20));
        else target=truncate(candidate.normal.sample(float(depth.alternate_depth),deviation,candidate.mt));
    }
    target=std::clamp(target,1,20);
    // Original discards a pool draw before starting its bounded search.
    (void)candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()));
    for (unsigned attempt=1;attempt<=101;++attempt) {
        const auto index=candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()));
        if (attempt==101 || (!pool[index].special && pool[index].level==target)) {
            random=std::move(candidate);return {index,target,attempt};
        }
    }
    throw std::logic_error("unreachable trap fallback");
}
std::size_t select_edible(const std::vector<FoodCandidate>& pool,DarwinRandom& random) {
    if (pool.empty() || pool.size()>std::numeric_limits<std::uint32_t>::max() ||
        std::none_of(pool.begin(),pool.end(),[](auto item){return item.mana<=0 && !item.special && !item.debug;}))
        throw std::invalid_argument("food pool has no eligible edible");
    for (;;) {
        const auto index=random.bounded(static_cast<std::uint32_t>(pool.size()));
        const auto& item=pool[index];
        if (item.mana<=0 && !item.special && !item.debug) return index;
    }
}
std::size_t select_booze(const std::vector<FoodCandidate>& pool,DarwinRandom& random,int target_level) {
    if (pool.empty() || pool.size()>std::numeric_limits<std::uint32_t>::max() ||
        std::none_of(pool.begin(),pool.end(),[](auto item){return item.mana!=0 && !item.special && !item.debug;}))
        throw std::invalid_argument("food pool has no eligible booze");
    target_level=std::clamp(target_level,1,20);
    // Original first samples an eligible booze, then discards that result.
    for (;;) {
        const auto index=random.bounded(static_cast<std::uint32_t>(pool.size()));
        const auto& item=pool[index];
        if (item.mana!=0 && !item.special && !item.debug) break;
    }
    for (unsigned attempt=1;;++attempt) {
        const auto index=random.bounded(static_cast<std::uint32_t>(pool.size()));
        const auto& item=pool[index];
        // This second phase does not exclude special items. At attempt 101 it
        // returns the selected entry even if it is not booze; preserve that.
        if ((item.mana!=0 && !item.debug && item.level==target_level) || attempt>100) return index;
    }
}
}
