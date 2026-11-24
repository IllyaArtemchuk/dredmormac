#pragma once
#include "dredmor/currency.hpp"
#include <array>
#include <set>

namespace dredmor {
// Ordered registration tables are explicit inputs. This does not infer expansion
// override/level-set membership from the flattened content catalog.
struct ItemFactoryTables {
    using Pool=std::vector<std::uint32_t>;
    std::array<Pool,4> named,potions,wands,bolts,traps;
    std::array<Pool,4> weapons,armour,food,gems,mushrooms,toolkits;
    Pool reagents;
};
// Original end-of-item registration; converted entries update existing objects
// and do not append. Only the low four level-set mask bits are observed.
void register_factory_item(ItemFactoryTables& tables,std::uint32_t index,const ItemDefinition& definition,
                           std::uint32_t level_sets,bool converted=false);
GearBuckets build_gear_buckets(const std::vector<ItemDefinition>& definitions,const ItemFactoryTables::Pool& named,
                              ItemKind kind);
struct ItemFactoryContext {
    std::size_t level_set=0;
    SpawnDepthContext depth;
    int gold_per_level=0;
    float trap_deviation=0;
    const ArtifactWords* artifact_words=nullptr;
    std::optional<int> level_depth;
};
struct ItemFactoryState {std::set<std::uint32_t> spawned_artifacts;};
enum class TreasureCategory : std::size_t {weapon,armour,ring,ammo,potion,lockpick,mushroom,wand,food};
struct TreasureOptions {
    struct Interval {int minimum=0,maximum=-1;};
    int roll_size=0;
    std::array<Interval,9> ranges{};
    std::size_t max_retries=1000000; // Port guard for malformed/unusable pools.
};
class Tweaks;
enum class Difficulty : std::size_t;
TreasureOptions treasure_options(const Tweaks& tweaks,Difficulty difficulty);
// Basic factory constructor plus Armour's automatic random accessory artifact.
// Caller can suppress it like ArmourC2's nonzero third argument. Other factory
// artifact rolls happen after this constructor and may replace its artifact.
InventoryItem construct_factory_item(const ItemDefinition& definition,std::uint32_t index,
    std::uint64_t instance,const ItemFactoryContext& context,GameRandom& random,bool suppress_armour_randoms=false);
// Level::GenTreasure, distinct from the room-record loot dispatcher. Always
// consumes its initial libc category roll, including forced artifacts. Uses
// current-level depth for selectors/artifact quality, argument-level level_set.
// Failure leaves RNG unchanged. Item IDs and ground placement are caller-owned.
InventoryItem make_level_treasure(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,const TreasureOptions& options,
    bool artifact,std::uint64_t instance,GameRandom& random);
// Full modeled loot dispatch. A supplied state retains original unique weapon
// artifact selection across calls; Session owns and persists that state.
std::optional<InventoryItem> make_loot_item(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,std::string_view type,
    std::string_view subtype,int quantity,bool artifact,std::uint64_t instance,GameRandom& random,
    const std::vector<SkillRank>& skills={},const std::vector<AbilityDefinition>& abilities={},ItemFactoryState* state=nullptr);
}
