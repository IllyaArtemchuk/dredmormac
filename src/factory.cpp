#include "dredmor/factory.hpp"
#include "dredmor/legacy_number.hpp"
#include "dredmor/monsters.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace dredmor {
void register_factory_item(ItemFactoryTables& tables,std::uint32_t index,const ItemDefinition& definition,
    std::uint32_t level_sets,bool converted) {
    if (converted || definition.builtin_subtype>=0) return;
    if (definition.alchemical) tables.reagents.push_back(index);
    for (std::size_t set=0;set<4;++set) if (level_sets&(1U<<set)) {
        tables.named[set].push_back(index);
        switch (definition.kind) {
        case ItemKind::weapon:
            tables.weapons[set].push_back(index);
            if (definition.weapon_type==6 && !definition.special) tables.bolts[set].push_back(index);
            break;
        case ItemKind::armour:tables.armour[set].push_back(index);break;
        case ItemKind::potion:tables.potions[set].push_back(index);break;
        case ItemKind::wand:tables.wands[set].push_back(index);break;
        case ItemKind::food:tables.food[set].push_back(index);break;
        case ItemKind::gem:tables.gems[set].push_back(index);break;
        case ItemKind::mushroom:tables.mushrooms[set].push_back(index);break;
        case ItemKind::toolkit:tables.toolkits[set].push_back(index);break;
        case ItemKind::trap:tables.traps[set].push_back(index);break;
        default:break;
        }
    }
}
GearBuckets build_gear_buckets(const std::vector<ItemDefinition>& definitions,const ItemFactoryTables::Pool& named,ItemKind kind) {
    if (kind!=ItemKind::weapon && kind!=ItemKind::armour) throw std::invalid_argument("invalid gear bucket kind");
    // splitUpItemLists populates levels0..19. Two empty upper buckets make the
    // selector's higher clamp values descend safely instead of reading globals.
    GearBuckets buckets(22);
    for (auto index:named) {
        const auto& item=definitions.at(index);
        if (item.kind==kind && !item.special && item.level>=0 && item.level<20) buckets[item.level].push_back(index);
    }
    return buckets;
}
namespace {
bool item_flag(const ItemDefinition& definition,std::string_view name) {
    for (const auto& attribute:definition.source.attributes) {
        auto key=attribute.first;
        for (auto& c:key) if (c>='A' && c<='Z') c+=('a'-'A');
        if (key==name) return legacy_integer(attribute.second)!=0;
    }
    return false;
}
bool needs_artifact(const ItemDefinition& definition) {
    return (definition.kind==ItemKind::weapon || definition.kind==ItemKind::armour) && item_flag(definition,"artifact");
}
std::uint32_t pick_gear(const std::vector<ItemDefinition>& definitions,const ItemFactoryTables& tables,
    const ItemFactoryContext& context,GameRandom& random,ItemFactoryState& state,ItemKind kind,bool allow_artifact) {
    const auto buckets=build_gear_buckets(definitions,tables.named[context.level_set],kind);
    std::vector<GearCandidate> gear;gear.reserve(definitions.size());
    for (std::size_t i=0;i<definitions.size();++i) {
        const auto& item=definitions[i];gear.push_back({item.weapon_type,item_flag(item,"craftoutput"),item_flag(item,"artifact"),
            state.spawned_artifacts.count(static_cast<std::uint32_t>(i))!=0,item.special,item.debug});
    }
    const auto selected=kind==ItemKind::weapon?select_weapon(gear,buckets,random,context.depth,context.trap_deviation,allow_artifact):
        select_armour(gear,buckets,random,context.depth,context.trap_deviation);
    const auto entry=static_cast<std::uint32_t>(selected.index);
    if (kind==ItemKind::weapon && gear[entry].artifact && gear[entry].artifact_spawned) state.spawned_artifacts.insert(entry);
    return entry;
}
int normal_integer(float value) {
    if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}
int max_stack(const ItemDefinition& definition) {
    for (const auto& attribute:definition.source.attributes) {
        auto key=attribute.first;for (auto& c:key) if (c>='A' && c<='Z') c+=('a'-'A');
        if (key=="maxstack") return legacy_integer(attribute.second);
    }
    return 0;
}
}
TreasureOptions treasure_options(const Tweaks& tweaks,Difficulty difficulty) {
    TreasureOptions options;options.roll_size=tweaks.integer(difficulty,"treasure roll size");
    const char* names[]={"weapon","armour","ring","ammo","potion","lockpick","mushroom","wand","food"};
    for (std::size_t i=0;i<options.ranges.size();++i) options.ranges[i]={tweaks.integer(difficulty,std::string(names[i])+" roll min"),tweaks.integer(difficulty,std::string(names[i])+" roll max")};
    return options;
}
InventoryItem construct_factory_item(const ItemDefinition& definition,std::uint32_t index,
    std::uint64_t instance,const ItemFactoryContext& context,GameRandom& random,bool suppress_armour_randoms) {
    auto candidate=random;auto item=make_inventory_item(definition,index,instance,candidate.libc);
    if (definition.kind==ItemKind::armour && !suppress_armour_randoms) {
        const auto rules=armour_constructor_rules(definition);
        if (rules.randoms && (rules.type==3 || rules.type==4 || rules.type==6)) {
            if (!context.artifact_words) throw std::invalid_argument("automatic armour artifact needs word tables");
            const auto first=std::uint32_t((std::uint64_t(candidate.libc.next())*6U)>>32);
            const auto count=context.depth.alternate?10:context.depth.depth;
            const int quality=int(first+candidate.libc.bounded(std::uint32_t(count))+1);
            item.artifact=create_artifact(*context.artifact_words,artifact_request(definition,quality),candidate);
        }
    }
    random=std::move(candidate);return item;
}
InventoryItem make_level_treasure(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,const TreasureOptions& options,
    bool artifact,std::uint64_t instance,GameRandom& random) {
    if (!instance || context.level_set>=4 || !options.max_retries) throw std::invalid_argument("invalid treasure inputs");
    auto candidate=random;ItemFactoryState state;
    const int roll=int(candidate.libc.bounded(std::uint32_t(options.roll_size)));
    auto within=[&](TreasureCategory category) {const auto r=options.ranges[std::size_t(category)];return roll>=r.minimum && roll<=r.maximum;};
    auto uniform=[&](const ItemFactoryTables::Pool& pool) {
        if (pool.empty() || pool.size()>0x7ffffffeU) throw std::invalid_argument("invalid treasure pool");
        return pool.at(candidate.libc.bounded(std::uint32_t(pool.size())));
    };
    auto gear=[&](ItemKind kind) {return pick_gear(definitions,tables,context,candidate,state,kind,false);};
    auto armour=[&](bool ring) {
        for (std::size_t attempt=0;attempt<options.max_retries;++attempt) {
            const auto index=gear(ItemKind::armour);const auto type=armour_constructor_rules(definitions.at(index)).type;
            const bool accessory=type==3 || type==4 || type==6;
            if (accessory==ring) return index;
        }
        throw std::runtime_error("treasure armour retry budget exceeded");
    };
    std::uint32_t index=0;bool ammunition=false,lockpicks=false;
    if (artifact) index=gear(candidate.mt.bounded(2)==0?ItemKind::weapon:ItemKind::armour);
    else if (within(TreasureCategory::weapon)) index=gear(ItemKind::weapon);
    else if (within(TreasureCategory::armour)) index=armour(false);
    else if (within(TreasureCategory::ring)) index=armour(true);
    else if (within(TreasureCategory::ammo)) {
        ammunition=true;
        if (candidate.mt.bounded(2)==0) {
            const auto buckets=build_gear_buckets(definitions,tables.named[context.level_set],ItemKind::weapon);
            int target=std::clamp(normal_integer(candidate.normal.sample(float(context.depth.depth),context.trap_deviation,candidate.mt)),0,20);
            unsigned attempts=0;bool found=false;
            for (std::size_t draws=0;draws<options.max_retries;++draws) {
                if (++attempts>100) {attempts=0;--target;}
                target=std::max(target,0);const auto& pool=buckets.at(std::size_t(target+1));
                if (pool.empty()) {--target;attempts=0;continue;}
                index=std::uint32_t(pool.at(candidate.libc.bounded(std::uint32_t(pool.size()))));const auto& item=definitions.at(index);
                if (item.weapon_type==5 && !item.debug && !item.special && !item_flag(item,"craftoutput")) {found=true;break;}
            }
            if (!found) throw std::runtime_error("treasure thrown-weapon retry budget exceeded");
        } else index=uniform(tables.bolts[context.level_set]);
    } else if (within(TreasureCategory::potion)) index=uniform(tables.potions[context.level_set]);
    else if (within(TreasureCategory::lockpick)) {
        lockpicks=true;auto found=std::find_if(definitions.begin(),definitions.end(),[](const auto& item) {return item.kind==ItemKind::misc && item.builtin_subtype==0;});
        if (found==definitions.end()) throw std::invalid_argument("missing lockpick definition");index=std::uint32_t(found-definitions.begin());
    } else if (within(TreasureCategory::mushroom)) index=uniform(tables.mushrooms[context.level_set]);
    else if (within(TreasureCategory::wand)) index=uniform(tables.wands[context.level_set]);
    else if (within(TreasureCategory::food)) {
        const auto& pool=tables.food[context.level_set];std::vector<FoodCandidate> food;
        for (auto entry:pool) {const auto& item=definitions.at(entry);food.push_back({item.food_mp,item.level,item.special,item.debug});}
        if (candidate.mt.bounded(2)==0) index=pool.at(select_edible(food,candidate.libc));
        else {
            const auto target=std::clamp(normal_integer(candidate.normal.sample(float(context.depth.depth)+1.0f,context.trap_deviation,candidate.mt)),1,20);
            index=pool.at(select_booze(food,candidate.libc,target));
        }
    } else index=gear(ItemKind::weapon);
    const auto& definition=definitions.at(index);auto item=construct_factory_item(definition,index,instance,context,candidate);
    if (artifact) {
        if (!context.artifact_words) throw std::invalid_argument("treasure artifact needs word tables");
        const auto quality=roll_depth_artifact_quality(context.depth.depth,candidate.libc);
        item.artifact=create_artifact(*context.artifact_words,artifact_request(definition,quality),candidate);
    } else if (ammunition || lockpicks) {
        const auto first=(std::uint64_t(candidate.libc.next())*0x4fffffffU)>>58;
        const auto second=(std::uint64_t(candidate.libc.next())*0x4fffffffU)>>59;
        const auto third=std::uint32_t(candidate.libc.next())>>30;
        item.quantity=int(first+second+third+1);item.stackable=true;
        const auto maximum=max_stack(definition);if (ammunition && maximum>0) item.quantity=std::min(item.quantity,maximum);
    }
    random=std::move(candidate);return item;
}
std::optional<InventoryItem> make_loot_item(const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,std::string_view type,
    std::string_view subtype,int quantity,bool artifact,std::uint64_t instance,GameRandom& random,
    const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities,ItemFactoryState* state) {
    if (!instance || context.level_set>=4) throw std::invalid_argument("invalid loot identity or level set");
    auto candidate=random;auto next_state=state?*state:ItemFactoryState{};
    enum class ArtifactRoll {none,named_depth,level_depth,explicit_three};ArtifactRoll artifact_roll=ArtifactRoll::none;
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
        for (const auto& pool:tables.named) for (auto entry:pool) if (definitions.at(entry).name==subtype) {
            index=entry;if (needs_artifact(definitions[entry])) artifact_roll=ArtifactRoll::named_depth;return;
        }
    };
    auto uniform=[&](const ItemFactoryTables::Pool& pool) {
        // The original dereferences an invalid table on an empty pool. Fail
        // atomically instead, without claiming original malformed-data behavior.
        if (pool.empty() || pool.size()>0x7ffffffeU) throw std::invalid_argument("invalid loot pool");
        index=pool[candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()))];
    };
    auto gear_pick=[&](ItemKind kind,bool allow_artifact) {
        return pick_gear(definitions,tables,context,candidate,next_state,kind,allow_artifact);
    };
    if (type=="artifact") {
        const auto kind=candidate.mt.bounded(2)==0?ItemKind::weapon:ItemKind::armour;
        index=gear_pick(kind,false);
        if (kind==ItemKind::weapon) {
            // Original constructs and destroys the first weapon, then selects
            // again. GetRandomWeapon excludes the pathological type5 loop.
            (void)make_inventory_item(definitions.at(*index),*index,instance,candidate.libc);
            index=gear_pick(kind,false);
        }
        artifact_roll=ArtifactRoll::level_depth;
    } else if (type=="zorkmids") {index=builtin(2);money=roll_currency(context.depth,candidate.libc);}
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
        index=gear_pick(type=="weapon"?ItemKind::weapon:ItemKind::armour,artifact);
        if (artifact) artifact_roll=ArtifactRoll::explicit_three;
    }
    if (!index) return std::nullopt;
    const auto& definition=definitions.at(*index);
    // CreateItem(name) returns null on unsupported types, even if a later
    // duplicate name would have a supported constructor.
    if (!money && (definition.kind==ItemKind::unknown || definition.kind==ItemKind::currency ||
        (definition.kind==ItemKind::misc && type!="misc"))) {random=std::move(candidate);return std::nullopt;}
    auto result=money?make_currency_item(definition,*index,instance,*money,skills,abilities):
        construct_factory_item(definition,*index,instance,context,candidate);
    if (artifact_roll!=ArtifactRoll::none) {
        if (!context.artifact_words) throw std::invalid_argument("artifact creation needs word tables");
        const auto quality=artifact_roll==ArtifactRoll::explicit_three?roll_explicit_artifact_quality(candidate.libc):
            roll_depth_artifact_quality(artifact_roll==ArtifactRoll::level_depth?context.level_depth.value_or(context.depth.depth):context.depth.depth,candidate.libc);
        result.artifact=create_artifact(*context.artifact_words,artifact_request(definition,quality),candidate);
    }
    if (quantity>1) {result.quantity=quantity;result.stackable=true;}
    random=std::move(candidate);if (state) *state=std::move(next_state);return result;
}
}
