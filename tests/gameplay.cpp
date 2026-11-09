#include "dredmor/combat.hpp"
#include "dredmor/inventory.hpp"
#include "dredmor/random.hpp"
#include "dredmor/monsters.hpp"
#include "dredmor/equipment.hpp"
#include "dredmor/selection.hpp"
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
        const auto meal=read_item(parse_xml("<item name='Fixture meal' special='1' debug='-2' level='3'><food hp='2' mp='4'/></item>"));
        check(meal.kind==ItemKind::food && meal.special && meal.debug && meal.food_hp==2 && meal.food_mp==4 && meal.level==3);
        DarwinRandom darwin(1);
        for (auto expected:{16807,282475249,1622650073,984943658,1144108930}) check(darwin.next()==expected);
        DarwinRandom bounded(1);check(bounded.bounded(0)==0 && bounded.state()==1);
        check(bounded.bounded(0x7fffffff)==0 && bounded.state()==1);
        check(bounded.bounded(1)==0 && bounded.state()==16807);
        std::vector<FoodCandidate> pool={{0,1,false,false},{1,1,false,false}};
        DarwinRandom selection(1);check(select_edible(pool,selection)==0 && selection.state()==16807);
        selection.seed(1);check(select_booze(pool,selection)==1);
        selection.seed(1);bool invalid_pool=false;
        try {select_booze({{0,1,false,false}},selection);} catch(const std::invalid_argument&) {invalid_pool=true;}
        check(invalid_pool && selection.state()==1);
        auto parent=read_monster(parse_xml("<monster name='Parent'><stats numFig='2' xpValue='5'/><damage crushing='1' acidic='2'/><idleSprite down='fixture.spr'/></monster>"));
        auto variant=read_monster(parse_xml("<monster name='Variant'><stats numRog='1' xpValue='10'/><damage acidic='0' voltaic='2'/></monster>"),&parent);
        check(variant.ranks==ClassRanks({2,0,1}) && variant.damage[0]==1 && variant.damage[10]==0 && variant.damage[6]==2);
        check(variant.idle_sprites[3]=="fixture.spr" && variant.source_chain.size()==2);
        auto tweaks=Tweaks::read(parse_xml("<tweakDB><tweak name='a' ival='2'/><easy><tweak name='a' ival='3'/><tweak name='float only' fval='9'/></easy></tweakDB>"));
        check(tweaks.integer(Difficulty::easy,"a")==3 && tweaks.integer(Difficulty::hard,"a")==2);
        check(tweaks.integer(Difficulty::easy,"float only")==0 && tweaks.real(Difficulty::easy,"float only")==9);
        std::vector<ItemDefinition> definitions={item};
        Inventory pack;Equipment worn;
        check(pack.insert({100,ItemKind::armour,0,-1,1,false},false).outcome==InsertOutcome::placed);
        check(!equip_from_inventory(pack,worn,100,EquipmentSlot::feet,definitions) && pack.find(100));
        check(equip_from_inventory(pack,worn,100,EquipmentSlot::head,definitions) && !pack.find(100));
        auto equipped=worn.rebuild({3,2,2},definitions);
        check(equipped.secondary[9]==8 && equipped.resistance[7]==3);
        check(unequip_to_inventory(pack,worn,EquipmentSlot::head) && pack.find(100));
        std::cout<<"combat, inventory, typed items and RNG fixtures passed\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
