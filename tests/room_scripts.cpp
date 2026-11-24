#include "dredmor/room_scripts.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("script condition assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomStamp template_room;template_room.width=template_room.height=7;template_room.cells.resize(49);RoomMarkers markers{{'3',{2,4}}};
        const auto alive=read_room_condition(parse_xml("<condition condition_type='isalive' id='missing'/>"),markers,template_room);
        check(alive.kind==RoomConditionKind::is_alive && alive.missing_value && !alive.negated);
        const auto changed_definition=read_room_condition(parse_xml("<condition condition_type='activated_changed' id='lever'/>"),markers,template_room);
        check(changed_definition.wanted==-1 && changed_definition.missing_value);
        const auto started_definition=read_room_condition(parse_xml("<condition condition_type='started' id='clock' started='-2tail'/>"),markers,template_room);
        check(started_definition.wanted==1 && !started_definition.missing_value);
        const auto point_definition=read_room_condition(parse_xml("<condition CONDITION_TYPE='at' ID='player' AT='3' not='true'/>"),markers,template_room);
        check(point_definition.point.x==2 && point_definition.point.y==4 && template_room.cells[30].kind==1 && !point_definition.negated);
        const auto type_definition=read_room_condition(parse_xml("<condition condition_type='type_at' type='weapon' x=' 2suffix' y='-1'/>"),markers,template_room);
        check(type_definition.point.x==2 && type_definition.point.y==-1 && type_definition.item_subtype=="random" && !type_definition.cached_item);
        bool rejected=false;try {read_room_condition(parse_xml("<condition condition_type='at' id='x' x='1'/>"),markers,template_room);} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        rejected=false;try {read_room_condition(parse_xml("<condition condition_type='entered' unused='1'/>"),markers,template_room);} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        const auto script_definition=read_room_script(parse_xml("<script REPEAT=' -1tail' ignored='yes'><action on='failure' action_type='ticker' text='failure'/><condition condition_type='entered'/><action on='success' action_type='ticker' text='first'/><action on='success' action_type='ticker' text='second'/></script>"),markers,template_room);
        check(script_definition.success_repeat==-1 && script_definition.conditions.size()==1 && script_definition.success.size()==2 && script_definition.success[0].text=="first" && script_definition.success[1].text=="second" && script_definition.failure[0].text=="failure");
        check(read_room_script(parse_xml("<script/>"),markers,template_room).success_repeat==1);
        rejected=false;try {read_room_script(parse_xml("<script><action action_type='ticker' text='missing route'/></script>"),markers,template_room);} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
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
