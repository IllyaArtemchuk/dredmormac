#include "dredmor/selection.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dredmor {
bool loadout_allowed(const ClassRanks& ranks,int category,bool always) {
    if (category<0 || category>2) throw std::invalid_argument("invalid loadout class category");
    return always || ranks[category]>=*std::max_element(ranks.begin(),ranks.end());
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
