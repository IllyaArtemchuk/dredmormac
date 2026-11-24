#pragma once
#include "dredmor/dungeon.hpp"
#include "dredmor/abilities.hpp"
#include <string>

namespace dredmor {
// Door::Clicked scans the first seven skill slots, using strict level < rank.
bool has_free_door_pick(const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities);
struct DoorState {
    DoorAssembly assembly{};
    std::array<std::int32_t,3> kicks{};
    bool destroyed=false;
};
struct DoorListener {
    Position position{};
    std::uint32_t flags=0;
    std::uint8_t tile_kind=0;
};
// Resolved ability flags, item kinds, attack sound and actor list are caller
// inputs. Zero means no lockpick in that slot; positive values are quantities.
struct DoorClickContext {
    Position player{};
    bool free_pick=false,burglary=false,female=false;
    std::int32_t depth=0,lockpick_xp=0,stealth=0;
    std::string attack_sound{};
    std::array<std::int32_t,9> belt_picks{};
    std::array<std::int32_t,48> bag_picks{};
    std::vector<DoorListener> listeners{};
};
struct DoorDamageRequest {
    int amount=2,type=2;
    std::string death_reason{};
};
struct DoorClickResult {
    bool handled=false,eye_candy=false;
    std::vector<std::string> messages{},sounds{};
    std::optional<int> animation_facing{};
    std::optional<std::int32_t> experience{};
    std::optional<DoorDamageRequest> damage{};
    // Belt slots 0..8, bag slots 9..56. Destruction is deferred in the game.
    std::optional<std::size_t> consumed_pick{},destroyed_pick{};
    std::vector<std::size_t> destroyed_parts{};
};
// Original Door::Clicked decision/RNG flow. Damage, XP, animation, sound and
// destruction are requests; their complete downstream routines are separate.
// The occupancy callback must inspect stable input; exceptions roll back state.
DoorClickResult click_door(DungeonLayout& layout,DoorState& door,std::size_t selected,
    DoorClickContext& context,GameRandom& random,const std::function<bool(Position)>& obstructed);
}
