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
DungeonLayout::DungeonLayout(int width,int height,std::vector<DungeonCell> cells)
    :width_(width),height_(height),cells_(std::move(cells)) {
    if (width<1 || height<1 || width>512 || height>512 || cells_.size()!=std::size_t(width)*height)
        throw std::invalid_argument("dungeon layout requires exact bounded dimensions");
}
void DungeonLayout::write_template(const RoomStamp& room,int x,int y,std::uint8_t room_id,int branch,Random& random) {
    if (room.width<1 || room.height<1 || room.width>512 || room.height>512 ||
        room.cells.size()!=std::size_t(room.width)*room.height || x<0 || y<0 ||
        std::int64_t(x)+room.width>width_ || std::int64_t(y)+room.height>height_)
        throw std::invalid_argument("room stamp exceeds layout or has invalid dimensions");
    for (int column=0;column<room.width;++column) for (int row=0;row<room.height;++row) {
        const auto& source=room.cells[std::size_t(row)*room.width+column];
        if (!source.kind) continue;
        auto& cell=cells_[std::size_t(y+row)*width_+x+column];
        if (source.kind==6 && cell.kind!=6) doors_.push_back({x+column,y+row});
        cell.kind=source.kind;
        if (cell.kind==1) ++floor_count_;
        if (cell.kind==6) {
            cell.door_flags=source.door_orientation!=1;
            if (random.bounded(2)==0) cell.door_flags|=2;
        }
        if (cell.kind>=7 && cell.kind<=10) {
            if (branch==8 || branch==9) cell.kind=8;
            else if (branch==6) cell.kind=10;
            else if (branch==3) cell.kind=7;
        }
        if (cell.kind==2 && source.wall_marker==4) cell.flags|=0x80;
        if (cell.kind==15) {cell.kind=1;cell.flags|=1;}
        if (room.shop && cell.kind==1) cell.kind=13;
        if (room.zoo && cell.kind==1) cell.kind=14;
        cell.room_id=room_id;
    }
}
}
