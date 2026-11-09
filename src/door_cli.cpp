#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        int width,height;
        while (std::cin>>width>>height) {
            if (width<1 || height<1 || width>512 || height>512) throw std::invalid_argument("invalid door fixture dimensions");
            std::vector<dredmor::DungeonCell> cells(std::size_t(width)*height);
            for (auto& cell:cells) {
                unsigned kind,id,doors;std::cin>>kind>>id>>doors>>cell.flags;
                if (!std::cin || kind>255 || id>255 || doors>255) throw std::invalid_argument("invalid door cell");
                cell.kind=kind;cell.room_id=id;cell.door_flags=doors;
            }
            dredmor::DungeonLayout layout(width,height,std::move(cells));const auto plan=dredmor::build_door_plan(layout);
            std::cout<<plan.doors.size()<<' '<<plan.repaired.size();
            for (auto position:plan.repaired) std::cout<<' '<<position.x<<' '<<position.y;
            for (const auto& cell:layout.cells()) std::cout<<' '<<unsigned(cell.kind)<<' '<<unsigned(cell.room_id)<<' '<<unsigned(cell.door_flags)<<' '<<cell.flags;
            for (const auto& door:plan.doors) {
                std::cout<<' '<<door.center.x<<' '<<door.center.y<<' '<<door.horizontal<<' '<<door.locked<<' '<<door.open;
                for (auto part:door.parts) std::cout<<' '<<part.x<<' '<<part.y;
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
