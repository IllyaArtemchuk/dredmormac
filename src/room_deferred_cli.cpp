#include "dredmor/room_deferred.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
std::size_t count() {std::size_t value;std::cin>>value;if (!std::cin || value>1024) throw std::invalid_argument("invalid deferred fixture count");return value;}
void entries(std::vector<std::uint64_t>& values) {values.resize(count());for (auto& value:values) std::cin>>value;}
void write_entries(const std::vector<std::uint64_t>& values) {std::cout<<' '<<values.size();for (auto value:values) std::cout<<' '<<value;}
void write_teleports(const std::vector<RoomTeleport>& values) {std::cout<<' '<<values.size();for (const auto& value:values) std::cout<<' '<<value.object<<' '<<value.level<<' '<<value.position.x<<' '<<value.position.y;}
int main() {
    try {
        int mode;
        while (std::cin>>mode) {
            RoomDeferredState state;std::cin>>state.current_level>>state.picked_item>>state.active_item;const auto objects=count();
            for (std::size_t i=0;i<objects;++i) {
                RoomPlacementObject object;std::uint64_t square_level;Position point;
                std::cin>>object.room.identity>>object.room.level>>object.room.assigned_room>>object.monster>>object.room.position.x>>object.room.position.y>>square_level>>point.x>>point.y;
                if (square_level) object.square=RoomSquareIdentity{square_level,point};state.objects.emplace(object.room.identity,std::move(object));
            }
            for (std::uint64_t level=1;level<=2;++level) {
                auto& registry=state.placement.levels[level];registry.width=registry.height=3;entries(registry.objects);entries(registry.monsters);
                for (int y=0;y<3;++y) for (int x=0;x<3;++x) entries(state.placement.squares[{level,{x,y}}]);
            }
            for (auto& item:state.belt) std::cin>>item;for (auto& item:state.bag) std::cin>>item;
            const auto pending=count();for (std::size_t i=0;i<pending;++i) {RoomTeleport entry;std::cin>>entry.object>>entry.level>>entry.position.x>>entry.position.y;queue_room_teleport(state,entry);}
            const auto removals=count();for (std::size_t i=0;i<removals;++i) {std::uint64_t object;std::cin>>object;remove_room_teleports(state,object);}
            const auto destroyed=count();for (std::size_t i=0;i<destroyed;++i) {std::uint64_t object;std::cin>>object;queue_room_destruction(state,object);}
            if (!std::cin) throw std::runtime_error("truncated deferred fixture");
            const auto result=mode==2?RoomDeferredResult{}:mode==1?clear_room_destruction(state):fix_room_teleports(state);
            std::cout<<state.picked_item<<' '<<state.active_item;for (auto item:state.belt) std::cout<<' '<<item;for (auto item:state.bag) std::cout<<' '<<item;
            std::cout<<' '<<state.objects.size();for (const auto& [identity,object]:state.objects) std::cout<<' '<<identity<<' '<<object.room.level<<' '<<object.room.assigned_room<<' '<<object.room.position.x<<' '<<object.room.position.y<<' '<<object.square.has_value()<<' '<<(object.square?object.square->level:0)<<' '<<(object.square?object.square->position.x:0)<<' '<<(object.square?object.square->position.y:0)<<' '<<object.destroyed;
            for (const auto& [level,registry]:state.placement.levels) {write_entries(registry.objects);write_entries(registry.monsters);for (int y=0;y<3;++y) for (int x=0;x<3;++x) write_entries(state.placement.squares.at({level,{x,y}}));}
            write_teleports(state.teleports);write_entries(state.destruction);write_entries(result.destroyed);write_teleports(result.performed);write_entries(result.detached);std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
