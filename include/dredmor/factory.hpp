#pragma once
#include "dredmor/currency.hpp"
#include <array>

namespace dredmor {
// Ordered registration tables are explicit inputs. This does not infer expansion
// override/level-set membership from the flattened content catalog.
struct ItemFactoryTables {
    using Pool=std::vector<std::uint32_t>;
    std::array<Pool,4> named,potions,wands,bolts,traps;
    Pool reagents;
};
struct ItemFactoryContext {
    std::size_t level_set=0;
    SpawnDepthContext depth;
    int gold_per_level=0;
    float trap_deviation=0;
};
// Named and non-artifact simple loot construction, including currency, misc,
// reagent/component, potion, wand, bolts and trap. Random weapon/armour/artifact
// generation requires further recovered code and throws before state commits.
std::optional<InventoryItem> make_loot_item(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,std::string_view type,
    std::string_view subtype,int quantity,bool artifact,std::uint64_t instance,GameRandom& random,
    const std::vector<SkillRank>& skills={},const std::vector<AbilityDefinition>& abilities={});
}
