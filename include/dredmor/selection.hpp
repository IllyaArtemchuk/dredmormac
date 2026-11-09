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
}
