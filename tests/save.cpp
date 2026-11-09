#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>
#define check(value) do { if (!(value)) throw std::runtime_error("save assertion at line "+std::to_string(__LINE__)); } while(false)
using namespace dredmor;
Session fixture() {
    std::istringstream room("#######\n#@....#\n#.....#\n#######\n");
    auto hero=make_player({3,2,2},"A&B \"fixture\"\nplayer");
    hero.secondary[2]=1;hero.secondary[6]=12;hero.secondary[7]=20;
    Session session(World::read(room),hero,1234);
    Actor enemy;enemy.position={2,1};enemy.name="Enemy <fixture>";enemy.health=enemy.secondary[0]=1000;
    enemy.secondary[2]=1;enemy.secondary[4]=10;enemy.secondary[6]=15;
    session.spawn_enemy(enemy);
    session.inventory().insert({100,ItemKind::food,0,-1,3,true});
    session.drop({101,ItemKind::armour,0,-1,1,false},{3,1});
    return session;
}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
void reject(std::string text) {
    bool rejected=false;
    try {std::istringstream input(text);Session::load(input);} catch (const std::exception&) {rejected=true;}
    check(rejected);
}
int main(int argc,char** argv) {
    try {
        auto original=fixture();for (int i=0;i<7;++i) original.step(Action::east);
        const auto text=saved(original);std::istringstream input(text);auto loaded=Session::load(input);
        check(original.snapshot()==loaded.snapshot() && saved(loaded)==text);
        for (int i=0;i<30;++i) {
            const auto action=i%3==0?Action::wait:Action::east;
            original.step(action);loaded.step(action);
            check(original.snapshot()==loaded.snapshot() && saved(original)==saved(loaded));
        }
        auto invalid=text;auto p=invalid.find("version=\"3\"");invalid.replace(p,11,"version=\"4\"");reject(invalid);
        auto legacy=text;legacy.replace(legacy.find("version=\"3\""),11,"version=\"1\"");
        p=legacy.find("<skills ");const auto end=legacy.find("</skills>\n",p);check(p!=std::string::npos && end!=std::string::npos);
        legacy.erase(p,end+10-p);std::istringstream legacy_input(legacy);auto legacy_loaded=Session::load(legacy_input);
        check(saved(legacy_loaded)==text);
        invalid=text;p=invalid.find("index=\"624\"");check(p!=std::string::npos);invalid.replace(p,11,"index=\"626\"");reject(invalid);
        invalid=text;p=invalid.find("<enemy id=\"2\"");check(p!=std::string::npos);invalid.replace(p,13,"<enemy id=\"1\"");reject(invalid);
        reject("<!DOCTYPE d [<!ENTITY x 'bad'>]>"+text);reject("<dredmor-session/>");
        invalid=text;p=invalid.find("</dredmor-session>");invalid.insert(p,"<random/>");reject(invalid);
        auto dead=fixture();while (dead.player().alive()) dead.step(Action::wait);
        std::istringstream dead_input(saved(dead));auto dead_loaded=Session::load(dead_input);
        check(!dead_loaded.player().alive() && saved(dead_loaded)==saved(dead));
        auto definition=read_item(parse_xml("<item name='Fixture helmet'><armour type='head' ac='1'/><secondarybuff id='10' amount='2'/></item>"));
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{definition});
        std::istringstream room("#####\n#@..#\n#####\n");Session gear(World::read(room),make_player({3,2,2}),3);
        gear.equipment_profile({3,2,2},definitions);
        gear.inventory().insert({500,ItemKind::armour,0,-1,1,false},false);
        check(gear.equip(500,EquipmentSlot::head) && gear.player().secondary[10]==3);
        std::istringstream gear_input(saved(gear));auto gear_loaded=Session::load(gear_input,definitions);
        check(saved(gear_loaded)==saved(gear) && gear_loaded.equipment().slots()[0]->instance==500);
        check(gear.unequip(EquipmentSlot::head) && gear_loaded.unequip(EquipmentSlot::head));
        check(saved(gear)==saved(gear_loaded));
        auto changed=definition;changed.name="Different content";
        auto wrong=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{changed});
        bool mismatch=false;try {std::istringstream bad(saved(gear));Session::load(bad,wrong);} catch (const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        auto sword=read_item(parse_xml("<item name='Fixture sword' type='0'><weapon crushing='3'/></item>"));
        auto profile_items=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{definition,sword});
        auto passive=read_ability(parse_xml("<ability name='Fixture unarmed' skill='6'><primarybuff id='0' amount='3'/><damagebuff crushing='2'/></ability>"));
        passive.skill_name="Unarmed Combat";
        auto abilities=std::make_shared<const std::vector<AbilityDefinition>>(std::vector<AbilityDefinition>{passive});
        std::vector<SkillRank> selected;for (int id=0;id<7;++id) selected.push_back({id,1});
        std::istringstream profile_room("#####\n#@..#\n#####\n");Session profile(World::read(profile_room),make_player({3,2,2}),4);
        profile.character_profile({3,2,2},profile_items,selected,abilities);
        check(profile.player().primary[0]==13 && profile.player().health==25 && profile.player().damage[0]==2);
        profile.inventory().insert({700,ItemKind::weapon,1,0,1,false},false);
        const auto profile_text=saved(profile);std::istringstream profile_input(profile_text);
        auto profile_loaded=Session::load(profile_input,profile_items,abilities);
        check(saved(profile_loaded)==profile_text && profile_loaded.skills().size()==7);
        check(profile.equip(700,EquipmentSlot::hand_left) && profile_loaded.equip(700,EquipmentSlot::hand_left));
        check(profile.player().primary[0]==10 && profile.player().damage[0]==3 && saved(profile)==saved(profile_loaded));
        check(profile.unequip(EquipmentSlot::hand_left) && profile_loaded.unequip(EquipmentSlot::hand_left));
        check(profile.player().primary[0]==13 && profile.player().damage[0]==2 && saved(profile)==saved(profile_loaded));
        auto wrong_ability=passive;wrong_ability.modifiers.primary[0]=4;
        auto wrong_abilities=std::make_shared<const std::vector<AbilityDefinition>>(std::vector<AbilityDefinition>{wrong_ability});
        mismatch=false;try {std::istringstream bad(profile_text);Session::load(bad,profile_items,wrong_abilities);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        mismatch=false;try {std::istringstream bad(profile_text);Session::load(bad,profile_items);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        auto duplicate=profile_text;auto location=duplicate.find("<skill id=\"1\"");check(location!=std::string::npos);
        duplicate.replace(location,13,"<skill id=\"0\"");
        mismatch=false;try {std::istringstream bad(duplicate);Session::load(bad,profile_items,abilities);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        check(argc==2);
        auto trap=read_item(parse_xml("<item name='Fixture trap'><trap/></item>"));
        auto toolkit=read_item(parse_xml("<item name='Fixture toolkit'><toolkit/></item>"));
        auto migrated_definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{trap,toolkit});
        std::ifstream legacy_types(argv[1]);check(legacy_types.good());
        auto migrated=Session::load(legacy_types,migrated_definitions);
        check(migrated.inventory().bag()[0]->instance==800 && migrated.inventory().bag()[0]->kind==ItemKind::trap);
        check(migrated.inventory().belt()[0]->instance==801 && migrated.inventory().belt()[0]->kind==ItemKind::toolkit);
        std::istringstream migrated_input(saved(migrated));auto migrated_again=Session::load(migrated_input,migrated_definitions);
        check(saved(migrated)==saved(migrated_again));
        auto builtin=ItemDefinition{};builtin.name="Lockpick";builtin.kind=ItemKind::misc;builtin.builtin_subtype=0;
        auto extended=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{trap,toolkit,builtin});
        std::ifstream old_with_builtins(argv[1]);auto appended=Session::load(old_with_builtins,extended);
        check(saved(appended)==saved(migrated));
        DarwinRandom item_random(1);appended.inventory().insert(make_inventory_item(builtin,2,802,item_random));
        std::istringstream appended_input(saved(appended));auto appended_loaded=Session::load(appended_input,extended);
        check(saved(appended)==saved(appended_loaded) && appended_loaded.inventory().find(802));
        auto wand=read_item(parse_xml("<item name='Fixture wand'><wand mincharge='2' maxcharge='3'/></item>"));
        auto wand_definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{wand});
        std::istringstream wand_room("#####\n#@..#\n#####\n");Session wands(World::read(wand_room),make_player({3,2,2}),5);
        wands.equipment_profile({3,2,2},wand_definitions);
        auto wand_instance=make_inventory_item(wand,0,900,item_random);wand_instance.charges=0;
        wands.inventory().insert(wand_instance);
        std::istringstream wand_input(saved(wands));auto wand_loaded=Session::load(wand_input,wand_definitions);
        check(wand_loaded.inventory().find(900)->charges==0 && saved(wand_loaded)==saved(wands));
        std::cout<<"save/load: exact state, future random turns, equipment, escaping and malformed-input rejection passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
