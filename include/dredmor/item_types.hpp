#pragma once
#include "dredmor/factory.hpp"
#include "dredmor/room_scripts.hpp"

namespace dredmor {
// Native Item +0x74/+0x78 comparison fields. The caller supplies the database
// entry ID; potion/wand constructors instead use the first matching ordinal
// across the four ordered registration pools, defaulting to base subtype0.
ScriptItemType script_item_type(const InventoryItem& item,const ItemDefinition& definition,
    const ItemFactoryTables& tables,std::uint32_t database_identity);
}
