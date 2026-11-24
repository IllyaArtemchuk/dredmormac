#include "dredmor/room_placement.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room placement assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomLevelRegistry registry{3,3,{1,2,1},{1,3,1}};
        unregister_room_object(registry,1,true);check((registry.objects==std::vector<std::uint64_t>{2,1}) && (registry.monsters==std::vector<std::uint64_t>{3,1}));
        register_room_object(registry,1);check((registry.objects==std::vector<std::uint64_t>{2,1,1}));
        RoomPlacementWorld world;world.levels[1]={3,3,{1,2,1},{1,3,1}};world.levels[2]={2,2,{1},{1}};
        RoomPlacementObject object{{1,1,7,"named",false,{1,1}},true,RoomSquareIdentity{1,{1,1}}};
        world.squares[*object.square]={2,1,1};
        teleport_room_object(object,world,{0,0},2);
        check(object.room.level==2 && object.room.assigned_room==7 && object.room.id=="named");
        check((world.levels.at(1).objects==std::vector<std::uint64_t>{2,1}) && (world.levels.at(1).monsters==std::vector<std::uint64_t>{3,1}));
        check((world.levels.at(2).objects==std::vector<std::uint64_t>{1,1}) && (world.levels.at(2).monsters==std::vector<std::uint64_t>{1}));
        check((world.squares.at({1,{1,1}})==std::vector<std::uint64_t>{2,1}));
        teleport_room_object(object,world,{0,0},2);check(world.squares.at({2,{0,0}}).size()==1);
        teleport_room_object(object,world,{-1,0},2);check(!object.square && object.room.position.x==-1 && world.squares.at({2,{0,0}}).empty());
        world.levels.at(2).objects.clear();teleport_room_object(object,world,{1,1},2);check(world.levels.at(2).objects.size()==1);
        bool rejected=false;try {teleport_room_object(object,world,{0,0},0);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && object.room.level==2 && object.room.position.x==1);
        // Monster override maintains its separate monster vector as well as
        // the shared object registry, repairing missing same-level entries.
        world.levels.at(2).monsters.clear();teleport_room_monster_forced(object,world,{0,1},2);
        check(world.levels.at(2).monsters==std::vector<std::uint64_t>{1});
        teleport_room_monster_forced(object,world,{1,1},2);check(world.levels.at(2).monsters.size()==1);
        world.levels.at(1).monsters={1,2};teleport_room_monster_forced(object,world,{1,1},1);
        check((world.levels.at(1).monsters==std::vector<std::uint64_t>{1,2,1}) && world.levels.at(2).monsters.empty());
        // Forced player teleport still updates visibility after registration,
        // including an outside coordinate, without movement-only effects.
        RoomPlacementObject player{{6,0,9,"player",true,{0,0}},false,{}};
        bool visible=false;
        teleport_room_player_forced(player,world,{-1,0},2,[&](auto level,auto point) {
            visible=true;check(level==2 && point.x==-1 && point.y==0 && player.room.level==2 && !player.square);
            check(world.levels.at(2).objects.back()==6 && player.room.assigned_room==9);
        });
        check(visible && player.room.id=="player");
        rejected=false;try {teleport_room_player_forced(player,world,{0,0},1,{});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && player.room.level==2 && player.room.position.x==-1);
        std::cout<<"base registration and teleport preserve scope, ordered duplicate removal, level transfer, tile attachment and same-level registration repair\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
