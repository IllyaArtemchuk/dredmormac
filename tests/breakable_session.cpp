#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("breakable session assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& value) {std::ostringstream out;value.save(out);return out.str();}
Session session() {
    std::istringstream map("#######\n#@....#\n#.....#\n#.....#\n#######\n");return Session(World::read(map),make_player({3,2,2}),1);
}
int main() {
    try {
        RoomStamp stamp{7,5,std::vector<TemplateCell>(35)};
        const auto definition=read_breakable_room_object(parse_xml("<custombreakable x='2' y='1' png='fixture.spr' breaking='breaking.spr' broken='broken.spr' hitsNeededToBreak='2' id='fragile'/>"),{},stamp);
        auto original=session();int calls=0;
        auto index=original.create_breakable(definition,{0,0},[&](const auto&){++calls;return BreakableFrames{3,1,4};});
        check(index==0 && calls==1 && !original.passable({2,1}));
        original.create_pedestal(read_room_pedestal(parse_xml("<pedestal x='4' y='1' name='Fixture' text='Read this'/>"),{},stamp));
        check(!original.passable({4,1}) && original.custom_objects()[0].kind==CustomRoomKind::pedestal);
        original.step(Action::east);
        check(original.world().turn()==1 && original.player().position.x==1 && original.breakables()[0].hits_taken==1 && original.breakables()[0].object.frame==1);
        check(original.events().size()==1 && original.events()[0].kind==EventKind::object_hit);
        const auto partial=saved(original);std::istringstream input(partial);auto loaded=Session::load(input);
        check(saved(loaded)==partial && !loaded.passable({2,1}) && !loaded.passable({4,1}));
        for (auto* current:{&original,&loaded}) {
            current->step(Action::east);check(current->world().turn()==2 && current->player().position.x==1 && current->passable({2,1}));
            check(current->events().size()==2 && current->events()[0].kind==EventKind::object_hit && current->events()[1].kind==EventKind::object_broken);
            check(current->breakables()[0].object.frame==3 && current->breakables()[0].hits_taken==2);
            current->step(Action::east);check(current->world().turn()==3 && current->player().position.x==2);
        }
        check(saved(original)==saved(loaded));const auto broken=saved(original);
        check(!original.strike_breakable(0) && saved(original)==broken);
        bool rejected=false;try {original.strike_breakable(99);} catch(const std::out_of_range&) {rejected=true;}
        check(rejected && saved(original)==broken);
        auto fail=session();auto gated=definition;gated.placement.percent=0;const auto unchanged=saved(fail);calls=0;rejected=false;
        try {fail.create_breakable(gated,{0,0},[&](const auto&)->BreakableFrames {++calls;throw std::runtime_error("synthetic asset failure");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && calls==1 && saved(fail)==unchanged);
        gated.placement.percent=-1;calls=0;
        check(!fail.create_breakable(gated,{0,0},[&](const auto&){++calls;return BreakableFrames{0,0,0};}) && calls==0 && fail.breakables().empty());
        check(parse_xml(saved(fail)).first("random")->attribute("libc")=="16807");
        auto outside=definition;outside.placement.relative={100,100};const auto before_outside=saved(fail);rejected=false;
        try {fail.create_breakable(outside);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(fail)==before_outside);
        for (const auto& change:std::vector<std::pair<std::string,std::string>>{{"intact-frames=\"3\"","intact-frames=\"0\""},
            {"hits-needed=\"2\"","hits-needed=\"2147483648\""},{"single-delay=\"10\"","single-delay=\"11\""}}) {
            auto invalid=partial;auto at=invalid.find(change.first);check(at!=std::string::npos);invalid.replace(at,change.first.size(),change.second);rejected=false;
            try {std::istringstream bad(invalid);Session::load(bad);} catch(const std::runtime_error&) {rejected=true;}
            check(rejected);
        }
        std::istringstream corridor("#####\n#@..#\n#####\n");Session chase(World::read(corridor),make_player({3,2,2}),1);
        chase.create_breakable(definition,{0,0},BreakableFrames{3,1,4});Actor enemy;enemy.position={3,1};enemy.health=enemy.secondary[0]=10;
        chase.spawn_enemy(enemy);chase.step(Action::east);check(chase.enemies()[0].position.x==3);
        chase.step(Action::east);check(chase.enemies()[0].position.x==2 && chase.player().position.x==1);
        std::cout<<"breakable ownership/collision/turn policy, pedestal persistence, resumed hits, asset gate rollback and saved-state validation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
