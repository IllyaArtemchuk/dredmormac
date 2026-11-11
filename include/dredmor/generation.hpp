#pragma once
#include <cstdint>
#include <vector>
#include "dredmor/random.hpp"
#include "dredmor/world.hpp"

namespace dredmor {
// Numeric room cells, before collision/rendering: 0 void, 2 wall, 6 door.
// Other nonzero values remain distinct; this is not a generated World.
class TileGrid {
public:
    TileGrid(int width,int height,std::vector<std::uint8_t> cells);
    int width() const { return width_; }
    int height() const { return height_; }
    std::uint8_t at(int x,int y) const;
    // Recovered Level::ScanRoomForTemplate kernel; no RNG draws or mutation.
    bool accepts(const TileGrid& room,int x,int y) const;
private:
    int width_,height_;
    std::vector<std::uint8_t> cells_;
};
struct TemplateCell { std::uint8_t kind=0,door_orientation=0,wall_marker=0; };
struct RoomPlacementRules {
    int minimum_level=0,maximum_level=25,room_set=0;
    bool quest=false,unique=false,diggle_god=false;
    bool no_traps=false,no_monsters=false,no_blockers=false,no_treasure=false;
};
struct RoomStamp {
    int width=0,height=0;
    std::vector<TemplateCell> cells;
    bool shop=false,zoo=false;
    RoomPlacementRules rules{};
    std::string fixed_name;
};
struct DungeonCell {
    std::uint8_t kind=0,room_id=0,door_flags=0;
    std::uint32_t flags=0;
    std::uint8_t stair_link=0;
};
class DungeonLayout {
public:
    DungeonLayout(int width,int height,std::vector<DungeonCell> cells);
    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<DungeonCell>& cells() const { return cells_; }
    DungeonCell& at(int x,int y);
    const DungeonCell& at(int x,int y) const;
    bool accepts(const RoomStamp& room,int x,int y) const;
    const std::vector<Position>& new_doors() const { return doors_; }
    std::uint32_t floor_count() const { return floor_count_; }
    void floor_count(std::uint32_t value) { floor_count_=value; }
    // Original WriteTemplate cell loop only. Room naming, placed-room records,
    // counters outside this loop and subsequent content population are separate.
    void write_template(const RoomStamp& room,int x,int y,std::uint8_t room_id,int depth,Random& random);
private:
    int width_,height_;
    std::vector<DungeonCell> cells_;
    std::vector<Position> doors_;
    std::uint32_t floor_count_=0;
};
}
