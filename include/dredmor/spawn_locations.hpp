#pragma once
#include "dredmor/generation.hpp"
#include <optional>

namespace dredmor {
// Level::SpawnMonster's normal (non-horde) placement phase. Either coordinate
// -1 randomizes both. Normal candidates need empty kind1/14 squares; special1
// instead samples the supplied ordered site list and accepts kind7 regardless
// of occupancy. No sites returns null after the initial x/y draws.
std::optional<Position> select_monster_spawn_position(const DungeonLayout& layout,const std::vector<bool>& occupied,
    const std::vector<Position>& special_sites,Position requested,std::int32_t special,DarwinRandom& random,
    std::size_t budget=1000000);
}
