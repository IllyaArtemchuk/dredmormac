#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/names.hpp"
#include <optional>
#include <array>

namespace dredmor {
struct GeometryOptions {
    int width=125,height=125,depth=0,max_shops=3;
    bool short_game=false;
    // Already-resolved active branch starter indices; empty uses original
    // default policy. Active template-pool registration is an input boundary.
    std::vector<std::size_t> starters{};
    std::size_t max_attempts=128,max_candidate_draws=1000000,max_rooms=4096;
};
struct GeneratedRoom {
    std::size_t template_index=0;
    Position origin{};
    RoomName name{};
    std::vector<std::size_t> neighbors{};
};
struct GeometryAttempt {
    DungeonLayout layout;
    std::vector<GeneratedRoom> rooms{};
    std::optional<Position> initial_player{};
    std::uint32_t shops=0;
    std::size_t candidate_draws=0,attempts=1;
    bool accepted=false;
};
// Stops at the original BuildDoors boundary: no content, stairs, secret doors,
// population or rendering. Guard failures leave the caller's RNG unchanged.
GeometryAttempt generate_geometry_attempt(const std::vector<RoomStamp>& active_pool,const RoomNameWords& words,
                                         GameRandom& random,const GeometryOptions& options={});
GeometryAttempt generate_level_geometry(const std::vector<RoomStamp>& active_pool,const RoomNameWords& words,
                                       GameRandom& random,const GeometryOptions& options={});
struct DoorAssembly {
    Position center{};
    bool horizontal=false,locked=false,open=false;
    std::array<Position,3> parts{};
};
struct DoorBuildPlan {
    std::vector<DoorAssembly> doors{};
    std::vector<Position> repaired{};
};
// Original BuildDoors traversal, repair rule and constructor/placement plan.
// Entity construction, sprites and PlaceInLevel/Open side effects are separate.
// Malformed layouts fail before mutation; generated layouts have valid margins.
DoorBuildPlan build_door_plan(DungeonLayout& layout);
}
