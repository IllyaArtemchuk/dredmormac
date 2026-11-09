#include "dredmor/generation.hpp"
#include <iostream>
#include <stdexcept>
namespace {
std::uint32_t number() {
    std::uint64_t value;
    if (!(std::cin>>value) || value>UINT32_MAX) throw std::invalid_argument("invalid stamp input integer");
    return static_cast<std::uint32_t>(value);
}
std::uint8_t byte() {
    const auto value=number();if (value>255) throw std::invalid_argument("invalid stamp byte");
    return static_cast<std::uint8_t>(value);
}
}
int main() {
    try {
        int width,height,rw,rh,x,y,branch;
        while (std::cin>>width) {
            if (!(std::cin>>height>>rw>>rh>>x>>y>>branch) || width<1 || height<1 || rw<1 || rh<1 ||
                width>512 || height>512 || rw>512 || rh>512) throw std::invalid_argument("invalid stamp dimensions");
            const auto room_id=byte();const auto seed=number(),count=number(),shop=number(),zoo=number();
            if (shop>1 || zoo>1) throw std::invalid_argument("invalid room flags");
            std::vector<dredmor::DungeonCell> cells(std::size_t(width)*height);
            for (auto& cell:cells) {cell.kind=byte();cell.room_id=byte();cell.door_flags=byte();cell.flags=number();}
            dredmor::RoomStamp room;room.width=rw;room.height=rh;room.shop=shop;room.zoo=zoo;
            room.cells.resize(std::size_t(rw)*rh);
            for (auto& cell:room.cells) {cell.kind=byte();cell.door_orientation=byte();cell.wall_marker=byte();}
            dredmor::DungeonLayout layout(width,height,std::move(cells));dredmor::Random random(seed);
            layout.floor_count(count);layout.write_template(room,x,y,room_id,branch,random);
            std::cout<<layout.floor_count()<<' '<<layout.new_doors().size();
            for (auto door:layout.new_doors()) std::cout<<' '<<door.x<<' '<<door.y;
            for (const auto& cell:layout.cells()) std::cout<<' '<<unsigned(cell.kind)<<' '<<unsigned(cell.room_id)
                <<' '<<unsigned(cell.door_flags)<<' '<<cell.flags;
            for (int i=0;i<8;++i) std::cout<<' '<<random.u32();
            std::cout<<'\n';
        }
        if (!std::cin.eof()) throw std::invalid_argument("invalid stamp input");
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
