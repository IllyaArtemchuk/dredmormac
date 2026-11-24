#include "dredmor/room_scripts.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
RoomConditionValue room_condition_value(const RoomCondition& condition,const RoomScriptContext& context) {
    if (condition.kind==RoomConditionKind::entered) return {context.player_room==context.room && context.previous_room!=context.room,{}};
    if (condition.kind==RoomConditionKind::exited) return {context.previous_room==context.room && context.player_room!=context.room,{}};
    if (!context.find_object) throw std::invalid_argument("room condition requires object lookup");
    const auto object=context.find_object(context.room,condition.object_id);
    if (!object) return {condition.missing_value,{}};
    auto wrapped=[](int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;};
    switch (condition.kind) {
        case RoomConditionKind::object:return {};
        case RoomConditionKind::at:return {object->position.x==wrapped(context.origin.x,condition.point.x) && object->position.y==wrapped(context.origin.y,condition.point.y),{}};
        case RoomConditionKind::is_dead:case RoomConditionKind::is_alive: {
            RoomConditionValue result;result.positive=condition.kind==RoomConditionKind::is_dead?object->dead!=0:object->dead==0;
            if (!object->monster) result.error=RoomConditionError::monster_required;
            return result;
        }
        case RoomConditionKind::activation_changed:
            if (!object->activatable) return {false,RoomConditionError::activatable_required};
            return {object->activation.changed() && (std::uint32_t(condition.wanted)>=2 || object->activation.activated()==(condition.wanted!=0)),{}};
        case RoomConditionKind::activated:
            if (!object->activatable) return {false,RoomConditionError::activatable_required};
            return {unsigned(object->activation.activated())==(std::uint32_t(condition.wanted)&255U),{}};
        case RoomConditionKind::stopped:case RoomConditionKind::started:
            if (!object->clock) return {false,RoomConditionError::clock_required};
            return {unsigned(condition.kind==RoomConditionKind::stopped?object->timer.stopped():!object->timer.stopped())==(std::uint32_t(condition.wanted)&255U),{}};
        case RoomConditionKind::event:
            if (!context.event_occurred) throw std::invalid_argument("room condition requires event lookup");
            return {context.event_occurred(context.room,object->room_local_id,condition.event_name),{}};
        default:throw std::invalid_argument("invalid room condition kind");
    }
}
RoomScriptEvaluation all_room_conditions_positive(const std::vector<RoomCondition>& conditions,const RoomScriptContext& context) {
    RoomScriptEvaluation result;
    for (std::size_t i=0;i<conditions.size();++i) {
        result.evaluated.push_back(i);const auto value=room_condition_value(conditions[i],context);
        if (value.error) result.errors.emplace_back(i,*value.error);
        if (value.positive==conditions[i].negated) {result.positive=false;break;}
    }
    return result;
}
std::vector<RoomActionRequest> perform_room_action(const RoomAction& action,const RoomActionContext& context) {
    std::vector<RoomActionRequest> result;
    auto emit=[&](RoomActionRequest request) {result.push_back(request);if (context.perform) context.perform(request);};
    auto request=[&](RoomActionRequestKind kind,std::uint64_t subject=0) {RoomActionRequest value;value.kind=kind;value.subject=subject;return value;};
    auto offset=[&](Position point) {
        auto add=[](int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;};
        return Position{add(context.origin.x,point.x),add(context.origin.y,point.y)};
    };
    if (action.kind==RoomActionKind::create) {
        auto value=request(RoomActionRequestKind::create,action.record);value.origin=context.origin;emit(value);return result;
    }
    if (action.kind==RoomActionKind::ticker) {
        auto value=request(RoomActionRequestKind::ticker);value.text=action.text;value.value=1;emit(value);return result;
    }
    if (action.kind==RoomActionKind::spell) {
        emit(request(RoomActionRequestKind::player_idle,context.player));
        auto flag=request(RoomActionRequestKind::player_action_flag,context.player);flag.value=1;emit(flag);
        emit(request(RoomActionRequestKind::player_detach_effects,context.player));
        if (!action.cast_sound.empty()) {auto sound=request(RoomActionRequestKind::sound);sound.text=action.cast_sound;emit(sound);}
        auto point=[&](Position relative) {
            if (relative.x!=-1 || relative.y!=-1) return offset(relative);
            if (!context.player_position) throw std::invalid_argument("scripted spell requires player position");
            return context.player_position();
        };
        auto cast=request(RoomActionRequestKind::cast_spell,action.spell);cast.origin=point(action.spell_origin);cast.target=point(action.spell_target);cast.caster=context.player;cast.text="by a spell";emit(cast);return result;
    }
    if (action.kind!=RoomActionKind::move && action.kind!=RoomActionKind::remove && action.kind!=RoomActionKind::set_property) throw std::invalid_argument("invalid room action kind");
    if (!context.find_object) throw std::invalid_argument("room action requires object lookup");
    const auto object=context.find_object(context.room,action.object_id);if (!object) return result;
    if (action.kind==RoomActionKind::move) {
        auto value=request(RoomActionRequestKind::teleport,object->identity);value.target=offset(action.point);value.forced=true;emit(value);
    } else if (action.kind==RoomActionKind::remove) emit(request(RoomActionRequestKind::destroy,object->identity));
    else if (action.property=="passable") {
        auto value=request(RoomActionRequestKind::set_blocking,object->identity);value.value=action.value==0;emit(value);
    } else if (action.property=="started" || action.property=="stopped") {
        if (!object->clock) {emit(request(RoomActionRequestKind::clock_required,object->identity));return result;}
        const bool start=std::int32_t(action.property=="started")==action.value;
        emit(request(start?RoomActionRequestKind::clock_start:RoomActionRequestKind::clock_stop,object->identity));
    } else {
        auto value=request(RoomActionRequestKind::unknown_property,object->identity);value.text=action.property;emit(value);
    }
    return result;
}
void perform_room_actions(const std::vector<RoomAction>& actions,const RoomActionContext& context) {
    for (const auto& action:actions) (void)perform_room_action(action,context);
}
}
