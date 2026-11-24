#include "dredmor/room_scripts.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room trigger assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        auto definitions=std::make_shared<std::vector<RoomScriptDefinition>>(3);definitions->at(0).success_repeat=-1;definitions->at(1).success_repeat=0;
        std::vector<RoomTriggerRoom> rooms{{true,1,0,definitions},{true,1,1,definitions},{false,1,2,definitions},{true,0,3,definitions},{true,1,4,{}}};
        RoomTriggerState state;state.previous_room=-1;state.success_counts[{0,0}]=std::numeric_limits<std::int32_t>::max();set_room_event(state.events,0,"clock","expired");
        const auto size=state.events.size();check(room_event_is_set(state.events,0,"clock","expired") && !room_event_is_set(state.events,0,"Clock","expired") && state.events.size()==size);
        int player_room=1,conditions=0,successes=0;bool saw_unlimited_absent=false;RoomTriggerCallbacks callbacks;
        callbacks.player_room=[&] {return player_room;};
        callbacks.conditions_positive=[&](std::size_t r,std::size_t s) {++conditions;check(state.previous_room==-1);if (r==1 && s==0) saw_unlimited_absent=state.success_counts.find({1,0})==state.success_counts.end();return true;};
        callbacks.success_actions=[&](auto r,auto s) {++successes;check(state.previous_room==-1);set_room_event(state.events,0,"clock","started");if (r==1 && s==2) player_room=0;};
        const auto result=run_room_trigger_logic(rooms,state,callbacks);
        check(result.checked.size()==6 && conditions==4 && successes==4 && result.invalid_room_ids.empty() && saw_unlimited_absent && state.events.empty() && state.previous_room==0);
        check(state.success_counts.at({0,0})==std::numeric_limits<std::int32_t>::min() && state.success_counts.at({1,0})==1 && state.success_counts.at({0,1})==0);
        callbacks.conditions_positive=[&](auto,auto){++conditions;check(state.previous_room==0);return true;};callbacks.success_actions=[&](auto,auto){++successes;};
        conditions=successes=0;const auto second=run_room_trigger_logic(rooms,state,callbacks);check(second.conditions.size()==2 && conditions==2 && successes==2 && state.success_counts.at({1,2})==1);
        RoomTriggerState debug_state;definitions=std::make_shared<std::vector<RoomScriptDefinition>>(1);definitions->at(0).success_repeat=-1;rooms={RoomTriggerRoom{true,1,0,definitions}};
        callbacks.conditions_positive=[&](auto,auto){check(debug_state.success_counts.count({0,0})==1);return false;};
        const auto debug=run_room_trigger_logic(rooms,debug_state,callbacks,true);check(debug.successes.empty() && debug_state.success_counts.at({0,0})==0);
        check(!wants_room_script_success(debug_state,{7,8},-2) && debug_state.success_counts.at({7,8})==0);
        std::cout<<"room script repeat gates, live event visibility, shared templates, counter wrapping/debug insertion and previous-room/event lifecycle passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
