#include "dredmor/room_scripts.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("script condition assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomScriptContext context;context.room=2;context.player_room=2;context.previous_room=1;
        RoomCondition entered;entered.kind=RoomConditionKind::entered;check(room_condition_value(entered,context).positive);
        context.previous_room=2;check(!room_condition_value(entered,context).positive);context.player_room=3;entered.kind=RoomConditionKind::exited;check(room_condition_value(entered,context).positive);
        ScriptObjectState object;object.activatable=true;object.activation.activate(true);object.room_local_id="lever";int lookups=0,events=0;
        context.find_object=[&](auto,std::string_view)->std::optional<ScriptObjectState>{++lookups;return object;};
        context.event_occurred=[&](auto,std::string_view name,std::string_view event){++events;check(name=="lever" && event=="expired");return true;};
        RoomCondition changed;changed.kind=RoomConditionKind::activation_changed;changed.wanted=1;check(room_condition_value(changed,context).positive);
        object.activation.advance_history();check(room_condition_value(changed,context).positive);object.activation.advance_history();check(!room_condition_value(changed,context).positive);
        RoomCondition event;event.kind=RoomConditionKind::event;event.event_name="expired";RoomCondition absent;absent.kind=RoomConditionKind::object;
        lookups=0;auto evaluation=all_room_conditions_positive({event,absent,event},context);check(!evaluation.positive && evaluation.evaluated.size()==2 && events==1 && lookups==2);
        absent.negated=true;check(all_room_conditions_positive({event,absent,event},context).positive);
        check(all_room_conditions_positive({},{}).positive);
        object.position={-2147483647-1,4};context.origin={2147483647,3};RoomCondition at;at.kind=RoomConditionKind::at;at.point={1,1};check(room_condition_value(at,context).positive);
        object.clock=true;object.timer.step=0;RoomCondition stopped;stopped.kind=RoomConditionKind::stopped;stopped.wanted=1;check(room_condition_value(stopped,context).positive);
        object.clock=false;check(room_condition_value(stopped,context).error==RoomConditionError::clock_required);
        context.find_object=[](auto,std::string_view)->std::optional<ScriptObjectState>{return std::nullopt;};stopped.missing_value=true;check(room_condition_value(stopped,context).positive && !room_condition_value(stopped,context).error);
        RoomCondition type_at;type_at.kind=RoomConditionKind::type_at;type_at.item_type="weapon";type_at.item_subtype="random";type_at.point={1,1};int created=0;
        context.create_item=[&](auto,auto)->std::optional<ScriptItemType>{++created;return ScriptItemType{1,3};};
        context.tile_items=[](Position p) {check(p.x==(-2147483647-1) && p.y==4);return std::vector<ScriptTileItem>{{false,{1,3}},{true,{1,5}}};};
        check(room_condition_value(type_at,context).positive && created==1);check(room_condition_value(type_at,context).positive && created==1);
        type_at.item_subtype="test weapon";check(!room_condition_value(type_at,context).positive && created==1);
        std::cout<<"script entry/exit, live object/event lookups, two-update activation, short circuit/negation, wrapped coordinates and missing objects passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
