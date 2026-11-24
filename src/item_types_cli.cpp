#include "dredmor/item_types.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t kind,index,database;int builtin;
        while (std::cin>>kind>>index>>database>>builtin) {
            if (!kind || kind>12) throw std::invalid_argument("invalid script item kind");
            dredmor::ItemFactoryTables tables;
            for (auto* pools:{&tables.potions,&tables.wands}) for (auto& pool:*pools) {
                std::size_t size;std::cin>>size;if (size>10000) throw std::invalid_argument("script item pool limit exceeded");
                pool.resize(size);for (auto& entry:pool) std::cin>>entry;
            }
            if (!std::cin) throw std::runtime_error("truncated script item fixture");
            dredmor::InventoryItem item;item.kind=dredmor::ItemKind(kind);item.definition=index;
            dredmor::ItemDefinition definition;definition.kind=item.kind;definition.builtin_subtype=builtin;
            const auto type=dredmor::script_item_type(item,definition,tables,database);std::cout<<type.kind<<' '<<type.subtype<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
