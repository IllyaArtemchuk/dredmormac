#include "dredmor/game_tick.hpp"
#include "dredmor/clocks.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("game tick assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        GameTickState state;state.level=TickLevelState{1,1,true,{1,2},{{10}}};state.effect_durations[10]=1;
        state.player.position={2,3};state.player.respawn_counter=100;state.player.action_delay=-1;state.spell_cooldowns={-1,0,1,2};
        state.colour_strings={"queued"};state.teleports={{1,7,{8,9}}};state.game_message="old";state.announced_full_inventory=true;
        GameRandom random;random.seed(7);ClockState clock{1,1,-1};bool expired=false,trigger_seen=false,teleported=false;std::vector<TickRequestKind> phases;std::vector<std::uint64_t> updates,posts;
        GameTickHooks hooks;hooks.ticks=[] {return 1U;};hooks.perform=[&](const TickRequest& request) {
            phases.push_back(request.kind);
            switch (request.kind) {
                case TickRequestKind::object_update:
                    updates.push_back(request.object);
                    if (request.object==2) {const auto result=advance_clock(clock,ClockAction::update);clock=result.state;expired=result.events.back().name=="expired";state.level->objects.push_back(3);}
                    break;
                case TickRequestKind::room_triggers:
                    check(expired && clock.remaining==0 && clock.stopped() && state.effect_durations.at(10)==1 && !teleported);trigger_seen=true;break;
                case TickRequestKind::post_game_tick:check(trigger_seen && state.level->tile_effects[0].empty());posts.push_back(request.object);break;
                case TickRequestKind::teleport:check((posts==std::vector<std::uint64_t>{1,2,3}) && request.level==7 && request.position.x==8);teleported=true;break;
                default:break;
            }
        };
        run_game_tick(state,random,hooks);
        check((updates==std::vector<std::uint64_t>{1,2,3}) && teleported && state.teleports.empty());
        check(state.player.respawn_counter==1 && state.player.action_delay==-2 && (state.spell_cooldowns==std::vector<std::int32_t>{-1,0,0,1}));
        check(!state.announced_full_inventory && state.game_message.empty() && state.colour_strings.empty());
        check(std::find(phases.begin(),phases.end(),TickRequestKind::room_triggers)<std::find(phases.begin(),phases.end(),TickRequestKind::clear_destruction));
        GameTickState absent;absent.slow_time_counter=1;absent.colour_strings={"retained"};absent.game_message="old";phases.clear();
        hooks.perform=[&](const TickRequest& request) {phases.push_back(request.kind);};run_game_tick(absent,random,hooks);
        check(absent.slow_time_counter==0 && absent.colour_strings.size()==1 && phases==std::vector<TickRequestKind>{TickRequestKind::fix_safe_teleports});
        state.player.dead=0;state.last_autosave=0;unsigned calls=0;hooks.ticks=[&] {return calls++?300123U:300001U;};phases.clear();run_game_tick(state,random,hooks);
        check(calls==2 && state.last_autosave==300123 && std::find(phases.begin(),phases.end(),TickRequestKind::save_game)!=phases.end());
        std::cout<<"original tick ordering exposes fresh clock expiry to scripts, defers movement until post-tick, updates appended objects, expires tile effects and preserves cooldown/autosave gates\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
