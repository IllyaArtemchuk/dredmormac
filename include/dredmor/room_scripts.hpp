#pragma once
#include "dredmor/clocks.hpp"
#include "dredmor/world.hpp"
#include <functional>

namespace dredmor {
enum class RoomConditionKind {object,at,is_dead,is_alive,activation_changed,event,stopped,started,activated,entered,exited};
struct RoomCondition {
    RoomConditionKind kind=RoomConditionKind::object;
    bool negated=false,missing_value=false;
    std::string object_id,event_name;
    Position point{};
    std::int32_t wanted=2;
};
struct ScriptObjectState {
    Position position{};
    std::string room_local_id;
    bool monster=false,activatable=false,clock=false;
    std::int32_t dead=0;
    ActivationState activation;
    ClockState timer;
};
struct RoomScriptContext {
    std::int32_t room=0,player_room=0,previous_room=0;
    Position origin{};
    std::function<std::optional<ScriptObjectState>(std::int32_t,std::string_view)> find_object;
    std::function<bool(std::int32_t,std::string_view,std::string_view)> event_occurred;
};
enum class RoomConditionError {monster_required,activatable_required,clock_required};
struct RoomConditionValue {
    bool positive=false;
    std::optional<RoomConditionError> error;
};
// Raw virtual condition results; negation is applied by the script's outer
// AllConditionsPositiveIn loop. Lookup/event delivery and original error-handler
// behavior are boundaries; invalid object types produce diagnostic requests.
RoomConditionValue room_condition_value(const RoomCondition& condition,const RoomScriptContext& context);
struct RoomScriptEvaluation {
    bool positive=true;
    std::vector<std::size_t> evaluated;
    std::vector<std::pair<std::size_t,RoomConditionError>> errors;
};
RoomScriptEvaluation all_room_conditions_positive(const std::vector<RoomCondition>& conditions,const RoomScriptContext& context);
}
