#include "dredmor/monster_sprites.hpp"
#include <stdexcept>

namespace dredmor {
namespace {
void current_sprite(MonsterSpriteState& state,MonsterSpriteHandle sprite) {
    if (!state.preserve_current && state.current!=sprite) {
        state.current=sprite;state.animation_flag=0;state.frame=state.phase=0;
    }
}
}
void post_game_tick_monster(std::int32_t& pending,std::int32_t suppressed,
    MonsterSpriteHandle cached,MonsterSpriteState& state) {
    if (!pending || suppressed) return;
    pending=0;current_sprite(state,cached);state.playing=true;
}
std::int32_t tint_monster_sprites(const MonsterDefinition& definition,MonsterSpriteState& state,
    std::int32_t angle,DarwinRandom& random,const MonsterTintLoader& load) {
    if (!load) throw std::invalid_argument("monster tint requires a sprite loader");
    if (angle==-1) angle=static_cast<std::int32_t>((std::uint64_t(random.next())*0x59ffff5dull)>>53);
    auto group=[&](const auto& paths,auto& handles) {
        for (const auto i:std::array<unsigned,4>{1,0,2,3})
            handles[i]=load(paths[i],angle);
    };
    group(definition.idle_sprites,state.idle);
    if (!definition.attack_sprites[0].empty()) group(definition.attack_sprites,state.attack);
    else state.attack.fill(0);
    if (!definition.hit_sprites[0].empty()) group(definition.hit_sprites,state.hit);
    if (!definition.beam_sprites[0].empty()) group(definition.beam_sprites,state.beam);
    else state.beam.fill(0);
    state.cast=definition.cast_sprite.empty()?0:load(definition.cast_sprite,angle);
    state.die=definition.die_sprite.empty()?0:load(definition.die_sprite,angle);
    if (!definition.dig_up_sprite.empty() && !definition.dig_down_sprite.empty()) {
        state.dig_up=load(definition.dig_up_sprite,angle);
        state.dig_down=load(definition.dig_down_sprite,angle);
    } else state.dig_up=state.dig_down=0;
    current_sprite(state,state.idle[3]);
    return angle;
}
std::optional<std::int32_t> rebuild_monster_sprites(const MonsterDefinition& definition,MonsterSpriteState& state,
    std::int32_t health,DarwinRandom& random,const MonsterSpriteAutoLoader& load,
    const MonsterTintLoader& tint,const std::function<std::uint16_t(MonsterSpriteHandle)>& frame_count) {
    std::optional<std::int32_t> angle;
    if (definition.palette_tint) angle=tint_monster_sprites(definition,state,definition.palette_tint,random,tint);
    else {
        if (!load) throw std::invalid_argument("monster rebuild requires an automatic sprite loader");
        auto one=[&](const std::string& path) {return load(path+definition.palette,path,definition.palette);};
        auto group=[&](const auto& paths,auto& handles,bool right_first) {
            const auto order=right_first?std::array<unsigned,4>{1,0,2,3}:std::array<unsigned,4>{0,1,2,3};
            for (auto i:order) handles[i]=one(paths[i]);
        };
        group(definition.idle_sprites,state.idle,true);
        if (!definition.attack_sprites[0].empty()) group(definition.attack_sprites,state.attack,false);
        else state.attack.fill(0);
        if (!definition.hit_sprites[0].empty()) group(definition.hit_sprites,state.hit,false);
        if (!definition.beam_sprites[0].empty()) group(definition.beam_sprites,state.beam,false);
        else state.beam.fill(0);
        state.cast=definition.cast_sprite.empty()?0:one(definition.cast_sprite);
        state.die=definition.die_sprite.empty()?0:one(definition.die_sprite);
        if (!definition.dig_up_sprite.empty() && !definition.dig_down_sprite.empty()) {
            state.dig_up=one(definition.dig_up_sprite);state.dig_down=one(definition.dig_down_sprite);
        } else state.dig_up=state.dig_down=0;
    }
    if (health>0) current_sprite(state,state.idle[3]);
    else {
        if (!state.die || !frame_count) throw std::invalid_argument("dead monster requires a death sprite and frame count");
        current_sprite(state,state.die);
        state.frame=std::uint32_t(frame_count(state.die))-1U;state.playing=false;
    }
    return angle;
}
}
