#include "dredmor/room_scopes.hpp"
#include <stdexcept>

namespace dredmor {
bool matches_room_object(const RoomObjectIdentity& object,std::uint64_t level,std::int32_t room,std::string_view id) {
    return object.level==level && (object.global || object.assigned_room==room) && std::string_view(object.id)==id;
}
RoomObjectLookup find_room_object(const std::vector<RoomObjectIdentity>& registered,std::uint64_t level,std::int32_t room,std::string_view id) {
    RoomObjectLookup result;
    for (const auto& object:registered) {
        ++result.tested;
        if (matches_room_object(object,level,room,id)) {result.object=object.identity;break;}
    }
    return result;
}
RoomIdAssignment assign_room_object_id(RoomObjectIdentity object,std::string id,const DungeonLayout* owner,
    const std::vector<RoomObjectIdentity>& registered,bool debug) {
    RoomIdAssignment result;object.id=std::move(id);object.assigned_room=-1;
    if (object.level) {
        if (!owner) throw std::invalid_argument("room ID assignment requires owning level grid");
        const auto p=object.position;
        if (p.x>=0 && p.y>=0 && p.x<owner->width() && p.y<owner->height()) object.assigned_room=owner->at(p.x,p.y).room_id;
        if (!object.id.empty()) {
            result.lookup_performed=true;
            for (const auto& entry:registered) {
                const auto& candidate=entry.identity==object.identity?object:entry;++result.lookup.tested;
                if (matches_room_object(candidate,object.level,object.assigned_room,object.id)) {result.lookup.object=candidate.identity;break;}
            }
            if (result.lookup.object && *result.lookup.object!=object.identity) result.duplicate=result.lookup.object;
        }
    }
    result.debug_requested=debug && !object.id.empty();result.object=std::move(object);return result;
}
}
