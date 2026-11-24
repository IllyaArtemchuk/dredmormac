#pragma once
#include "dredmor/monsters.hpp"
#include "dredmor/generation.hpp"
#include "dredmor/room_placement.hpp"
#include <functional>

namespace dredmor {
struct HordeRoomBounds {Position minimum{},maximum{};};
struct HordeQuest {std::uint64_t identity=0;std::string platoon;};
struct HordeOptions {
    Position position{};
    std::int32_t depth=0,minimum=-1,additional=-1;
    std::size_t level_set=0,budget=1000000;
    bool expansion=false;
    std::uint64_t level=1;
    std::optional<std::string> named;
    std::optional<HordeQuest> quest;
};
struct HordeWorld {
    const DungeonLayout& argument_level;
    const DungeonLayout& current_level;
    const std::vector<bool>& argument_occupied;
    const std::vector<bool>& current_occupied;
    const std::vector<HordeRoomBounds>& current_rooms;
};
struct HordeMember {
    std::uint64_t identity=0,leader=0,quest=0;
    std::uint32_t definition=0;
    PrimaryStats primary_percent=default_primary_percentages();
    std::string moniker,platoon;
    std::vector<std::uint64_t> followers;
    bool horde=false;
};
struct HordeSearch {Position seed{},position{};bool found=false,accepted=false;};
struct HordeResult {
    bool accepted=false;
    std::int32_t depth=0,table_level=0,minimum=0,additional=0;
    std::uint32_t selected=0;
    Position leader_position{};
    std::optional<std::int32_t> quest_count;
    std::vector<HordeMember> members;
    std::vector<RoomTeleport> teleports;
    std::vector<HordeSearch> searches;
};
struct HordeHooks {
    std::function<std::int32_t(std::string_view)> tweak;
    std::function<bool(std::uint32_t,std::int32_t,bool)> can_spawn;
    std::function<std::uint32_t(std::string_view)> find_named;
    std::function<HordeMember(std::uint32_t,std::int32_t,GameRandom&)> construct;
    std::function<std::string(GameRandom&)> monster_name,platoon_name;
    std::function<void(HordeMember&,std::int32_t,GameRandom&)> tint;
    std::function<void(HordeMember&,GameRandom&)> rebuild;
    // SafeTeleport queues placement; it must not reserve tile occupancy here.
    std::function<void(const RoomTeleport&,const HordeMember&)> queue;
};
// Full Level::SpawnHorde control flow around explicit constructor/name/sprite/
// attribute boundaries. Caller applies returned depth and quest count and owns
// a transaction over the world, callbacks and RNG. Malformed pools/search loops
// use the execution budget rather than reproducing original unbounded loops.
HordeResult spawn_horde(const MonsterTables& tables,const std::vector<MonsterDefinition>& definitions,
    HordeOptions options,const HordeWorld& world,GameRandom& random,const HordeHooks& hooks);
}
