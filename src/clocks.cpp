#include "dredmor/clocks.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
ClockResult advance_clock(ClockState state,ClockAction action,std::int32_t amount,bool same_room) {
    ClockResult result;auto event=[&](const char* name) {result.events.push_back({name,state});};
    auto set=[&](std::int32_t value) {
        if (state.remaining!=value) {event("updated");state.remaining=value;}
        if (!state.stopped() && state.remaining==0) {state.step=0;event("stopped");event("expired");}
    };
    switch (action) {
        case ClockAction::set_remaining:set(amount);break;
        case ClockAction::start:state.step=-1;event("started");break;
        case ClockAction::stop:state.step=0;event("stopped");break;
        case ClockAction::reset:set(state.duration);break;
        case ClockAction::update:
            if (!state.stopped()) {
                const auto bits=std::uint32_t(state.remaining)+std::uint32_t(state.step);std::int32_t value;std::memcpy(&value,&bits,4);set(value);
                result.room_queried=true;
                if (same_room) result.message=state.remaining>0?"The timer ticks down to "+std::to_string(state.remaining)+"...":"The timer ticks down to 0!";
            }
            break;
        default:throw std::invalid_argument("invalid clock action");
    }
    result.state=state;return result;
}
}
