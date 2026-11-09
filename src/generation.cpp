#include "dredmor/generation.hpp"
#include <stdexcept>

namespace dredmor {
TileGrid::TileGrid(int width,int height,std::vector<std::uint8_t> cells)
    :width_(width),height_(height),cells_(std::move(cells)) {
    if (width<1 || height<1 || width>512 || height>512 || cells_.size()!=std::size_t(width)*height)
        throw std::invalid_argument("tile grid requires 1..512 dimensions and exact cell count");
}
std::uint8_t TileGrid::at(int x,int y) const {
    if (x<0 || y<0 || x>=width_ || y>=height_) throw std::out_of_range("tile grid coordinate");
    return cells_[std::size_t(y)*width_+x];
}
bool TileGrid::accepts(const TileGrid& room,int x,int y) const {
    // Original reserves a three-cell margin, including void template cells.
    if (x<3 || y<3 || std::int64_t(x)+room.width_>width_-3 || std::int64_t(y)+room.height_>height_-3)
        return false;
    for (int column=0;column<room.width_;++column) for (int row=0;row<room.height_;++row) {
        const auto incoming=room.at(column,row),existing=at(x+column,y+row);
        if (incoming && existing && !(incoming==2 && existing==2) && !(incoming==6 && existing==6))
            return false;
    }
    return true;
}
}
