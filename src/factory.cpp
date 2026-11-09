#include "dredmor/factory.hpp"
#include "dredmor/legacy_number.hpp"
#include <stdexcept>

namespace dredmor {
namespace {
bool needs_artifact(const ItemDefinition& definition) {
    if (definition.kind!=ItemKind::weapon && definition.kind!=ItemKind::armour) return false;
    for (const auto& attribute:definition.source.attributes) {
        auto key=attribute.first;
        for (auto& c:key) if (c>='A' && c<='Z') c+=('a'-'A');
        if (key=="artifact") return legacy_integer(attribute.second)!=0;
    }
    return false;
}
}
std::optional<InventoryItem> make_loot_item(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,std::string_view type,
    std::string_view subtype,int quantity,bool artifact,std::uint64_t instance,GameRandom& random,
    const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities) {
    if (!instance || context.level_set>=4) throw std::invalid_argument("invalid loot identity or level set");
    auto candidate=random;
    auto builtin=[&](int subtype) {
        for (std::size_t i=0;i<definitions.size();++i)
            if (definitions[i].builtin_subtype==subtype) return static_cast<std::uint32_t>(i);
        throw std::invalid_argument("missing builtin loot definition");
    };
    std::optional<std::uint32_t> index;
    std::optional<std::int32_t> money;
    auto named=[&]() {
        if (subtype=="zorkmids") {
            index=builtin(2);money=roll_named_currency(context.depth.depth,context.gold_per_level,candidate.libc);return;
        }
        for (const auto& pool:tables.named) for (auto entry:pool) if (definitions.at(entry).name==subtype) {index=entry;return;}
    };
    auto uniform=[&](const ItemFactoryTables::Pool& pool) {
        // The original dereferences an invalid table on an empty pool. Fail
        // atomically instead, without claiming original malformed-data behavior.
        if (pool.empty() || pool.size()>0x7ffffffeU) throw std::invalid_argument("invalid loot pool");
        index=pool[candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()))];
    };
    if (type=="artifact") throw std::runtime_error("artifact loot generation is not implemented");
    if (type=="zorkmids") {index=builtin(2);money=roll_currency(context.depth,candidate.libc);}
    else if (type=="misc") {
        if (subtype=="lockpick" || subtype=="lockpicks") index=builtin(0);
        else if (subtype=="spores") index=builtin(1);
    } else if (subtype!="random") named();
    else if (type=="reagent" || type=="component") uniform(tables.reagents);
    else if (type=="potion") uniform(tables.potions[context.level_set]);
    else if (type=="wand") uniform(tables.wands[context.level_set]);
    else if (type=="bolts") uniform(tables.bolts[context.level_set]);
    else if (type=="trap") {
        const auto& pool=tables.traps[context.level_set];std::vector<TrapCandidate> traps;
        for (auto entry:pool) {const auto& item=definitions.at(entry);traps.push_back({item.level,item.special,item.debug});}
        index=pool.at(select_trap(traps,candidate,context.depth,context.trap_deviation).index);
    } else if (type=="weapon" || type=="armour") {
        (void)artifact;
        throw std::runtime_error("random weapon/armour loot generation is not implemented");
    }
    if (!index) return std::nullopt;
    const auto& definition=definitions.at(*index);
    if (needs_artifact(definition)) throw std::runtime_error("named artifact generation is not implemented");
    // CreateItem(name) returns null on unsupported types, even if a later
    // duplicate name would have a supported constructor.
    if (!money && (definition.kind==ItemKind::unknown || definition.kind==ItemKind::currency ||
        (definition.kind==ItemKind::misc && type!="misc"))) {random=std::move(candidate);return std::nullopt;}
    auto result=money?make_currency_item(definition,*index,instance,*money,skills,abilities):
        make_inventory_item(definition,*index,instance,candidate.libc);
    if (quantity>1) {result.quantity=quantity;result.stackable=true;}
    random=std::move(candidate);return result;
}
}
