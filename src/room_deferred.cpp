#include "dredmor/room_deferred.hpp"
#include <algorithm>
#include <stdexcept>

namespace dredmor {
namespace {
template<class T,class Predicate> bool erase_first(std::vector<T>& values,Predicate predicate) {
    const auto found=std::find_if(values.begin(),values.end(),predicate);if (found==values.end()) return false;
    values.erase(found);return true;
}
void remove_square(RoomDeferredState& state,RoomPlacementObject& object,bool force_clear) {
    bool removed=false;
    if (object.square) {
        const auto found=state.placement.squares.find(*object.square);
        if (found!=state.placement.squares.end()) removed=erase_first(found->second,[&](auto id) {return id==object.room.identity;});
    }
    if (removed || force_clear) object.square.reset();
}
RoomPlacementObject& live_object(RoomDeferredState& state,std::uint64_t identity) {
    const auto found=state.objects.find(identity);
    if (!identity || found==state.objects.end() || found->second.destroyed) throw std::invalid_argument("deferred operation requires a live object");
    return found->second;
}
}
void queue_room_teleport(RoomDeferredState& state,RoomTeleport teleport) {state.teleports.push_back(teleport);}
void remove_room_teleports(RoomDeferredState& state,std::uint64_t object) {
    state.teleports.erase(std::remove_if(state.teleports.begin(),state.teleports.end(),[&](const auto& entry) {return entry.object==object;}),state.teleports.end());
}
void queue_room_destruction(RoomDeferredState& state,std::uint64_t object) {
    live_object(state,object);
    if (state.picked_item==object) state.picked_item=0;
    if (std::find(state.destruction.begin(),state.destruction.end(),object)==state.destruction.end()) state.destruction.push_back(object);
}
void destroy_room_object_base(RoomDeferredState& state,std::uint64_t identity) {
    auto& object=live_object(state,identity);remove_room_teleports(state,identity);remove_square(state,object,true);
    if (object.room.level) {
        // The base destructor only erases the first main-registry entry. It
        // does not use Level::RemoveObject or touch the monster vector here.
        unregister_room_object(state.placement.levels.at(object.room.level),identity,false);object.room.level=0;
    }
    object.destroyed=true;
}
RoomDeferredResult clear_room_destruction(RoomDeferredState& state,const RoomDeferredHooks& hooks) {
    RoomDeferredResult result;
    for (std::size_t i=0;i<state.destruction.size();++i) {
        const auto identity=state.destruction[i];auto& object=live_object(state,identity);
        erase_first(state.teleports,[&](const auto& entry) {return entry.object==identity;});
        if (object.square) remove_square(state,object,false);
        if (state.current_level) unregister_room_object(state.placement.levels.at(state.current_level),identity,object.monster);
        for (auto& item:state.belt) if (item==identity) item=0;
        for (auto& item:state.bag) if (item==identity) item=0;
        if (state.active_item==identity) state.active_item=0;
        if (state.picked_item==identity) state.picked_item=0;
        result.destroyed.push_back(identity);
        if (hooks.destroy) hooks.destroy(identity);else destroy_room_object_base(state,identity);
    }
    state.destruction.clear();return result;
}
RoomDeferredResult fix_room_teleports(RoomDeferredState& state,const RoomDeferredHooks& hooks) {
    auto result=clear_room_destruction(state,hooks);
    for (std::size_t i=0;i<state.teleports.size();++i) {
        const auto entry=state.teleports[i];if (!entry.object) continue;auto& object=live_object(state,entry.object);
        if (entry.level) {
            result.performed.push_back(entry);
            if (hooks.teleport) hooks.teleport(entry,true);else teleport_room_object(object,state.placement,entry.position,entry.level);
        } else {
            if ((entry.position.x==-1 || entry.position.y==-1) && (object.room.position.x==-1 || object.room.position.y==-1)) continue;
            if (!object.square || !object.room.level) throw std::invalid_argument("null-level teleport detachment requires attached owning level");
            remove_square(state,object,true);
            unregister_room_object(state.placement.levels.at(object.room.level),entry.object,object.monster);object.room.level=0;
            result.detached.push_back(entry.object);
        }
    }
    state.teleports.clear();return result;
}
}
