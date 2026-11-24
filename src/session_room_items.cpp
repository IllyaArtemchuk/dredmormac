#include "dredmor/session.hpp"
#include "dredmor/item_types.hpp"
#include <stdexcept>

namespace dredmor {
std::optional<ScriptItemType> Session::create_room_item_prototype(const ItemFactoryTables& tables,
    const ItemFactoryContext& context,std::string_view type,std::string_view subtype) {
    if (!definitions_) throw std::invalid_argument("script prototype requires item definitions");
    auto random=random_;auto factory=factory_state_;
    static const std::vector<AbilityDefinition> empty;
    const auto prototype=make_loot_item(*definitions_,tables,context,type,subtype,1,false,next_item_instance(),random,skills_,abilities_?*abilities_:empty,&factory);
    std::optional<ScriptItemType> result;
    if (prototype) result=script_item_type(*prototype,definitions_->at(prototype->definition),tables,prototype->definition);
    random_=std::move(random);factory_state_=std::move(factory);return result;
}
std::vector<ScriptTileItem> Session::room_tile_items(const ItemFactoryTables& tables,Position point) const {
    if (point.x<0 || point.y<0 || point.x>=world_.width() || point.y>=world_.height()) throw std::invalid_argument("script tile query outside world");
    std::vector<ScriptTileItem> result;
    auto append=[&](const InventoryItem& item) {
        if (!definitions_) throw std::invalid_argument("script tile item requires definitions");
        result.push_back({true,script_item_type(item,definitions_->at(item.definition),tables,item.definition)});
    };
    for (const auto& ground:ground_) if (ground.position.x==point.x && ground.position.y==point.y) append(ground.item);
    // A placed Trap is an Item. Its optional origin blocker and shop pedestal
    // stock are not items registered in the target square's item list.
    for (const auto& trap:traps_) if (trap.position.x==point.x && trap.position.y==point.y) {
        InventoryItem item;item.kind=ItemKind::trap;item.definition=trap.definition;append(item);
    }
    return result;
}
}
