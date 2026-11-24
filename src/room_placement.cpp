#include "dredmor/room_placement.hpp"
#include <algorithm>
#include <stdexcept>

namespace dredmor {
namespace {
void erase_first(std::vector<std::uint64_t>& entries,std::uint64_t identity) {
    const auto found=std::find(entries.begin(),entries.end(),identity);
    if (found!=entries.end()) entries.erase(found);
}
}
void register_room_object(RoomLevelRegistry& level,std::uint64_t identity) {level.objects.push_back(identity);}
void unregister_room_object(RoomLevelRegistry& level,std::uint64_t identity,bool monster) {
    if (monster) erase_first(level.monsters,identity);
    erase_first(level.objects,identity);
}
void teleport_room_object(RoomPlacementObject& object,RoomPlacementWorld& world,Position destination,std::uint64_t level) {
    if (object.destroyed || !level || !world.levels.count(level) || (object.room.level && !world.levels.count(object.room.level)))
        throw std::invalid_argument("room teleport requires registered owning levels");
    auto& target=world.levels.at(level);
    if (object.square) {
        const auto found=world.squares.find(*object.square);
        if (found!=world.squares.end()) erase_first(found->second,object.room.identity);
    }
    object.room.position=destination;
    if (object.room.level!=level) {
        if (object.room.level) unregister_room_object(world.levels.at(object.room.level),object.room.identity,object.monster);
        register_room_object(target,object.room.identity);object.room.level=level;
    } else if (std::find(target.objects.begin(),target.objects.end(),object.room.identity)==target.objects.end())
        register_room_object(target,object.room.identity);
    if (destination.x>=0 && destination.y>=0 && destination.x<target.width && destination.y<target.height) {
        object.square=RoomSquareIdentity{level,destination};auto& entries=world.squares[*object.square];
        if (std::find(entries.begin(),entries.end(),object.room.identity)==entries.end()) entries.push_back(object.room.identity);
    } else object.square.reset();
}
void teleport_room_monster_forced(RoomPlacementObject& object,RoomPlacementWorld& world,Position destination,std::uint64_t level) {
    if (!object.monster) throw std::invalid_argument("monster teleport requires monster identity");
    const auto previous=object.room.level;
    teleport_room_object(object,world,destination,level);
    auto& monsters=world.levels.at(level).monsters;
    if (previous!=level || std::find(monsters.begin(),monsters.end(),object.room.identity)==monsters.end())
        monsters.push_back(object.room.identity);
}
void teleport_room_player_forced(RoomPlacementObject& object,RoomPlacementWorld& world,Position destination,std::uint64_t level,
    const std::function<void(std::uint64_t,Position)>& visibility) {
    if (object.monster || !visibility) throw std::invalid_argument("player teleport requires player identity and visibility hook");
    teleport_room_object(object,world,destination,level);visibility(level,object.room.position);
}
}
