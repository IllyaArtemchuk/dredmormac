#pragma once
#include "dredmor/random.hpp"
#include "dredmor/stats.hpp"
#include <vector>

namespace dredmor {
struct FoodCandidate { int mana=0,level=0;bool special=false,debug=false; };
// Pools are already ordered as the original LevelSet's food table. Table
// registration/expansion merging and box-muller target-level choice are separate.
std::size_t select_edible(const std::vector<FoodCandidate>& pool,DarwinRandom& random);
std::size_t select_booze(const std::vector<FoodCandidate>& pool,DarwinRandom& random,int target_level=1);
bool loadout_allowed(const ClassRanks& ranks,int category,bool always);
struct TrapCandidate {int level=0;bool special=false,debug=false;};
struct SpawnDepthContext {int depth=0;bool alternate=false;int alternate_depth=-1;};
struct TrapSelection {std::size_t index=0;int target_level=1;unsigned attempts=0;};
// Original complete GetRandomTrap selection with supplied ordered LevelSet pool
// and tweak deviation. Entity construction and table registration are separate.
TrapSelection select_trap(const std::vector<TrapCandidate>& pool,GameRandom& random,
                          SpawnDepthContext depth,float deviation);
}
