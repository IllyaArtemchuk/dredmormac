#include "dredmor/room_deferred.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room deferred assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
RoomDeferredState fixture() {
    RoomDeferredState state;state.current_level=1;state.placement.levels[1]={3,3,{1,2},{1,2}};state.placement.levels[2]={3,3,{},{}};
    state.objects[1]={RoomObjectIdentity{1,1,7,"first",false,{1,1}},true,RoomSquareIdentity{1,{1,1}}};
    state.objects[2]={RoomObjectIdentity{2,1,8,"second",false,{2,1}},true,RoomSquareIdentity{1,{2,1}}};
    state.placement.squares[{1,{1,1}}]={1};state.placement.squares[{1,{2,1}}]={2};return state;
}
int main() {
    try {
        auto state=fixture();state.belt[0]=state.belt[8]=state.bag[47]=1;state.picked_item=state.active_item=1;
        queue_room_teleport(state,{1,2,{0,0}});queue_room_teleport(state,{2,2,{1,1}});queue_room_teleport(state,{1,2,{2,2}});
        queue_room_destruction(state,1);queue_room_destruction(state,1);check(!state.picked_item && state.active_item==1 && state.destruction.size()==1);
        const auto result=fix_room_teleports(state);check(result.destroyed==std::vector<std::uint64_t>{1} && result.performed.size()==1 && result.performed[0].object==2);
        check(state.objects.at(1).destroyed && !state.objects.at(1).room.level && !state.objects.at(1).square && state.objects.at(2).room.level==2);
        check(!state.belt[0] && !state.belt[8] && !state.bag[47] && !state.active_item && state.teleports.empty() && state.destruction.empty());
        state=fixture();queue_room_teleport(state,{1,0,{-1,-1}});const auto detached=fix_room_teleports(state);
        check(detached.detached==std::vector<std::uint64_t>{1} && !state.objects.at(1).room.level && !state.objects.at(1).square && state.objects.at(1).room.position.x==1);
        state=fixture();state.objects.at(1).room.position.x=-1;queue_room_teleport(state,{1,0,{-1,0}});fix_room_teleports(state);check(state.objects.at(1).room.level==1 && state.objects.at(1).square);
        state=fixture();queue_room_teleport(state,{1,2,{0,0}});queue_room_teleport(state,{1,2,{1,1}});remove_room_teleports(state,1);check(state.teleports.empty());
        state=fixture();queue_room_destruction(state,1);RoomDeferredHooks hooks;
        hooks.destroy=[&](auto object) {destroy_room_object_base(state,object);if (object==1) queue_room_destruction(state,2);};
        check(clear_room_destruction(state,hooks).destroyed==std::vector<std::uint64_t>({1,2}) && state.objects.at(2).destroyed);
        state=fixture();queue_room_teleport(state,{1,2,{0,0}});hooks={};hooks.teleport=[&](const RoomTeleport& entry,bool forced) {check(forced);teleport_room_object(state.objects.at(entry.object),state.placement,entry.position,entry.level);if (entry.object==1) queue_room_teleport(state,{2,2,{1,1}});};
        check(fix_room_teleports(state,hooks).performed.size()==2 && state.objects.at(2).room.level==2);
        std::cout<<"deferred deletion precedes movement, removes duplicate queued requests/inventory references, preserves detach sentinels and visits callback-appended work\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
