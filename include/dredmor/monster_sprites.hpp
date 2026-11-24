#pragma once
#include "dredmor/monsters.hpp"
#include <functional>

namespace dredmor {
using MonsterSpriteHandle=std::uint64_t;
struct MonsterSpriteState {
    std::array<MonsterSpriteHandle,4> idle{},attack{},hit{},beam{}; // left/right/up/down
    MonsterSpriteHandle die=0,cast=0,dig_up=0,dig_down=0,current=0;
    bool preserve_current=false; // original Bob byte +0x19
    std::uint8_t animation_flag=0; // Bob +0x17, cleared when current changes
    std::uint32_t frame=0,phase=0; // Bob +0x1c/+0x20
};
using MonsterTintLoader=std::function<MonsterSpriteHandle(const std::string&,std::int32_t)>;
// Original Monster::TintSprites routing. -1 draws one shared tint angle from
// libc; explicit angles consume no random numbers. Resource/cache I/O is a
// caller boundary. Existing hit handles survive an absent left hit sprite.
std::int32_t tint_monster_sprites(const MonsterDefinition& definition,MonsterSpriteState& state,
    std::int32_t angle,DarwinRandom& random,const MonsterTintLoader& load);
}
