#pragma once
#include "dredmor/content.hpp"

namespace dredmor {
// Base-content character initialization. Room generation/population, tutorials,
// weapon-family ability rebuilding and active effects are separate.
Session create_character(const GameContent& content,World world,const std::vector<int>& selected,
                         std::uint32_t seed,Difficulty difficulty=Difficulty::medium);
}
