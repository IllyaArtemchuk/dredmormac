#pragma once
#include <cstdint>
#include <vector>

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
}
