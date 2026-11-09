#include "dredmor/inventory.hpp"
#include <iostream>
#include <string>
int main() {
    dredmor::Inventory inventory;
    std::string command;
    while (std::cin>>command) {
        if (command=="reset") { inventory={};continue; }
        if (command!="insert") return 1;
        dredmor::InventoryItem item;
        unsigned kind;int stackable,prefer_belt;
        if (!(std::cin>>item.instance>>kind>>item.definition>>item.weapon_type>>item.quantity>>stackable>>prefer_belt)) return 1;
        item.kind=static_cast<dredmor::ItemKind>(kind);item.stackable=stackable!=0;
        try {
            auto result=inventory.insert(item,prefer_belt!=0);
            std::cout<<static_cast<int>(result.outcome)<<' '
                     <<(result.position?static_cast<int>(result.position->location):-1)<<' '
                     <<(result.position?static_cast<int>(result.position->slot):-1)<<' '<<result.destination;
            for (const auto& slot:inventory.bag()) std::cout<<' '<<(slot?slot->instance:0)<<' '<<(slot?slot->quantity:0);
            for (const auto& slot:inventory.belt()) std::cout<<' '<<(slot?slot->instance:0)<<' '<<(slot?slot->quantity:0);
            std::cout<<'\n';
        } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
    }
}
