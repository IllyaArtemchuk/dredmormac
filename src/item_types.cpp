#include "dredmor/item_types.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dredmor {
ScriptItemType script_item_type(const InventoryItem& item,const ItemDefinition& definition,
    const ItemFactoryTables& tables,std::uint32_t database_identity) {
    if (item.kind!=definition.kind || item.kind==ItemKind::unknown || std::uint32_t(item.kind)>12) throw std::invalid_argument("script item type requires matching definition");
    ScriptItemType type{std::uint32_t(item.kind),database_identity};
    if (item.kind==ItemKind::currency) type.subtype=0; // Zorkmids retains Item's base subtype.
    else if (item.kind==ItemKind::misc) {
        if (definition.builtin_subtype<0 || definition.builtin_subtype>1) throw std::invalid_argument("unsupported script miscellaneous item");
        type.subtype=std::uint32_t(definition.builtin_subtype);
    } else if (item.kind==ItemKind::potion || item.kind==ItemKind::wand) {
        type.subtype=0;const auto& pools=item.kind==ItemKind::potion?tables.potions:tables.wands;
        for (const auto& pool:pools) {
            const auto found=std::find(pool.begin(),pool.end(),item.definition);
            if (found!=pool.end()) {
                const auto ordinal=std::size_t(found-pool.begin());
                if (ordinal>std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("script item registration ordinal overflow");
                type.subtype=std::uint32_t(ordinal);break;
            }
        }
    }
    return type;
}
}
