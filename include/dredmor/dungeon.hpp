#pragma once
#include "dredmor/generation.hpp"
#include "dredmor/names.hpp"
#include <optional>
#include <array>
#include <functional>

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
enum class DoorOutcome { opened,closed,obstructed,center_ignored,locked };
struct DoorTransition { DoorOutcome outcome;bool handled; };
// The raw toggle does not test locks. Remote activation checks the selected
// door's lock first. Click/lockpick/kicking and action scheduling are separate.
DoorTransition toggle_door(DungeonLayout& layout,DoorAssembly& door,std::size_t selected_part,
                           const std::function<bool(Position)>& obstructed);
DoorTransition activate_remote_door(DungeonLayout& layout,DoorAssembly& door,std::size_t selected_part,
                                    const std::function<bool(Position)>& obstructed);
// Original content-pass preparation: clear eight cells around each remaining
// door marker, then add a wall above ordinary floor where that cell is void.
// Traversal observes earlier writes; recorded floor count is not recomputed.
void punch_out_dungeon_content(DungeonLayout& layout);
struct StairPlacement {
    Position position{};
    bool up=false;
    std::uint8_t link=0;
};
// Original BuildStairs, after stair markers have been placed. Visits x-major,
// descending y, observing walls written above earlier stairs. No RNG draws.
std::vector<StairPlacement> build_stair_plan(DungeonLayout& layout,std::int32_t depth,bool expanded_campaign);
struct StairDestination {
    std::uint8_t link=0;
    std::int32_t branch=0,depth=0;
};
// Stairs::SteppedOn dispatch arguments; caller reads link from the player tile.
// Loading/caching a destination level is a separate operation.
StairDestination stair_destination(bool up,std::uint8_t player_tile_link,std::int32_t branch,std::int32_t depth);
// Ascend searches matching downward stairs, then any downward stair. Descend
// falls back to any upward stair only for a newly generated destination.
std::optional<Position> stair_arrival(const std::vector<StairPlacement>& stairs,bool ascending,std::uint8_t link,bool newly_generated=false);
}
