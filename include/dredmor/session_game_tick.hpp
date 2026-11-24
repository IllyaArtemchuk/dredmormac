#pragma once
#include "dredmor/game_tick.hpp"
#include "dredmor/session.hpp"

namespace dredmor {
struct SessionGameTickHooks {
    // All mutations must use these candidates, including any Session methods
    // that consume randomness. Do not capture and mutate the committed owners.
    // Virtual object methods, scripts and subsystem bodies remain explicit
    // boundaries; the bridge does not substitute the provisional enemy AI.
    std::function<void(Session&,GameTickState&,const TickRequest&)> perform;
    std::function<std::uint32_t(Session&,GameTickState&)> ticks;
};
// Owns the transaction around the recovered outer driver. Callbacks and the
// driver share the candidate Session's libc, MT and normal-cache state. Driver
// state is caller-owned and must be persisted alongside Session by its owner.
// Presentation/I/O requests should be buffered by callbacks until success.
class SessionGameTickExecutor {
public:
    static void run(Session& session,GameTickState& state,const SessionGameTickHooks& hooks);
};
}
