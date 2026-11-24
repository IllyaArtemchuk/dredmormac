#pragma once
#include "dredmor/random.hpp"
#include "dredmor/room_placement.hpp"
#include <functional>

namespace dredmor {
struct TickPlayerState {
    Position position{};
    std::int32_t dead=0,respawn_counter=0,action_delay=0;
};
struct TickLevelState {
    int width=0,height=0;
    // Original Level byte+0x60 gates player buffs, respawn and spell cooldowns.
    bool updates_player=false;
    std::vector<std::uint64_t> objects;
    std::vector<std::vector<std::uint64_t>> tile_effects;
};
struct TickTeleport {
    std::uint64_t object=0,level=0;
    Position position{};
};
struct GameTickState {
    bool announced_full_inventory=false;
    std::int32_t slow_time_counter=0,tutorial=0;
    std::string game_message;
    std::optional<TickLevelState> level;
    TickPlayerState player;
    std::map<std::uint64_t,std::int32_t> effect_durations;
    std::vector<std::string> colour_strings;
    std::vector<std::int32_t> spell_cooldowns;
    std::vector<TickTeleport> teleports;
    std::uint32_t last_autosave=0;
};
enum class TickRequestKind {
    fix_safe_teleports,give_zero_experience,visibility,object_update,player_buffs,
    player_triggers,room_triggers,clear_destruction,director,spawn_monster,ticker,
    play_synchro_bobs,clear_synchro_bobs,post_game_tick,teleport,tutorial,save_game,
    platform_callbacks
};
struct TickRequest {
    TickRequestKind kind;
    std::uint64_t object=0,level=0;
    Position position{};
    std::string text;
};
struct GameTickHooks {
    // Invoked immediately; callbacks may mutate state and consume supplied RNG.
    // Original virtual methods/subsystems stay separate from this outer driver.
    std::function<void(const TickRequest&)> perform;
    std::function<std::uint32_t()> ticks;
};
// Outer GameTick::0x2b480 control flow, including timed effect membership,
// incidental colour RNG, cooldowns and deferred end-of-tick teleports. The
// owning Session must transact callbacks/state/RNG together on failure.
void run_game_tick(GameTickState& state,GameRandom& random,const GameTickHooks& hooks);
}
