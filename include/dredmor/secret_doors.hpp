#pragma once
#include "dredmor/dungeon.hpp"
#include "dredmor/inventory.hpp"

namespace dredmor {
struct SecretDoorPlacement {
    Position wall{},pocket{};
    std::optional<InventoryItem> treasure;
    std::optional<Position> lever;
};
// Original GenerateSecretDoors numeric/placement phase. room_no_blockers supplies
// each placed room's noblockers flag (+0x7c); occupied is the tile object-list
// occupancy in row-major order. Both refer to the post-room-content floor.
// The treasure factory executes at the original point in the random sequence,
// receives a provisional RNG and returns a value without external side effects.
// Generated treasure and levers join occupancy for subsequent searches.
// Failure leaves layout/RNG unchanged. Entity construction/IDs/sprites and
// destruction-list bookkeeping are separate from this placement plan.
std::vector<SecretDoorPlacement> generate_secret_door_plan(DungeonLayout& layout,
    GameRandom& random,const std::vector<bool>& room_no_blockers,
    const std::vector<bool>& occupied,
    const std::function<std::optional<InventoryItem>(GameRandom&)>& treasure_factory);
}
