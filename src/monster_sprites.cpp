#include "dredmor/monster_sprites.hpp"
#include <stdexcept>

namespace dredmor {
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
    if (!state.preserve_current && state.current!=state.idle[3]) {
        state.current=state.idle[3];state.animation_flag=0;state.frame=state.phase=0;
    }
    return angle;
}
}
