#include "dredmor/catalog.hpp"
#include "dredmor/items.hpp"
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
    if (argc<2 || argc>3) return 2;
    try {
        auto catalog=dredmor::Catalog::load(argv[1],argc==3 && std::string(argv[2])=="--expansions");
        for (const auto& entry:catalog.entries()) if (entry.node->tag=="item" && entry.database->source.filename()=="itemDB.xml") {
            auto item=dredmor::read_item(*entry.node);
            std::cout<<std::quoted(entry.database->layer)<<' '<<std::quoted(item.name)<<' '
                     <<static_cast<unsigned>(item.kind)<<' '<<item.level<<' '<<item.price<<' '
                     <<item.weapon_type<<' '<<item.armour_type<<' '<<item.armour_ac<<' '<<item.food_hp;
            for (auto value:item.modifiers.primary) std::cout<<' '<<value;
            for (auto value:item.modifiers.secondary) std::cout<<' '<<value;
            for (auto value:item.weapon_damage) std::cout<<' '<<value;
            for (auto value:item.modifiers.damage) std::cout<<' '<<value;
            for (auto value:item.modifiers.resistance) std::cout<<' '<<value;
            std::cout<<'\n';
        }
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
