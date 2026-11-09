#include "dredmor/selection.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cmath>
#include <cstring>

namespace dredmor {
int select_monster_level(SpawnDepthContext depth,int boost_chance,bool expansion,Random& random) {
    const int cap=expansion?14:9;
    if (depth.alternate && depth.alternate_depth<0) return static_cast<int>(random.bounded(cap));
    const auto chance=static_cast<std::uint32_t>(boost_chance);const int level=depth.alternate?depth.alternate_depth:depth.depth;
    if (chance-1U<0x7ffffffeU && random.bounded(chance)!=0) return level;
    if (level< -1) return 0;
    const std::uint32_t bits=std::uint32_t(level)+1U;std::int32_t boosted;std::memcpy(&boosted,&bits,4);
    return std::min(boosted,cap);
}
namespace {
int gear_target(GameRandom& random,SpawnDepthContext depth,float deviation,int offset,bool weapon) {
    auto truncate=[](float value) {
        if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
        return static_cast<std::int32_t>(value);
    };
    auto add=[](std::int32_t value,int offset) {
        const auto bits=std::uint32_t(value)+std::uint32_t(offset);std::int32_t result;std::memcpy(&result,&bits,4);return result;
    };
    int target=add(truncate(random.normal.sample(float(depth.depth),deviation,random.mt)),offset);
    if (depth.alternate) {
        if (depth.alternate_depth<0) target=int(random.libc.bounded(20));
        else {
            const auto sample=random.normal.sample(float(depth.alternate_depth),deviation,random.mt);
            // GetRandomWeapon consumes this sample but retains its first target.
            if (!weapon) target=add(truncate(sample),offset);
        }
    }
    return std::clamp(target,0,weapon?20:18);
}
const std::vector<std::size_t>& gear_bucket(const GearBuckets& buckets,int index) {
    if (index<0 || std::size_t(index)>=buckets.size()) throw std::runtime_error("gear selection exhausted level buckets");
    const auto& pool=buckets[index];
    if (pool.size()>0x7ffffffeU) throw std::invalid_argument("gear pool too large");
    return pool;
}
}
GearSelection select_weapon(std::vector<GearCandidate>& entries,const GearBuckets& buckets,GameRandom& random,
    SpawnDepthContext depth,float deviation,bool allow_artifact,int offset,std::size_t draw_budget) {
    auto candidate=random;const int target=gear_target(candidate,depth,deviation,offset,true);int level=target;std::size_t draws=0;
    for (;;) {
        unsigned attempts=0;
        for (;;) {
            if (++attempts>100) {attempts=0;--level;}
            const auto& pool=gear_bucket(buckets,level+1);
            if (pool.empty()) {--level;break;}
            if (draws>=draw_budget) throw std::runtime_error("gear selection draw budget exhausted");
            ++draws;const auto index=pool[candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()))];const auto& item=entries.at(index);
            if (!item.craft_output && !item.debug && std::uint32_t(item.weapon_type)-5U>1U && !item.special &&
                ((allow_artifact && !item.artifact_spawned) || !item.artifact)) {
                random=std::move(candidate);if (item.artifact) entries[index].artifact_spawned=true;
                return {index,target,level+1,draws};
            }
        }
    }
}
GearSelection select_armour(const std::vector<GearCandidate>& entries,const GearBuckets& buckets,GameRandom& random,
    SpawnDepthContext depth,float deviation,int offset,std::size_t draw_budget) {
    auto candidate=random;const int target=gear_target(candidate,depth,deviation,offset,false);int bucket=target+1;std::size_t draws=0;
    for (;;) {
        const auto& pool=gear_bucket(buckets,bucket);
        if (pool.empty()) {--bucket;continue;}
        if (draws>=draw_budget) throw std::runtime_error("gear selection draw budget exhausted");
        ++draws;const auto index=pool[candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()))];const auto& item=entries.at(index);
        if (!item.craft_output && !item.special && !item.debug) {random=std::move(candidate);return {index,target,bucket,draws};}
    }
}
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
