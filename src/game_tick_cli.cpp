#include "dredmor/game_tick.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            GameRandom random;random.seed(seed);GameTickState state;bool has_level,updates;int width,height,kill_at;std::uint64_t move_at,append_at;std::uint32_t now,next_time;std::size_t count;
            std::cin>>has_level>>updates>>width>>height>>state.slow_time_counter>>state.tutorial>>state.player.dead>>state.player.respawn_counter>>state.player.action_delay>>state.last_autosave>>now>>next_time>>kill_at>>move_at>>append_at;
            state.announced_full_inventory=true;state.game_message="old";state.player.position={2,3};
            if (width<0 || height<0 || width>8 || height>8) throw std::invalid_argument("invalid tick grid");
            TickLevelState level;level.width=width;level.height=height;level.updates_player=updates;
            std::cin>>count;if (count>100) throw std::invalid_argument("too many tick objects");for (std::size_t i=0;i<count;++i) {std::uint64_t object;std::cin>>object;level.objects.push_back(object);}
            std::cin>>count;if (count>100) throw std::invalid_argument("too many tick effects");for (std::size_t i=0;i<count;++i) {std::uint64_t id;std::int32_t duration;std::cin>>id>>duration;state.effect_durations[id]=duration;}
            for (int i=0;i<width*height;++i) {std::cin>>count;if (count>100) throw std::invalid_argument("too many tile effects");std::vector<std::uint64_t> entries(count);for (auto& entry:entries) std::cin>>entry;level.tile_effects.push_back(std::move(entries));}
            std::cin>>count;if (count>100) throw std::invalid_argument("too many cooldowns");state.spell_cooldowns.resize(count);for (auto& cooldown:state.spell_cooldowns) std::cin>>cooldown;
            std::cin>>count;if (count>100) throw std::invalid_argument("too many colour strings");for (std::size_t i=0;i<count;++i) {std::string text;std::cin>>std::quoted(text);state.colour_strings.push_back(text);}
            std::cin>>count;if (count>100) throw std::invalid_argument("too many teleports");state.teleports.resize(count);for (auto& teleport:state.teleports) std::cin>>teleport.object>>teleport.level>>teleport.position.x>>teleport.position.y;
            if (!std::cin) throw std::runtime_error("truncated tick fixture");
            if (has_level) state.level=std::move(level);
            std::vector<TickRequest> requests;std::size_t clock_calls=0;
            GameTickHooks hooks;hooks.ticks=[&] {return clock_calls++?next_time:now;};hooks.perform=[&](const TickRequest& request) {
                requests.push_back(request);
                if (request.kind==TickRequestKind::object_update) {
                    if (request.object==std::uint64_t(kill_at)) state.player.dead=1;
                    if (request.object==move_at) state.player.position={7,8};
                    if (request.object==append_at) state.level->objects.push_back(99);
                }
            };
            run_game_tick(state,random,hooks);
            std::cout<<state.announced_full_inventory<<' '<<state.slow_time_counter<<' '<<std::quoted(state.game_message)<<' '<<state.player.dead<<' '<<state.player.position.x<<' '<<state.player.position.y<<' '<<state.player.respawn_counter<<' '<<state.player.action_delay<<' '<<state.last_autosave<<' '<<clock_calls<<' '<<random.libc.state();
            const auto mt=random.mt.state();std::cout<<' '<<mt.index;for (auto word:mt.words) std::cout<<' '<<word;
            for (unsigned i=0;i<8;++i) std::cout<<' '<<random.mt.u32();
            std::cout<<' '<<state.effect_durations.size();for (const auto& [id,duration]:state.effect_durations) std::cout<<' '<<id<<' '<<duration;
            const auto& final_level=state.level?*state.level:level;
            std::cout<<' '<<final_level.objects.size();for (auto id:final_level.objects) std::cout<<' '<<id;
            for (const auto& entries:final_level.tile_effects) {std::cout<<' '<<entries.size();for (auto id:entries) std::cout<<' '<<id;}
            for (auto cooldown:state.spell_cooldowns) std::cout<<' '<<cooldown;
            std::cout<<' '<<state.colour_strings.size()<<' '<<state.teleports.size()<<' '<<requests.size();
            for (const auto& request:requests) std::cout<<' '<<int(request.kind)<<' '<<request.object<<' '<<request.level<<' '<<request.position.x<<' '<<request.position.y<<' '<<std::quoted(request.text);
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
