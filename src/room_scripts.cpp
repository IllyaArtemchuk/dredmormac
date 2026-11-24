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
}
