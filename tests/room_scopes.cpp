#include "dredmor/room_scopes.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room scope assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomObjectIdentity player{1,10,0,"player",true,{0,0}},clock{2,10,3,"clock",false,{1,1}};
        check(matches_room_object(player,10,99,"player") && !matches_room_object(player,11,99,"player") && !matches_room_object(player,10,0,"Player"));
        check(matches_room_object(clock,10,3,"clock") && !matches_room_object(clock,10,0,"clock"));
        const auto first=find_room_object({player,clock,clock},10,3,"clock");check(first.object==2 && first.tested==2);
        DungeonLayout layout(3,3,std::vector<DungeonCell>(9));layout.at(1,1).room_id=7;
        const auto assigned=assign_room_object_id(clock,"new",&layout,{clock},true);check(assigned.object.assigned_room==7 && assigned.object.id=="new" && assigned.lookup.object==2 && !assigned.duplicate && assigned.debug_requested);
        auto other=clock;other.identity=3;other.assigned_room=7;other.id="new";
        const auto conflict=assign_room_object_id(clock,"new",&layout,{other,clock});check(conflict.duplicate==3 && conflict.lookup.tested==1);
        check(!assign_room_object_id(clock,"new",&layout,{clock,other}).duplicate); // Self matches first after its label is written.
        clock.position={3,1};check(assign_room_object_id(clock,"new",&layout,{}).object.assigned_room==-1);
        clock.level=0;const auto detached=assign_room_object_id(clock,"new",nullptr,{other},true);check(detached.object.assigned_room==-1 && !detached.lookup_performed && detached.debug_requested);
        clock.level=10;const auto unnamed=assign_room_object_id(clock,"",&layout,{other},true);check(!unnamed.lookup_performed && !unnamed.debug_requested);
        std::cout<<"room lookup level/global/local/case rules, ordered first match, tile scope, self-registration and duplicate diagnostics passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
