#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("session door assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
World room() {std::istringstream input("#########\n#.......#\n#...@...#\n#..###..#\n#.......#\n#.......#\n#########\n");return World::read(input);}
std::string saved(const Session& s) {std::ostringstream out;s.save(out);return out.str();}
SessionDoors doors(bool locked=false) {
    SessionDoors result(DungeonLayout(9,7,std::vector<DungeonCell>(63)));DoorState door;
    door.assembly={{4,3},true,locked,false,{{{3,3},{4,3},{5,3}}}};result.doors.push_back(door);result.lockpick_xp=5;result.attack_sound="kick";
    for (auto p:door.assembly.parts) {result.layout.at(p.x,p.y).kind=p.x==4?6:2;result.layout.at(p.x,p.y).door_flags=locked?3:1;}
    return result;
}
int main() {
    try {
        Session s(room(),make_player({3,2,2}),7);s.floor_doors(doors());const auto initial=saved(s);
        check(parse_xml(initial).attribute("version")=="21" && s.world().passable({4,3}) && !s.passable({4,3}) && !s.world().passable({3,3}));
        s.step(Action::south);check(s.doors()->doors[0].assembly.open && s.world().turn()==1 && s.player().position.y==2 && s.doors()->frames[0]==1);
        check(s.last_door_click()->sounds[0]=="door_open" && s.passable({4,3}) && !s.world().passable({3,3}));
        s.step(Action::south);check(s.player().position.y==3 && s.world().turn()==2);
        check(!s.use_door(0,1).handled && s.world().turn()==2);
        check(s.use_door(0,0).messages[0]=="The doorway is blocked!" && s.world().turn()==3 && s.doors()->doors[0].assembly.open);
        s.step(Action::south);s.step(Action::east);check(s.use_door(0,2).handled && !s.doors()->doors[0].assembly.open && s.doors()->frames[0]==0);
        std::istringstream encoded(saved(s));auto restored=Session::load(encoded);check(saved(restored)==saved(s) && !restored.passable({4,3}));
        auto bad=saved(s);auto pos=bad.find("frame=\"0\"");bad.replace(pos,9,"frame=\"3\"");bool rejected=false;
        try {std::istringstream in(bad);Session::load(in);} catch(const std::exception&) {rejected=true;}check(rejected);
        const auto before=saved(s);auto malformed=doors();malformed.doors.push_back(malformed.doors[0]);rejected=false;
        try {s.floor_doors(std::move(malformed));} catch(const std::exception&) {rejected=true;}check(rejected && saved(s)==before);
        ItemDefinition definition;definition.name="Lockpicks";definition.kind=ItemKind::misc;definition.builtin_subtype=0;
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{definition});
        Session picking(room(),make_player({3,2,2}),7);picking.equipment_profile({3,2,2},definitions);auto locked=doors(true);locked.burglary=true;picking.floor_doors(locked);
        Inventory::Belt belt{};InventoryItem pick;pick.instance=1;pick.kind=ItemKind::misc;pick.stackable=true;pick.quantity=2;pick.for_sale=true;belt[8]=pick;picking.inventory()=Inventory::restore({},belt);
        const auto libc=parse_xml(saved(picking)).first("random")->attribute("libc");picking.step(Action::south);
        check(picking.inventory().belt()[8]->quantity==1 && picking.inventory().belt()[8]->for_sale && picking.progression().experience==5 && picking.doors()->doors[0].assembly.open);
        check(parse_xml(saved(picking)).first("random")->attribute("libc")==libc);
        std::istringstream picked(saved(picking));check(saved(Session::load(picked,definitions))==saved(picking));
        Session kicking(room(),make_player({3,2,2}),7);locked=doors(true);locked.doors[0].kicks[1]=3;kicking.floor_doors(locked);kicking.step(Action::south);
        check(kicking.doors()->doors[0].destroyed && kicking.world().passable({3,3}) && kicking.world().passable({5,3}) && kicking.world().turn()==1);
        std::istringstream shattered(saved(kicking));check(saved(Session::load(shattered))==saved(kicking));
        kicking.floor_stairs({{{5,2},false,0}});Session lower(room(),make_player({3,2,2}),1);lower.depth_context({1,false,-1});lower.floor_stairs({{{5,2},true,0}});lower.floor_doors(doors(true));
        FloorRegistry registry;registry.add_floor(kicking,lower.floor_state());
        check(registry.step(kicking,Action::east)==FloorTravel::travelled && !kicking.doors()->doors[0].destroyed && kicking.doors()->doors[0].assembly.locked);
        check(registry.travel(kicking,true)==FloorTravel::travelled && kicking.doors()->doors[0].destroyed && kicking.world().passable({3,3}));
        std::ostringstream run;registry.save(kicking,run);std::istringstream run_input(run.str());auto copy=FloorRegistry::load(run_input);std::ostringstream copy_run;copy.second.save(copy.first,copy_run);check(run.str()==copy_run.str());
        std::cout<<"session door click/bump, wall-end collision, blocked closing, lockpick/XP/unpaid consumption, shatter, save21 and cached floors passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
