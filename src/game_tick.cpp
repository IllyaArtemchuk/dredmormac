#include "dredmor/game_tick.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t add(std::int32_t value,std::int32_t amount) {
    const auto bits=std::uint32_t(value)+std::uint32_t(amount);std::int32_t result;
    std::memcpy(&result,&bits,4);return result;
}
}
void run_game_tick(GameTickState& state,GameRandom& random,const GameTickHooks& hooks) {
    if (!hooks.perform || !hooks.ticks) throw std::invalid_argument("game tick requires subsystem and clock hooks");
    auto request=[&](TickRequestKind kind,std::uint64_t object=0,Position position=Position{},std::string text={},std::uint64_t level=0) {
        hooks.perform({kind,object,level,position,std::move(text)});
    };
    state.announced_full_inventory=false;
    request(TickRequestKind::fix_safe_teleports);
    state.slow_time_counter=add(state.slow_time_counter,1);
    if (state.slow_time_counter==2) state.slow_time_counter=0;
    state.game_message.clear();
    if (!state.level) return;
    request(TickRequestKind::give_zero_experience);
    request(TickRequestKind::visibility,0,state.player.position);
    for (std::size_t i=0;i<state.level->objects.size();++i) {
        if (state.player.dead!=0) break;
        request(TickRequestKind::object_update,state.level->objects[i]);
    }
    if (state.level->updates_player) {
        request(TickRequestKind::player_buffs);
        request(TickRequestKind::player_triggers);
    }
    request(TickRequestKind::room_triggers);
    request(TickRequestKind::clear_destruction);
    request(TickRequestKind::director);
    auto& level=*state.level;
    if (level.width<0 || level.height<0 || level.tile_effects.size()!=std::size_t(level.width)*std::size_t(level.height))
        throw std::invalid_argument("game tick effect grid mismatch");
    for (auto& effects:level.tile_effects) {
        std::vector<std::uint64_t> expired;
        for (const auto effect:effects) {
            auto& duration=state.effect_durations.at(effect);
            if (duration==-1) continue;
            duration=add(duration,-1);
            if (!duration) expired.push_back(effect);
        }
        for (const auto effect:expired) effects.erase(std::remove(effects.begin(),effects.end(),effect),effects.end());
    }
    if (!state.tutorial && state.level->updates_player) {
        state.player.respawn_counter=add(state.player.respawn_counter,1);
        if (state.player.respawn_counter>=101) {
            state.player.respawn_counter=add(state.player.respawn_counter,-100);
            request(TickRequestKind::spawn_monster,0,{-1,-1});
        }
    }
    request(TickRequestKind::visibility,0,state.player.position);
    std::uint32_t draw;
    do {draw=random.mt.u32();} while (draw>0xfffffed7U);
    if (draw<=0x83126dU && !state.tutorial && !state.colour_strings.empty()) {
        const auto selected=random.libc.bounded(std::uint32_t(state.colour_strings.size()));
        request(TickRequestKind::ticker,0,{},state.colour_strings.at(selected));
    }
    state.colour_strings.clear();
    request(TickRequestKind::play_synchro_bobs);
    request(TickRequestKind::clear_synchro_bobs);
    for (std::size_t i=0;i<state.level->objects.size();++i) request(TickRequestKind::post_game_tick,state.level->objects[i]);
    if (state.level->updates_player) for (auto& cooldown:state.spell_cooldowns) if (cooldown>0) --cooldown;
    if (state.player.action_delay) state.player.action_delay=add(state.player.action_delay,-1);
    for (std::size_t i=0;i<state.teleports.size();++i) {
        const auto teleport=state.teleports[i];request(TickRequestKind::teleport,teleport.object,teleport.position,{},teleport.level);
    }
    state.teleports.clear();
    if (state.tutorial) request(TickRequestKind::tutorial);
    const auto now=hooks.ticks();
    if (std::uint32_t(now-state.last_autosave)>=300001U && !state.player.dead) {
        request(TickRequestKind::save_game,0,{},"autosave.dredmorsave");
        state.last_autosave=hooks.ticks();
        request(TickRequestKind::ticker,0,{},"Game autosaved.");
    }
    request(TickRequestKind::platform_callbacks);
}
}
