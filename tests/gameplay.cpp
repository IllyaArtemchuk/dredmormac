#include "dredmor/combat.hpp"
#include "dredmor/inventory.hpp"
#include "dredmor/random.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("gameplay assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    using namespace dredmor;
    try {
        CombatInput attack;attack.damage[0]=8;attack.damage[3]=10;attack.melee_power=4;attack.armour=4;
        attack.block=101;
        auto block=resolve_combat(attack,[] {return 0;});
        check(block.blocked && block.physical==2 && block.elemental==5 && block.total==7 && block.draws==4);
        attack.critical=101;attack.dodge=101;attack.counter=101;
        auto crit=resolve_combat(attack,[] {return 0;});
        check(crit.critical && !crit.blocked && crit.physical==12 && crit.total==22 && crit.draws==4);
        attack.critical=99;
        auto edge=resolve_combat(attack,[] {return 2147483647;});
        check(!edge.critical && edge.outcome==CombatOutcome::dodged && edge.draws==2);
        Inventory inventory;
        InventoryItem food{1,ItemKind::food,2,-1,3,true};
        check(inventory.insert(food).position->location==InventoryLocation::belt);
        food.instance=2;food.quantity=4;
        check(inventory.insert(food).outcome==InsertOutcome::stacked && inventory.find(1)->quantity==7);
        auto split=inventory.split_one(1,3);
        check(split && split->instance==3 && split->quantity==1 && inventory.find(1)->quantity==6);
        for (std::uint64_t id=10;id<58;++id)
            check(inventory.insert({id,ItemKind::armour,0,-1,1,false}).outcome==InsertOutcome::placed);
        check(inventory.insert({60,ItemKind::armour,0,-1,1,false}).outcome==InsertOutcome::full);
        check(inventory.remove(10).has_value() && inventory.insert({60,ItemKind::armour,0,-1,1,false}).position->slot==0);
        auto item=read_item(parse_xml("<item name='Fixture'><armour type='head'/><secondaryBuff id='9' amount='2'/><secondarybuff id='9' amount='5'/><resistBuff toxic='3'/></item>"));
        check(item.modifiers.secondary[9]==5 && item.modifiers.resistance[7]==3);
        check(item.can_equip(EquipmentSlot::head) && !item.can_equip(EquipmentSlot::chest));
        DarwinRandom darwin(1);
        for (auto expected:{16807,282475249,1622650073,984943658,1144108930}) check(darwin.next()==expected);
        std::cout<<"combat, inventory, typed items and RNG fixtures passed\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
