#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("dungeon assertion at line "+std::to_string(__LINE__)); } while(false)

namespace {
std::vector<dredmor::RoomStamp> pool(int size) {
    std::vector<dredmor::RoomStamp> rooms(9);
    for (std::size_t i=0;i<rooms.size();++i) {
        auto& room=rooms[i];room.width=room.height=size;
        for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
            std::uint8_t kind=x==0 || y==0 || x==size-1 || y==size-1?2:1;
            if (((x==0 || x==size-1) && y==size/2) || ((y==0 || y==size-1) && x==size/2)) kind=6;
            if (i==5 && x==3 && y==3) kind=15;
            if (i==6 && x==4 && y==4) kind=9;
            room.cells.push_back({kind,std::uint8_t(kind==6 && (y==0 || y==size-1)),std::uint8_t(i==7 && kind==2?4:0)});
        }
        if (i==0 || i==2) room.fixed_name="Fixed";
        room.shop=i==2;room.zoo=i==3;room.rules.quest=i==0;room.rules.unique=i==4;room.rules.diggle_god=i==5;
        if (i==8) room.rules.minimum_level=25;
    }
    return rooms;
}
bool same(const dredmor::GameRandom& a,const dredmor::GameRandom& b) {
    return a.libc.state()==b.libc.state() && a.mt.state().words==b.mt.state().words && a.mt.state().index==b.mt.state().index;
}
}
int main() {
    try {
        using namespace dredmor;
        RoomNameWords words{{"A0","A1","A2"},{"S0","S1","S2"},{"P0","P1","P2"},{"N0","N1","N2"}};
        GeometryOptions options;options.width=options.height=64;options.short_game=true;options.max_shops=2;
        GameRandom random;random.seed(0);
        auto result=generate_level_geometry(pool(11),words,random,options);
        // Golden values come from the original 0x195bac through BuildDoors,
        // with these synthetic inputs and supplied library/config boundaries.
        check(result.accepted && result.attempts==1 && result.rooms.size()==20 && result.shops==1);
        check(result.layout.floor_count()==1618 && result.initial_player->x==33 && result.initial_player->y==35);
        check(random.libc.state()==2116969294U && random.mt.u32()==2393880710U);
        std::size_t edges=0;unsigned unique=0,zoo=0,god=0;
        for (std::size_t i=0;i<result.rooms.size();++i) {
            const auto& room=result.rooms[i];edges+=room.neighbors.size();
            unique+=room.template_index==4;zoo+=room.template_index==3;god+=room.template_index==5;
            check(room.template_index!=8 && (i==0 || room.template_index!=0));
            for (auto neighbor:room.neighbors) {
                check(neighbor<result.rooms.size());bool reciprocal=false;
                for (auto back:result.rooms[neighbor].neighbors) reciprocal|=back==i;
                check(reciprocal);
            }
        }
        check(edges==2*(result.rooms.size()-1) && unique<=1 && zoo<=1 && god<=1);
        options.width=options.height=48;options.depth=3;options.starters={1,4};random.seed(5489);
        result=generate_level_geometry(pool(9),words,random,options);
        check(result.accepted && result.attempts==2 && result.rooms.size()==20 && result.shops==0 && !result.initial_player);
        check(result.layout.floor_count()==972 && random.libc.state()==1956328826U && random.mt.u32()==2088947962U);
        check(result.layout.at(38,14).kind==0 && result.layout.at(38,14).flags==128);
        random.seed(5489);auto before=random;options.max_attempts=1;bool rejected=false;
        try {generate_level_geometry(pool(9),words,random,options);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random));
        result=generate_geometry_attempt(pool(9),words,random,options);
        check(!result.accepted && !same(before,random));
        random=before;options.max_candidate_draws=1;rejected=false;
        try {generate_geometry_attempt(pool(9),words,random,options);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random));
        options.starters={99};rejected=false;
        try {generate_geometry_attempt(pool(9),words,random,options);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && same(before,random));
        DungeonLayout doors(11,11,std::vector<DungeonCell>(121));
        doors.at(3,7)={6,42,2,0x80}; // vertical, locked
        doors.at(3,4)={6,43,1,0x81}; // bad horizontal doorway
        doors.at(1,4).kind=doors.at(5,4).kind=1;
        doors.at(7,7)={6,44,5,0x82}; // horizontal, open
        doors.at(5,7).kind=13;doors.at(9,7).kind=1;
        doors.at(0,0)={6,45,7,0x83}; // ignored boundary row
        auto plan=build_door_plan(doors);
        check(plan.doors.size()==2 && plan.repaired.size()==1);
        check(plan.repaired[0].x==3 && plan.repaired[0].y==4 && doors.at(3,4).kind==1);
        check(doors.at(3,4).room_id==43 && doors.at(3,4).door_flags==1 && doors.at(3,4).flags==0x81);
        check(plan.doors[0].center.x==3 && plan.doors[0].center.y==7 && !plan.doors[0].horizontal && plan.doors[0].locked);
        check(plan.doors[0].parts[0].y==6 && plan.doors[0].parts[2].y==8 && !plan.doors[0].open);
        check(plan.doors[1].horizontal && plan.doors[1].open && !plan.doors[1].locked);
        check(plan.doors[1].parts[0].x==6 && plan.doors[1].parts[2].x==8 && doors.at(0,0).kind==6);
        DungeonLayout malformed(10,10,std::vector<DungeonCell>(100));
        malformed.at(3,4).kind=6;malformed.at(3,4).door_flags=1;
        malformed.at(1,4).kind=malformed.at(5,4).kind=1;
        malformed.at(9,4).kind=6;malformed.at(9,4).door_flags=1;rejected=false;
        try {build_door_plan(malformed);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && malformed.at(3,4).kind==6);
        auto& active=plan.doors[0];unsigned queries=0;
        auto no_obstruction=[&](Position) {++queries;return false;};
        auto transition=toggle_door(doors,active,1,no_obstruction);
        check(transition.outcome==DoorOutcome::opened && transition.handled && active.open && queries==0);
        for (auto p:active.parts) check((doors.at(p.x,p.y).door_flags&4)!=0);
        transition=activate_remote_door(doors,active,0,no_obstruction);
        check(transition.outcome==DoorOutcome::locked && !transition.handled && active.open && queries==0);
        transition=toggle_door(doors,active,1,no_obstruction);
        check(transition.outcome==DoorOutcome::center_ignored && !transition.handled && active.open && queries==0);
        // Original end-cap closing checks only itself and its immediate neighbor,
        // so an obstruction at the far end does not prevent the close.
        transition=toggle_door(doors,active,0,[&](Position p){++queries;return p.y==8;});
        check(transition.outcome==DoorOutcome::closed && transition.handled && !active.open && queries==2);
        for (auto p:active.parts) check((doors.at(p.x,p.y).door_flags&4)==0);
        active.locked=false;transition=activate_remote_door(doors,active,2,no_obstruction);
        check(transition.outcome==DoorOutcome::opened && transition.handled && active.open);
        std::vector<int> queried_y;
        transition=toggle_door(doors,active,2,[&](Position p){queried_y.push_back(p.y);return p.y==7;});
        check(transition.outcome==DoorOutcome::obstructed && transition.handled && active.open);
        check(queried_y==std::vector<int>({8,7}));
        rejected=false;try {toggle_door(doors,active,99,no_obstruction);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && active.open);
        rejected=false;try {toggle_door(doors,active,0,[](Position)->bool{throw std::runtime_error("occupancy unavailable");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && active.open && (doors.at(3,7).door_flags&4));
        DungeonLayout punched(7,5,std::vector<DungeonCell>(35,{0,77,8,0x1000}));
        punched.floor_count(321);punched.at(2,2).kind=punched.at(3,2).kind=6;
        punched.at(1,1).kind=13;punch_out_dungeon_content(punched);
        check(punched.at(2,2).kind==6 && punched.at(3,2).kind==1 && punched.at(1,1).kind==1);
        check(punched.at(2,0).kind==2 && punched.at(2,4).kind==0 && punched.at(4,1).kind==0);
        check(punched.floor_count()==321 && punched.at(1,1).room_id==77 && punched.at(1,1).door_flags==8 && punched.at(1,1).flags==0x1000);
        DungeonLayout bad_punch(7,5,std::vector<DungeonCell>(35));
        bad_punch.at(2,2).kind=bad_punch.at(6,4).kind=6;rejected=false;
        try {punch_out_dungeon_content(bad_punch);} catch(const std::out_of_range&) {rejected=true;}
        check(rejected && bad_punch.at(1,1).kind==0 && bad_punch.at(2,2).kind==6);
        std::cout<<"original geometry goldens, retry flags, graph constraints and atomic guard failures passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
