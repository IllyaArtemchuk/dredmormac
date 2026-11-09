#include "dredmor/items.hpp"
#include <iostream>
int main() {
    int kind,type,slot;
    while (std::cin>>kind>>type>>slot) {
        dredmor::ItemDefinition item;
        item.kind=static_cast<dredmor::ItemKind>(kind);
        item.weapon_type=type;item.armour_type=type;
        std::cout<<item.can_equip(static_cast<dredmor::EquipmentSlot>(slot))<<'\n';
    }
    return std::cin.eof()?0:1;
}
