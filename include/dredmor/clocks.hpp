#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dredmor {
struct ActivationState {
    std::int32_t current=0,previous=0,older=0;
    bool activated() const {return current!=0;}
    bool changed() const {return current!=older;}
    void activate(bool value) {current=value?1:0;}
    // Called after the base GameObj::Update boundary. Change detection spans
    // two stored updates in the original, rather than a single previous value.
    void advance_history() {older=previous;previous=current;}
};
struct ClockState {
    std::int32_t duration=0,remaining=0,step=0;
    bool stopped() const {return step==0;}
};
enum class ClockAction {set_remaining,start,stop,reset,update};
struct ClockEvent {
    std::string name;
    // State at the original SendEvent call, before subsequent mutations.
    ClockState state;
};
struct ClockResult {
    ClockState state;
    std::vector<ClockEvent> events;
    bool room_queried=false;
    std::optional<std::string> message;
};
// Clock's event/state decisions. Event queue delivery and room identity lookup
// are external; same_room compares resolved RoomInfo identities, not tile IDs.
ClockResult advance_clock(ClockState state,ClockAction action,std::int32_t amount=0,bool same_room=false);
}
