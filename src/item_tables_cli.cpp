#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>

void print(const dredmor::ItemFactoryTables& tables) {
    for (const auto* sets:{&tables.named,&tables.potions,&tables.wands,&tables.bolts,&tables.traps,&tables.weapons,
        &tables.armour,&tables.food,&tables.gems,&tables.mushrooms,&tables.toolkits})
        for (const auto& pool:*sets) {std::cout<<pool.size();for (auto entry:pool) std::cout<<' '<<entry;std::cout<<' ';}
    std::cout<<tables.reagents.size();for (auto entry:tables.reagents) std::cout<<' '<<entry;std::cout<<'\n';
}
int main(int argc,char** argv) {
    try {
        if (argc==2) {print(dredmor::GameContent::load(argv[1]).item_factory_tables());return 0;}
        std::size_t count;
        while (std::cin>>count) {
            if (count>10000) throw std::invalid_argument("too many registration fixtures");
            dredmor::ItemFactoryTables tables;
            for (std::size_t i=0;i<count;++i) {
                dredmor::ItemDefinition item;unsigned kind;std::uint32_t mask;bool converted;
                std::cin>>kind>>item.weapon_type>>item.special>>item.alchemical>>mask>>converted;
                item.kind=static_cast<dredmor::ItemKind>(kind);
                dredmor::register_factory_item(tables,static_cast<std::uint32_t>(i),item,mask,converted);
            }
            if (!std::cin) throw std::invalid_argument("incomplete registration fixture");print(tables);
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
