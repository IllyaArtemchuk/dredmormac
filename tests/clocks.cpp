#include "dredmor/clocks.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("clock assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        ActivationState activation;activation.activate(true);check(activation.changed());activation.advance_history();check(activation.changed());activation.advance_history();check(!activation.changed());
        activation.activate(false);check(activation.changed());activation.activate(true);check(!activation.changed());
        const auto expired=advance_clock({5,1,-1},ClockAction::update,0,true);
        check(expired.state.stopped() && expired.state.remaining==0 && expired.events.size()==3 && expired.message=="The timer ticks down to 0!");
        check(expired.events[0].name=="updated" && expired.events[0].state.remaining==1 && expired.events[0].state.step==-1);
        check(expired.events[1].name=="stopped" && expired.events[1].state.remaining==0 && expired.events[1].state.step==0 && expired.events[2].name=="expired");
        auto stopped=advance_clock(expired.state,ClockAction::update,0,true);check(!stopped.room_queried && !stopped.message && stopped.events.empty());
        auto reset=advance_clock(expired.state,ClockAction::reset);check(reset.state.stopped() && reset.state.remaining==5 && reset.events.size()==1);
        auto started=advance_clock({5,0,0},ClockAction::start);check(started.events[0].name=="started" && started.state.step==-1);
        auto negative=advance_clock(started.state,ClockAction::update,0,true);check(negative.state.remaining==-1 && !negative.state.stopped() && negative.events.size()==1 && negative.message=="The timer ticks down to 0!");
        auto same=advance_clock({5,0,-1},ClockAction::set_remaining,0);check(same.events.size()==2 && same.events[0].name=="stopped" && same.events[1].name=="expired");
        auto wrapped=advance_clock({0,-2147483647-1,-1},ClockAction::update);check(wrapped.state.remaining==2147483647 && !wrapped.message);
        check(advance_clock({5,5,0},ClockAction::reset).events.empty());
        check(advance_clock({0,1,0},ClockAction::stop).events[0].name=="stopped");
        std::cout<<"activation two-update history and clock event ordering, expiry, reset/start/stop, negative and wrapping counters passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
