#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("session assertion at line "+std::to_string(__LINE__)); } while(false)

dredmor::Session fixture() {
    std::istringstream room("#######\n#@....#\n#.....#\n#######\n");
    auto player=dredmor::make_player({3,2,2},"Fixture player");
    player.secondary.fill(0);player.secondary[0]=20;player.secondary[2]=5;player.health=20;player.mana=0;
    return dredmor::Session(dredmor::World::read(room),player,1,10);
}
int main() {
    using namespace dredmor;
    try {
        auto session=fixture();
        Actor enemy;enemy.name="Fixture enemy";enemy.position={2,1};enemy.health=6;
        enemy.secondary[0]=6;enemy.secondary[2]=1;enemy.experience_reward=10;
        session.spawn_enemy(enemy);
        session.drop({100,ItemKind::food,0,-1,2,true},{2,1});
        session.step(Action::east);
        check(session.world().turn()==1 && session.enemies()[0].health==1 && session.player().health==19);
        auto copy=session;
        session.step(Action::east);copy.step(Action::east);
        check(session.snapshot()==copy.snapshot() && session.enemies()[0].health==0);
        check(session.progression().experience==10 && session.progression().level==0);
        session.step(Action::east);session.pickup();
        check(session.world().turn()==3 && session.player().position.x==2 && session.inventory().find(100));
        check(session.ground_items().empty());
        auto chase=fixture();enemy.position={5,1};enemy.health=enemy.secondary[0]=10;
        chase.spawn_enemy(enemy);chase.step(Action::wait);
        check(chase.enemies()[0].position.x==4 && chase.player().health==20);
        chase.step(Action::wait);chase.step(Action::wait);chase.step(Action::wait);
        check(chase.enemies()[0].position.x==2 && chase.player().health==19);
        auto lethal=fixture();enemy.position={2,1};enemy.secondary[2]=100;
        lethal.spawn_enemy(enemy);lethal.step(Action::wait);
        check(!lethal.player().alive() && lethal.world().turn()==1);
        lethal.step(Action::east);check(lethal.world().turn()==1);
        auto atomic=fixture();enemy.position={2,1};enemy.secondary[8]=101;
        auto hero=make_player({3,2,2});hero.secondary[8]=101;
        // Copying after an invalid action must leave all gameplay/RNG state unchanged.
        auto before=atomic.snapshot();bool rejected=false;
        try { atomic.step(static_cast<Action>(999)); } catch (const std::invalid_argument&) { rejected=true; }
        check(rejected && atomic.snapshot()==before);
        auto objects=fixture();CustomRoomDefinition obstacle;obstacle.placement.relative={2,1};obstacle.sprite.image="fixture.spr";
        check(objects.create_custom_object(obstacle)==0 && !objects.passable({2,1}) && objects.world().passable({2,1}));
        objects.step(Action::east);check(objects.world().turn()==0 && objects.player().position.x==1 && objects.events()[0].kind==EventKind::blocked);
        auto decorative=fixture();obstacle.kind=CustomRoomKind::engraving;
        check(decorative.create_custom_object(obstacle)==0 && decorative.passable({2,1}));
        decorative.step(Action::east);check(decorative.world().turn()==1 && decorative.player().position.x==2);
        auto detour=fixture();obstacle.kind=CustomRoomKind::blocker;obstacle.placement.relative={4,1};detour.create_custom_object(obstacle);
        enemy.position={5,1};enemy.health=enemy.secondary[0]=10;detour.spawn_enemy(enemy);detour.step(Action::wait);
        check(detour.enemies()[0].position.x==5 && detour.enemies()[0].position.y==2);
        auto bad_object=fixture();std::ostringstream before_object;bad_object.save(before_object);
        obstacle.placement.relative={99,1};obstacle.placement.percent=0;rejected=false;
        try {bad_object.create_custom_object(obstacle);} catch(const std::invalid_argument&) {rejected=true;}
        std::ostringstream after_object;bad_object.save(after_object);check(rejected && before_object.str()==after_object.str());
        obstacle.placement.percent=-1;check(!bad_object.create_custom_object(obstacle));
        std::ostringstream skipped;bad_object.save(skipped);const auto skipped_xml=parse_xml(skipped.str());
        check(skipped_xml.first("random")->attribute("libc")=="16807" && bad_object.custom_objects().empty());
        Progression xp;check(!xp.gain(10,10) && xp.experience==10);
        check(xp.gain(1,10) && xp.experience==0 && xp.level==1 && xp.unspent_points==1);
        std::cout<<"session: attacks, enemy turns, death, pickup, progression and atomic rejection passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
