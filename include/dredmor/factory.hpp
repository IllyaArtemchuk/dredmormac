#pragma once
#include "dredmor/currency.hpp"
#include <array>

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
