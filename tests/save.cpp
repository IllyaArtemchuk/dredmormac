#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
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
int main() {
    try {
        auto original=fixture();for (int i=0;i<7;++i) original.step(Action::east);
        const auto text=saved(original);std::istringstream input(text);auto loaded=Session::load(input);
        check(original.snapshot()==loaded.snapshot() && saved(loaded)==text);
        for (int i=0;i<30;++i) {
            const auto action=i%3==0?Action::wait:Action::east;
            original.step(action);loaded.step(action);
            check(original.snapshot()==loaded.snapshot() && saved(original)==saved(loaded));
        }
        auto invalid=text;auto p=invalid.find("version=\"1\"");invalid.replace(p,11,"version=\"2\"");reject(invalid);
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
        std::cout<<"save/load: exact state, future random turns, equipment, escaping and malformed-input rejection passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
