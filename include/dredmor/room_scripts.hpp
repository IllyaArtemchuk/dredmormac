#pragma once
#include "dredmor/clocks.hpp"
#include "dredmor/world.hpp"
#include "dredmor/room_records.hpp"
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace dredmor {
enum class RoomConditionKind {object,at,is_dead,is_alive,activation_changed,event,stopped,started,activated,entered,exited,type_at};
struct ScriptItemType {std::uint32_t kind=0,subtype=0;};
struct ScriptTileItem {bool item=false;ScriptItemType type;};
struct RoomCondition {
    RoomConditionKind kind=RoomConditionKind::object;
    bool negated=false,missing_value=false;
    std::string object_id,event_name;
    Position point{};
    std::int32_t wanted=2;
    std::string item_type,item_subtype;
    mutable std::optional<ScriptItemType> cached_item;
};
RoomCondition read_room_condition(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room);
struct ScriptObjectState {
    Position position{};
    std::string room_local_id;
    bool monster=false,activatable=false,clock=false;
    std::int32_t dead=0;
    ActivationState activation;
    ClockState timer;
    std::uint64_t identity=0;
};
struct RoomScriptContext {
    std::int32_t room=0,player_room=0,previous_room=0;
    Position origin{};
    std::function<std::optional<ScriptObjectState>(std::int32_t,std::string_view)> find_object;
    std::function<bool(std::int32_t,std::string_view,std::string_view)> event_occurred;
    // Original prototype factory receives quantity1 and artifact=false. It is
    // called even when the target tile is empty, and only until a cache exists.
    std::function<std::optional<ScriptItemType>(std::string_view,std::string_view)> create_item;
    std::function<std::vector<ScriptTileItem>(Position)> tile_items;
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
enum class RoomActionKind {create,move,remove,ticker,set_property,spell};
struct RoomAction {
    RoomActionKind kind=RoomActionKind::ticker;
    std::string object_id,text,property,cast_sound;
    std::int32_t value=0;
    std::uint64_t record=0,spell=0;
    Position point{},spell_origin{-1,-1},spell_target{-1,-1};
};
struct ScriptSpellBinding {std::uint64_t identity=0;std::string cast_sound;};
struct RoomActionReaders {
    std::function<ScriptSpellBinding(std::string_view)> spell;
    // Receives a canonical record node after action_type/create_type removal.
    // The record reader owns its remaining schema and point/template effects.
    std::function<std::uint64_t(const XmlNode&,const RoomMarkers&,RoomStamp&)> record;
};
// The owning script consumes action routing ('on') before calling this reader.
RoomAction read_room_action(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room,const RoomActionReaders& readers={});
enum class RoomActionRequestKind {create,teleport,destroy,ticker,set_blocking,clock_start,clock_stop,player_idle,player_action_flag,player_detach_effects,sound,cast_spell,unknown_property,clock_required};
struct RoomActionRequest {
    RoomActionRequestKind kind;
    std::uint64_t subject=0,caster=0;
    Position origin{},target{};
    std::int32_t value=0;
    bool forced=false;
    std::string text;
};
struct RoomActionContext {
    std::int32_t room=0;
    Position origin{};
    std::uint64_t player=1;
    std::function<std::optional<ScriptObjectState>(std::int32_t,std::string_view)> find_object;
    std::function<Position()> player_position;
    // Requests execute immediately in original order. A callback can mutate
    // live state used by later actions; no script-wide rollback is imposed.
    std::function<void(const RoomActionRequest&)> perform;
};
// Constructors, teleport, destruction, ticker, spell execution and actor
// presentation are boundaries. A missing perform callback returns a plan only.
// Invalid clock types request a diagnostic and stop that action; the original
// roomError's fatal behavior is not emulated by calling an unrelated vtable.
std::vector<RoomActionRequest> perform_room_action(const RoomAction& action,const RoomActionContext& context);
void perform_room_actions(const std::vector<RoomAction>& actions,const RoomActionContext& context);
struct RoomScriptDefinition {
    std::int32_t success_repeat=1;
    std::vector<RoomCondition> conditions;
    std::vector<RoomAction> success,failure;
};
using RoomEventKey=std::pair<std::int32_t,std::string>;
using RoomEventQueue=std::map<RoomEventKey,std::set<std::string>>;
void set_room_event(RoomEventQueue& events,std::int32_t assigned_room,std::string_view object_id,std::string_view event);
bool room_event_is_set(const RoomEventQueue& events,std::int32_t room,std::string_view object_id,std::string_view event);
using RoomScriptKey=std::pair<std::int32_t,std::int32_t>;
struct RoomTriggerState {
    std::map<RoomScriptKey,std::int32_t> success_counts;
    std::int32_t previous_room=0;
    RoomEventQueue events;
};
struct RoomTriggerRoom {
    bool present=true;
    // DirectorFillRoom sets RoomInfo +0x10 when content has been filled. A null
    // scripts pointer represents the original missing room template.
    std::int32_t content_filled=1,identity=0;
    std::shared_ptr<const std::vector<RoomScriptDefinition>> scripts;
};
struct RoomTriggerCallbacks {
    std::function<bool(std::size_t,std::size_t)> conditions_positive;
    std::function<void(std::size_t,std::size_t)> success_actions;
    std::function<std::int32_t()> player_room;
};
struct RoomTriggerPass {
    std::vector<RoomScriptKey> checked,conditions,successes;
    std::vector<std::size_t> invalid_room_ids;
};
bool wants_room_script_success(RoomTriggerState& state,RoomScriptKey key,std::int32_t repeat);
// Original level pass. Only success actions run here. Callbacks see live counts,
// events and the prior room identity; at the end player_room is resolved after
// all actions, then every queued event is cleared. Debug logging materializes
// otherwise absent counter entries, even for unlimited/failed scripts.
RoomTriggerPass run_room_trigger_logic(const std::vector<RoomTriggerRoom>& rooms,RoomTriggerState& state,
    const RoomTriggerCallbacks& callbacks,bool debug=false);
}
