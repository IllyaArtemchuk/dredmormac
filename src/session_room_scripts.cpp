#include "dredmor/session_room_scripts.hpp"
#include "dredmor/session.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace dredmor {
SessionRoomObjectRef SessionRoomScriptExecutor::reference(const SessionRoomObjects& objects,std::uint64_t identity) {
    if (const auto ref=objects.reference(identity)) return *ref;
    throw std::runtime_error("script action refers to an unregistered object");
}
CustomRoomObject& SessionRoomScriptExecutor::custom(Session& session,SessionRoomObjectRef ref) {
    switch (ref.kind) {
        case SessionRoomObjectKind::custom:return session.custom_objects_.at(ref.subject-1);
        case SessionRoomObjectKind::breakable:return session.breakables_.at(ref.subject-1).object;
        case SessionRoomObjectKind::lever:return session.script_levers_.at(ref.subject-1).object;
        default:throw std::runtime_error("script blocking flag is not yet supported for this Session object");
    }
}
SessionRoomScriptPass SessionRoomScriptExecutor::run(Session& session,const SessionRoomScriptProfile& profile,
    SessionRoomObjects& objects,RoomRuntimeState& runtime,const SessionRoomScriptHooks& hooks) {
    auto candidate=session;auto bindings=objects;auto state=runtime;
    auto pending=evaluate(candidate,profile,bindings,state,hooks);
    clear_destruction(candidate,bindings,pending);
    while (pending.next_move<pending.moves.size()) place_next(candidate,bindings,pending);
    auto result=finish(candidate,bindings,pending);
    session=std::move(candidate);objects=std::move(bindings);runtime=std::move(state);return result;
}
SessionRoomScriptQueue SessionRoomScriptExecutor::evaluate(Session& candidate,const SessionRoomScriptProfile& profile,
    SessionRoomObjects& bindings,RoomRuntimeState& state,const SessionRoomScriptHooks& hooks) {
    if (candidate.script_phase_pending_) throw std::invalid_argument("room script phase is already active");
    if (profile.layout.width()!=candidate.world_.width() || profile.layout.height()!=candidate.world_.height() || profile.rooms.size()!=profile.programs.size())
        throw std::invalid_argument("room script profile does not match Session");
    for (std::size_t i=0;i<profile.rooms.size();++i) {
        const auto& room=profile.rooms[i];const auto& program=profile.programs[i];
        if (bool(room.trigger.scripts)!=bool(program) || (program && room.trigger.scripts.get()!=&program->scripts))
            throw std::invalid_argument("room script profile has mismatched program");
    }
    if (!bindings.player()) throw std::invalid_argument("room scripts require a registered player");
    if (!bindings.unplaced().empty()) throw std::invalid_argument("finish reserved initial room objects before a scripted turn");
    if (candidate.queued_horde_members()) throw std::invalid_argument("finish initial horde placement before a scripted turn");
    candidate.script_phase_pending_=true;
    SessionRoomScriptQueue pending;pending.level=bindings.level();pending.stage=SessionRoomScriptStage::evaluated;
    auto& result=pending.pass;auto& destruction=pending.destruction;auto& teleports=pending.moves;
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
                if (const auto* horde=std::get_if<RoomHordeRecord>(&record)) {
                    if (!hooks.queue_horde) throw std::runtime_error("script horde creation requires a constructor boundary");
                    const auto registered=candidate.enemies_.size();
                    hooks.queue_horde(candidate,*horde,request.origin);
                    if (candidate.enemies_.size()!=registered) throw std::runtime_error("script horde constructor must leave placements queued");
                    for (auto& group:candidate.pending_hordes_) for (std::size_t i=0;i<group.actors.size();++i) {
                        // The source actors have been constructed but have no
                        // level registration until their queued placements.
                        // Negative port coordinates keep these actors out of
                        // subsequent occupancy searches during this pass.
                        auto& actor=group.actors[i];actor.position={-1,-1};const auto actor_id=actor.id;
                        candidate.enemies_.push_back(std::move(actor));
                        const auto object_id=bindings.reserve(candidate,{SessionRoomObjectKind::monster,actor_id});
                        teleports.push_back({object_id,group.teleports[i].position,true});
                    }
                    candidate.pending_hordes_.clear();break;
                }
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
                teleports.push_back({request.subject,request.target,false});break;
            case RoomActionRequestKind::set_blocking:custom(candidate,reference(bindings,request.subject)).blocks_movement=request.value!=0;break;
            case RoomActionRequestKind::cast_spell: {
                if (!request.subject || reference(bindings,request.caster).kind!=SessionRoomObjectKind::player) throw std::runtime_error("invalid scripted spell caster or handle");
                const auto spell=candidate.spell_definition(request.subject-1);
                candidate.cast_spell(spell,request.origin,request.target,candidate.player_.id);break;
            }
            case RoomActionRequestKind::clock_start:case RoomActionRequestKind::clock_stop:
            case RoomActionRequestKind::clock_required:case RoomActionRequestKind::unknown_property:
                throw std::runtime_error("room script clock/property request is not yet supported by Session");
        }
    };
    result.triggers=run_room_scripts(profile.rooms,state,live);
    return pending;
}
void SessionRoomScriptExecutor::clear_destruction(Session& candidate,SessionRoomObjects& bindings,SessionRoomScriptQueue& pending) {
    if (!candidate.script_phase_pending_ || pending.stage!=SessionRoomScriptStage::evaluated || pending.level!=bindings.level())
        throw std::invalid_argument("room script destruction requires its evaluated candidate");
    // Pending removals deliberately remained available to every action and
    // condition in this pass. Delete now, preserving other opaque identities.
    auto erase_id=[](auto& values,std::uint64_t id,auto identity) {
        const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return identity(value)==id;});
        if (found==values.end()) throw std::runtime_error("destroyed script subject has disappeared");
        values.erase(found);
    };
    for (auto identity:pending.destruction) {
        const auto ref=reference(bindings,identity);
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
                candidate.unlink_horde_member(ref.subject);
                erase_id(candidate.enemies_,ref.subject,[](const auto& value){return value.id;});break;
            }
            case SessionRoomObjectKind::keeper:case SessionRoomObjectKind::player:throw std::runtime_error("script player/keeper destruction is not supported");
        }
        bindings.erase_subject(ref);
        // Source derived/base destruction removes queued teleports targeting
        // this object before the later forced-placement pass.
        pending.moves.erase(std::remove_if(pending.moves.begin(),pending.moves.end(),[&](const auto& value){return value.subject==identity;}),pending.moves.end());
    }
    pending.destruction.clear();pending.stage=SessionRoomScriptStage::cleared;
}
void SessionRoomScriptExecutor::place_next(Session& candidate,SessionRoomObjects& bindings,SessionRoomScriptQueue& pending) {
    if (!candidate.script_phase_pending_ || pending.stage!=SessionRoomScriptStage::cleared || pending.level!=bindings.level() || pending.next_move>=pending.moves.size())
        throw std::invalid_argument("room script placement requires its cleared candidate");
    const auto move=pending.moves[pending.next_move];
    const auto identity=move.subject;const auto point=move.point;
    if (point.x<0 || point.y<0 || point.x>=candidate.world_.width() || point.y>=candidate.world_.height())
        throw std::runtime_error("Session cannot yet persist out-of-grid script teleports");
    if (move.reserved) {
        const auto ref=reference(bindings,identity);
        if (ref.kind!=SessionRoomObjectKind::monster) throw std::runtime_error("reserved horde placement requires monster identity");
        candidate.actor_by_id(ref.subject).position=point;
        bindings.place_reserved(candidate,identity);
    } else {
        const auto ref=reference(bindings,identity);
        switch (ref.kind) {
            case SessionRoomObjectKind::player:
                candidate.world_.place_player(point,candidate.world_.turn());candidate.player_.position=point;pending.pass.player_teleported=true;break;
            case SessionRoomObjectKind::monster:
                if (!candidate.world_.passable(point) && !candidate.actor_by_id(ref.subject).horde) throw std::runtime_error("Session cannot yet persist ordinary monsters teleported onto walls");
                candidate.actor_by_id(ref.subject).position=point;break;
            case SessionRoomObjectKind::item:
                if (!candidate.world_.passable(point)) throw std::runtime_error("Session cannot yet persist items teleported onto walls");
                for (auto& value:candidate.ground_) if (value.item.instance==ref.subject) value.position=point;
                break;
            case SessionRoomObjectKind::custom:case SessionRoomObjectKind::breakable:case SessionRoomObjectKind::lever:custom(candidate,ref).position=point;break;
            case SessionRoomObjectKind::trap:throw std::runtime_error("script trap teleport requires origin/effect lifetime integration");
            case SessionRoomObjectKind::keeper:throw std::runtime_error("script keeper teleport requires shop guard integration");
        }
    }
    ++pending.next_move;
}
SessionRoomScriptPass SessionRoomScriptExecutor::finish(Session& candidate,const SessionRoomObjects& bindings,SessionRoomScriptQueue& pending) {
    if (!candidate.script_phase_pending_ || pending.stage!=SessionRoomScriptStage::cleared || pending.level!=bindings.level() || pending.next_move!=pending.moves.size())
        throw std::invalid_argument("room script finish requires all deferred placements");
    if (!bindings.unplaced().empty()) throw std::runtime_error("scripted turn left unplaced room objects");
    candidate.validate_hordes();
    candidate.script_phase_pending_=false;pending.stage=SessionRoomScriptStage::finished;pending.moves.clear();pending.next_move=0;
    return std::move(pending.pass);
}
}
