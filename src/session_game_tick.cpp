#include "dredmor/session_game_tick.hpp"
#include <stdexcept>

namespace dredmor {
void SessionGameTickExecutor::run(Session& session,GameTickState& state,const SessionGameTickHooks& hooks) {
    if (!hooks.perform || !hooks.ticks) throw std::invalid_argument("Session game tick requires subsystem and clock hooks");
    if (session.game_tick_active_ || session.script_phase_pending_)
        throw std::invalid_argument("finish the current Session phase before starting a game tick");
    auto candidate=session;auto pending=state;candidate.game_tick_active_=true;
    GameTickHooks bridge;
    bridge.perform=[&](const TickRequest& request) {hooks.perform(candidate,pending,request);};
    bridge.ticks=[&] {return hooks.ticks(candidate,pending);};
    run_game_tick(pending,candidate.random_,bridge);
    if (candidate.script_phase_pending_ || candidate.queued_horde_members())
        throw std::runtime_error("game tick left unfinished Session placements or scripts");
    candidate.game_tick_active_=false;
    session=std::move(candidate);state=std::move(pending);
}
}
