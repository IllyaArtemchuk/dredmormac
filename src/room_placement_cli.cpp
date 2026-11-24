#include "dredmor/room_placement.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
void read_entries(std::vector<std::uint64_t>& entries) {
    std::size_t size;std::cin>>size;if (size>4096) throw std::invalid_argument("too many entries");
    entries.resize(size);for (auto& entry:entries) std::cin>>entry;
}
void write_entries(const std::vector<std::uint64_t>& entries) {
    std::cout<<' '<<entries.size();for (auto entry:entries) std::cout<<' '<<entry;
}
int main(int argc,char** argv) {
    try {
        const std::string mode=argc==2?argv[1]:"";
        if (argc>2 || (!mode.empty() && mode!="--monster" && mode!="--player" && mode!="--item-detach")) throw std::invalid_argument("Usage: dredmor-room-placement [--monster|--player|--item-detach]");
        int w1,h1,w2,h2;
        while (std::cin>>w1>>h1>>w2>>h2) {
            if (w1<1 || h1<1 || w2<1 || h2<1 || w1>16 || h1>16 || w2>16 || h2>16) throw std::invalid_argument("invalid room dimensions");
            RoomPlacementObject object;object.room.identity=1;object.room.id="target";
            std::uint64_t square_level,target;Position square,destination;
            std::cin>>object.room.level>>object.room.assigned_room>>object.monster>>object.room.position.x>>object.room.position.y>>square_level>>square.x>>square.y>>target>>destination.x>>destination.y;
            if (square_level) object.square=RoomSquareIdentity{square_level,square};
            RoomPlacementWorld world;world.levels[1]={w1,h1,{},{}};world.levels[2]={w2,h2,{},{}};
            for (auto& [level,registry]:world.levels) {
                read_entries(registry.objects);read_entries(registry.monsters);
                for (int y=0;y<registry.height;++y) for (int x=0;x<registry.width;++x) read_entries(world.squares[{level,{x,y}}]);
            }
            if (!std::cin) throw std::runtime_error("truncated room placement fixture");
            std::vector<std::vector<std::int64_t>> visibility;
            if (mode=="--monster") teleport_room_monster_forced(object,world,destination,target);
            else if (mode=="--item-detach") detach_room_item(object,world);
            else if (mode=="--player") teleport_room_player_forced(object,world,destination,target,[&](auto level,auto point) {
                visibility.push_back({std::int64_t(level),point.x,point.y,std::int64_t(object.room.level),object.room.position.x,object.room.position.y,object.square.has_value(),std::int64_t(world.levels.at(level).objects.size()),std::int64_t(world.levels.at(level).monsters.size())});
            });
            else teleport_room_object(object,world,destination,target);
            std::cout<<object.room.level<<' '<<object.room.assigned_room<<' '<<object.room.position.x<<' '<<object.room.position.y<<' '<<object.square.has_value()<<' '<<(object.square?object.square->level:0)<<' '<<(object.square?object.square->position.x:0)<<' '<<(object.square?object.square->position.y:0);
            for (const auto& [level,registry]:world.levels) {
                write_entries(registry.objects);write_entries(registry.monsters);
                for (int y=0;y<registry.height;++y) for (int x=0;x<registry.width;++x) write_entries(world.squares.at({level,{x,y}}));
            }
            if (mode=="--monster" || mode=="--player") {std::cout<<' '<<visibility.size();for (const auto& call:visibility) for (auto value:call) std::cout<<' '<<value;}
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
