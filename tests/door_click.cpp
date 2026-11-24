#include "dredmor/doors.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("door click assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        auto ability=read_ability(parse_xml("<ability name='Lucky Pick' skill='26' level='1'><flags opendoors='1'/></ability>"));
        check(!has_free_door_pick({{26,1}},{ability}) && has_free_door_pick({{26,2}},{ability}));
        check(!has_free_door_pick({{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},{26,2}},{ability}));
        ability.source=parse_xml("<ability><flags OpenDoors='1' opendoors='0'/></ability>");check(!has_free_door_pick({{26,2}},{ability}));
        DungeonLayout layout(9,9,std::vector<DungeonCell>(81));DoorState door;
        door.assembly={{4,4},true,true,false,{{{3,4},{4,4},{5,4}}}};
        for (auto p:door.assembly.parts) {layout.at(p.x,p.y).kind=2;layout.at(p.x,p.y).door_flags=2;}
        DoorClickContext context;context.player={3,3};context.belt_picks[7]=2;context.bag_picks[0]=1;context.burglary=true;context.depth=4;context.lockpick_xp=5;
        GameRandom random;random.seed(7);const auto before=random;
        auto result=click_door(layout,door,0,context,random,[](Position){return false;});
        check(result.handled && result.eye_candy && *result.experience==25 && door.assembly.open && !door.assembly.locked);
        check(context.belt_picks[7]==1 && context.bag_picks[0]==1 && *result.consumed_pick==7 && !result.destroyed_pick);
        check(random.libc.state()==before.libc.state() && random.mt.state().index==before.mt.state().index);
        check(layout.at(3,4).door_flags==4 && layout.at(4,4).door_flags==4 && layout.at(5,4).door_flags==6);
        check(result.sounds==std::vector<std::string>{"door_open"});
        context.player={4,4};result=click_door(layout,door,1,context,random,[](Position){return false;});check(!result.handled && door.assembly.open);
        context.player={3,3};bool rejected=false;
        try {click_door(layout,door,0,context,random,[](Position)->bool {throw std::runtime_error("occupancy unavailable");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && door.assembly.open && context.belt_picks[7]==1 && random.libc.state()==before.libc.state());
        result=click_door(layout,door,0,context,random,[](Position){return true;});check(result.handled && door.assembly.open && result.messages[0]=="The doorway is blocked!");
        result=click_door(layout,door,0,context,random,[](Position){return false;});check(result.handled && !door.assembly.open);
        door.assembly.locked=true;door.kicks[2]=3;context.player={5,5};context.belt_picks.fill(0);context.bag_picks.fill(0);context.attack_sound="kick";
        result=click_door(layout,door,2,context,random,[](Position){return false;});
        check(door.destroyed && result.destroyed_parts==std::vector<std::size_t>({2,1,0}) && result.sounds==std::vector<std::string>({"kick","breakable"}));
        for (auto p:door.assembly.parts) check(layout.at(p.x,p.y).kind==1);
        check(random.mt.state().index==1 && result.animation_facing==2);
        rejected=false;try {click_door(layout,door,2,context,random,{});} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        std::cout<<"door belt priority/Burglary, end lock-flag quirk, center/obstruction handling, fourth-kick destruction and rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
