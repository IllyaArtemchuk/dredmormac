#include "dredmor/combat.hpp"
#include "dredmor/inventory.hpp"
#include "dredmor/random.hpp"
#include "dredmor/monsters.hpp"
#include "dredmor/equipment.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/abilities.hpp"
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
        const auto trap=read_item(parse_xml("<item name='Fixture trap'><trap/></item>"));
        const auto toolkit=read_item(parse_xml("<item name='Fixture toolkit'><toolkit/></item>"));
        check(static_cast<unsigned>(trap.kind)==12 && static_cast<unsigned>(toolkit.kind)==11);
        check(trap.trap && trap.trap->trigger==TrapTrigger::repeat && trap.trap->mount==TrapMount::ceiling && trap.trap->facing==TrapFacing::south);
        const auto typed_trap=read_item(parse_xml("<item name='Typed trap' level='4'><trap LEVEL=' 7tail' trigger='charges' originMount='wall' originFacing='north' origin='fixture.spr' casts='Fixture Spell' charges=' 3x' targetiscaster='-1'/></item>"));
        check(typed_trap.level==7 && typed_trap.trap->trigger==TrapTrigger::charges && typed_trap.trap->mount==TrapMount::wall && typed_trap.trap->facing==TrapFacing::north);
        check(typed_trap.trap->charges==3 && typed_trap.trap->target_is_caster && typed_trap.trap->spell=="Fixture Spell" && typed_trap.trap->origin=="fixture.spr");
        const auto repeated_trap=read_item(parse_xml("<item name='Repeated trap' level='4'><trap level='8' originFacing='west' origin='first' charges='3' casts='first'/><trap/></item>"));
        check(repeated_trap.level==8 && repeated_trap.trap->facing==TrapFacing::west && repeated_trap.trap->origin=="first");
        check(repeated_trap.trap->charges==0 && repeated_trap.trap->spell.empty() && repeated_trap.trap->trigger==TrapTrigger::repeat);
        Inventory tools;
        check(tools.insert({800,trap.kind,0,0,1,false}).position->location==InventoryLocation::belt);
        check(tools.insert({801,toolkit.kind,1,0,1,true}).position->location==InventoryLocation::bag);
        auto wand=read_item(parse_xml("<item name='Fixture wand'><wand mincharge='2' maxcharge='3'/></item>"));
        DarwinRandom creation(1);const auto charged=make_inventory_item(wand,0,900,creation);
        check(charged.charges==2 && !charged.stackable && creation.state()==16807);
        wand.wand_max_charge=wand.wand_min_charge;creation.seed(1);
        check(make_inventory_item(wand,0,901,creation).charges==2 && creation.state()==1);
        check(make_inventory_item(toolkit,1,902,creation).stackable && creation.state()==1);
        check(make_inventory_item(trap,0,903,creation).stackable && creation.state()==1);
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
        auto ability=read_ability(parse_xml("<ability name='Fixture passive' skill='7' level='0'><PrimaryBuff id='0' amount='3'/><primarybuff id='0' amount='5'/><secondaryBuff id='2' amount='-10'/><damagebuff toxic='2'/></ability>"));
        ability.skill_name="Fixture skill";
        PassiveStatInput base;base.ranks={3,2,2};
        auto passive=derive_passives(base,{{7,1}},{ability});
        check(ability.modifiers.primary[0]==5 && passive.primary[0]==15 && passive.secondary[2]==-7 && passive.damage[7]==2);
        check(!player_passive_applies(ability,{{7,0}}));
        ability.skill_name="Unarmed Combat";check(player_passive_applies(ability,{{7,1}}));
        AbilityContext armed;armed.weapon_left=true;check(!player_passive_applies(ability,{{7,1}},armed));
        ability.skill_name="Shield Bearer";check(!player_passive_applies(ability,{{7,1}}));
        armed.armour_right=true;check(player_passive_applies(ability,{{7,1}},armed));
        ability.polymorphed_only=true;ability.polymorph_tag="fixture";
        check(!player_passive_applies(ability,{{7,1}},armed));
        armed.polymorph="fixture";check(player_passive_applies(ability,{{7,1}},armed));
        auto training=read_ability(parse_xml("<ability name='Fixture sword training' skill='0'><primarybuff id='0' amount='-20'/><secondarybuff id='2' amount='-10'/><damagebuff crushing='2'/></ability>"));
        training.skill_name="Swords";
        auto general=read_ability(parse_xml("<ability name='Fixture general' skill='13'><primarybuff id='0' amount='3'/></ability>"));
        general.skill_name="Fixture skill";
        auto archery=read_ability(parse_xml("<ability name='Fixture archery' skill='4'><primarybuff id='0' amount='2'/><damagebuff crushing='7'/><resistbuff crushing='4'/></ability>"));
        archery.skill_name="Archery";
        ItemDefinition trained;trained.kind=ItemKind::weapon;trained.weapon_type=0;trained.modifiers.primary[0]=5;trained.modifiers.damage[0]=3;
        Equipment trained_gear;auto unused=trained_gear.equip({910,ItemKind::weapon,0,0,1,false},EquipmentSlot::hand_left,{trained});check(!unused);
        const std::vector<SkillRank> learned={{0,1},{4,1},{13,1}};const std::vector<AbilityDefinition> rules={training,general,archery};
        auto trained_stats=trained_gear.rebuild({3,2,2},{trained},{},{},learned,rules);
        check(trained_stats.primary[0]==3 && trained_stats.secondary[2]==0 && trained_stats.damage_bonus[0]==5);
        auto bow=trained;bow.weapon_type=4;
        unused=trained_gear.equip({911,ItemKind::weapon,1,4,1,false},EquipmentSlot::ranged,{trained,bow});check(!unused);
        trained_stats=trained_gear.rebuild({3,2,2},{trained,bow},{},{},learned,rules);
        check(trained_stats.primary[0]==5 && trained_stats.damage_bonus[0]==5 && trained_stats.resistance[0]==4);
        check(weapon_skill_id(7)==45 && weapon_skill_id(8)==46 && weapon_skill_id(6)==4 && weapon_skill_id(-1)==2);
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
