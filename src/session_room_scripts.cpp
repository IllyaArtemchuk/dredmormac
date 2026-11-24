#include "dredmor/session_room_scripts.hpp"
#include "dredmor/session.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace dredmor {
SessionRoomScriptPass SessionRoomScriptExecutor::run(Session& session,const SessionRoomScriptProfile& profile,
    SessionRoomObjects& objects,RoomRuntimeState& runtime,const SessionRoomScriptHooks& hooks) {
    if (profile.layout.width()!=session.world_.width() || profile.layout.height()!=session.world_.height() || profile.rooms.size()!=profile.programs.size())
        throw std::invalid_argument("room script profile does not match Session");
    for (std::size_t i=0;i<profile.rooms.size();++i) {
        const auto& room=profile.rooms[i];const auto& program=profile.programs[i];
        if (bool(room.trigger.scripts)!=bool(program) || (program && room.trigger.scripts.get()!=&program->scripts))
            throw std::invalid_argument("room script profile has mismatched program");
    }
    if (!objects.player()) throw std::invalid_argument("room scripts require a registered player");
    auto candidate=session;auto bindings=objects;auto state=runtime;
    SessionRoomScriptPass result;
    std::vector<std::uint64_t> destruction;
    std::vector<std::pair<std::uint64_t,Position>> teleports;
    auto reference=[&](std::uint64_t identity) {
        for (const auto& binding:bindings.bindings()) if (binding.object.identity==identity) return binding.reference;
        throw std::runtime_error("script action refers to an unregistered object");
    };
    auto custom=[&](SessionRoomObjectRef ref)->CustomRoomObject& {
        switch (ref.kind) {
            case SessionRoomObjectKind::custom:return candidate.custom_objects_.at(ref.subject-1);
            case SessionRoomObjectKind::breakable:return candidate.breakables_.at(ref.subject-1).object;
            case SessionRoomObjectKind::lever:return candidate.script_levers_.at(ref.subject-1).object;
            default:throw std::runtime_error("script blocking flag is not yet supported for this Session object");
        }
    };
    RoomRuntimeHooks live;live.player=*bindings.player();
    live.player_position=[&] {return candidate.player_.position;};
    live.player_room=[&] {
        const auto p=candidate.player_.position;
        if (p.x<0 || p.y<0 || p.x>=profile.layout.width() || p.y>=profile.layout.height()) return std::int32_t(-1);
        const auto room=profile.layout.at(p.x,p.y).room_id;
        return std::size_t(room)<profile.rooms.size()?profile.rooms[room].trigger.identity:std::int32_t(-1);
    };
    live.find_object=[&](auto room,auto id) {return bindings.find(candidate,room,id);};
    if (hooks.create_prototype) live.create_item=[&](auto type,auto subtype) {return hooks.create_prototype(candidate,type,subtype);};
    if (hooks.tile_items) live.tile_items=[&](Position p) {return hooks.tile_items(candidate,p);};
    live.perform=[&](const RoomActionRequest& request) {
        switch (request.kind) {
            case RoomActionRequestKind::ticker:case RoomActionRequestKind::sound:
                result.presentation.push_back(request);break;
            case RoomActionRequestKind::player_idle:case RoomActionRequestKind::player_action_flag:case RoomActionRequestKind::player_detach_effects:
                if (!hooks.prepare_spell_actor) throw std::runtime_error("scripted spell actor/effect setup is not yet integrated");
                hooks.prepare_spell_actor(candidate,request);break;
            case RoomActionRequestKind::create: {
                if (request.room<0 || std::size_t(request.room)>=profile.programs.size() || !request.subject)
                    throw std::runtime_error("invalid script Create handle");
                const auto& record=profile.programs.at(request.room)->created_records.at(request.subject-1);
                if (std::holds_alternative<RoomHordeRecord>(record)) throw std::runtime_error("script horde creation requires merged deferred placement queues");
                auto created=std::visit([&](const auto& definition)->std::optional<SessionRoomObjectRef> {
                    using T=std::decay_t<decltype(definition)>;
                    if constexpr (std::is_same_v<T,CustomRoomDefinition>) {
                        if (const auto index=candidate.create_custom_object(definition,request.origin)) return SessionRoomObjectRef{SessionRoomObjectKind::custom,*index+1};
                    } else if constexpr (std::is_same_v<T,RoomPedestalRecord>) {
                        if (const auto index=candidate.create_pedestal(definition,request.origin)) return SessionRoomObjectRef{SessionRoomObjectKind::custom,*index+1};
                    } else if constexpr (std::is_same_v<T,BreakableRoomDefinition>) {
                        if (!hooks.breakable_frames) throw std::runtime_error("script breakable requires resource frame counts");
                        if (const auto index=candidate.create_breakable(definition,request.origin,hooks.breakable_frames)) return SessionRoomObjectRef{SessionRoomObjectKind::breakable,*index+1};
                    } else if constexpr (std::is_same_v<T,RoomElementDefinition>) {
                        if (definition.type!="lever") {
                            if (!hooks.create_record) throw std::runtime_error("script element constructor is not yet supported");
                            return hooks.create_record(candidate,record,request.origin);
                        }
                        if (const auto index=candidate.create_script_lever(definition,request.origin)) return SessionRoomObjectRef{SessionRoomObjectKind::lever,*index+1};
                    } else {
                        if (!hooks.create_record) throw std::runtime_error("script record constructor is not yet supported");
                        return hooks.create_record(candidate,record,request.origin);
                    }
                    return std::nullopt;
                },record);
                if (created) {
                    const auto assignment=bindings.bind(candidate,*created,profile.layout,!std::holds_alternative<RoomHordeRecord>(record));
                    if (assignment.duplicate) throw std::runtime_error("script Create duplicates a room object label");
                }
                break;
            }
            case RoomActionRequestKind::destroy:
                if (std::find(destruction.begin(),destruction.end(),request.subject)==destruction.end()) destruction.push_back(request.subject);
                break;
            case RoomActionRequestKind::teleport:
                if (!request.forced) throw std::runtime_error("room script teleport requires forced placement");
                teleports.emplace_back(request.subject,request.target);break;
            case RoomActionRequestKind::set_blocking:custom(reference(request.subject)).blocks_movement=request.value!=0;break;
            case RoomActionRequestKind::cast_spell: {
                if (!request.subject || reference(request.caster).kind!=SessionRoomObjectKind::player) throw std::runtime_error("invalid scripted spell caster or handle");
                const auto spell=candidate.spell_definition(request.subject-1);
                candidate.cast_spell(spell,request.origin,request.target,candidate.player_.id);break;
            }
            case RoomActionRequestKind::clock_start:case RoomActionRequestKind::clock_stop:
            case RoomActionRequestKind::clock_required:case RoomActionRequestKind::unknown_property:
                throw std::runtime_error("room script clock/property request is not yet supported by Session");
        }
    };
    result.triggers=run_room_scripts(profile.rooms,state,live);
    // Pending removals deliberately remained available to every action and
    // condition in this pass. Delete now, preserving other opaque identities.
    auto erase_id=[](auto& values,std::uint64_t id,auto identity) {
        const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return identity(value)==id;});
        if (found==values.end()) throw std::runtime_error("destroyed script subject has disappeared");
        values.erase(found);
    };
    for (auto identity:destruction) {
        const auto ref=reference(identity);
        switch (ref.kind) {
            case SessionRoomObjectKind::custom:candidate.custom_objects_.erase(candidate.custom_objects_.begin()+ref.subject-1);break;
            case SessionRoomObjectKind::breakable:
                candidate.breakables_.erase(candidate.breakables_.begin()+ref.subject-1);
                // Integration presentation events use one-based typed indices.
                // A destroyed breakable cannot retain an animation request for
                // the different object that shifts into its old index.
                candidate.events_.erase(std::remove_if(candidate.events_.begin(),candidate.events_.end(),[&](auto& event) {
                    if (event.kind!=EventKind::object_hit && event.kind!=EventKind::object_broken) return false;
                    if (event.target==ref.subject) return true;
                    if (event.target>ref.subject) --event.target;
                    return false;
                }),candidate.events_.end());
                break;
            case SessionRoomObjectKind::lever:candidate.script_levers_.erase(candidate.script_levers_.begin()+ref.subject-1);break;
            case SessionRoomObjectKind::item:erase_id(candidate.ground_,ref.subject,[](const auto& value){return value.item.instance;});break;
            case SessionRoomObjectKind::trap:throw std::runtime_error("script trap destruction requires origin/effect lifetime integration");
            case SessionRoomObjectKind::monster: {
                for (const auto& actor:candidate.enemies_) if (actor.id==ref.subject && actor.horde) throw std::runtime_error("script horde destruction requires group lifetime integration");
                erase_id(candidate.enemies_,ref.subject,[](const auto& value){return value.id;});break;
            }
            case SessionRoomObjectKind::keeper:case SessionRoomObjectKind::player:throw std::runtime_error("script player/keeper destruction is not supported");
        }
        bindings.erase_subject(ref);
        // Source derived/base destruction removes queued teleports targeting
        // this object before the later forced-placement pass.
        teleports.erase(std::remove_if(teleports.begin(),teleports.end(),[&](const auto& value){return value.first==identity;}),teleports.end());
    }
    for (const auto& [identity,point]:teleports) {
        if (point.x<0 || point.y<0 || point.x>=candidate.world_.width() || point.y>=candidate.world_.height())
            throw std::runtime_error("Session cannot yet persist out-of-grid script teleports");
        const auto ref=reference(identity);
        switch (ref.kind) {
            case SessionRoomObjectKind::player:
                candidate.world_.place_player(point,candidate.world_.turn());candidate.player_.position=point;result.player_teleported=true;break;
            case SessionRoomObjectKind::monster:
                if (!candidate.world_.passable(point) && !candidate.actor_by_id(ref.subject).horde) throw std::runtime_error("Session cannot yet persist ordinary monsters teleported onto walls");
                candidate.actor_by_id(ref.subject).position=point;break;
            case SessionRoomObjectKind::item:
                if (!candidate.world_.passable(point)) throw std::runtime_error("Session cannot yet persist items teleported onto walls");
                for (auto& value:candidate.ground_) if (value.item.instance==ref.subject) value.position=point;
                break;
            case SessionRoomObjectKind::custom:case SessionRoomObjectKind::breakable:case SessionRoomObjectKind::lever:custom(ref).position=point;break;
            case SessionRoomObjectKind::trap:throw std::runtime_error("script trap teleport requires origin/effect lifetime integration");
            case SessionRoomObjectKind::keeper:throw std::runtime_error("script keeper teleport requires shop guard integration");
        }
    }
    session=std::move(candidate);objects=std::move(bindings);runtime=std::move(state);return result;
}
}
