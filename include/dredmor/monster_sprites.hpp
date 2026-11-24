#pragma once
#include "dredmor/monsters.hpp"
#include <functional>

namespace dredmor {
using MonsterSpriteHandle=std::uint64_t;
struct MonsterSpriteState {
    std::array<MonsterSpriteHandle,4> idle{},attack{},hit{},beam{}; // left/right/up/down
    MonsterSpriteHandle die=0,cast=0,dig_up=0,dig_down=0,current=0;
    bool preserve_current=false; // original Bob byte +0x19
    bool playing=true; // Bob +0x16; stopped on the death sprite's last frame
    std::uint8_t animation_flag=0; // Bob +0x17, cleared when current changes
    std::uint32_t frame=0,phase=0; // Bob +0x1c/+0x20
};
// Monster::PostGameTick (0xe2c12) uses the cached sprite at actor+0x3a8;
// selecting that cache during Update/AI is a separate boundary. Nonzero
// actor+0x3d4 suppresses this pass without clearing actor+0x350.
void post_game_tick_monster(std::int32_t& pending,std::int32_t suppressed,
    MonsterSpriteHandle cached,MonsterSpriteState& state);
using MonsterTintLoader=std::function<MonsterSpriteHandle(const std::string&,std::int32_t)>;
// Original Monster::TintSprites routing. -1 draws one shared tint angle from
// libc; explicit angles consume no random numbers. Resource/cache I/O is a
// caller boundary. Existing hit handles survive an absent left hit sprite.
std::int32_t tint_monster_sprites(const MonsterDefinition& definition,MonsterSpriteState& state,
    std::int32_t angle,DarwinRandom& random,const MonsterTintLoader& load);
using MonsterSpriteAutoLoader=std::function<MonsterSpriteHandle(const std::string& key,const std::string& path,const std::string& palette)>;
// AutoLoad calls always have the original trailing flags 0,0,1. A nonzero
// definition tint selects TintSprites instead, including -1's random angle.
std::optional<std::int32_t> rebuild_monster_sprites(const MonsterDefinition& definition,MonsterSpriteState& state,
    std::int32_t health,DarwinRandom& random,const MonsterSpriteAutoLoader& load,
    const MonsterTintLoader& tint,const std::function<std::uint16_t(MonsterSpriteHandle)>& frame_count);
}
