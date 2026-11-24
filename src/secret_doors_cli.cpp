#include "dredmor/secret_doors.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::uint32_t seed;int width,height;std::size_t rooms;
        while (std::cin>>seed>>width>>height>>rooms) {
            if (width<6 || width>512 || height<6 || height>512 || !rooms || rooms>256) throw std::invalid_argument("invalid secret-door fixture");
            std::vector<bool> blocked(rooms);for (std::size_t i=0;i<rooms;++i) {unsigned value;std::cin>>value;blocked[i]=value!=0;}
            struct Roll {unsigned mt,libc,present;};std::array<Roll,3> rolls;
            for (auto& r:rolls) {std::cin>>r.mt>>r.libc>>r.present;if (r.mt>3 || r.libc>3 || r.present>1) throw std::invalid_argument("invalid treasure script");}
            std::vector<DungeonCell> cells(std::size_t(width)*height);std::vector<bool> occupied(cells.size());
            for (std::size_t i=0;i<cells.size();++i) {
                auto& c=cells[i];unsigned kind,room,door,link,object;
                std::cin>>kind>>room>>door>>c.flags>>link>>object;
                if (!std::cin || kind>255 || room>=rooms || door>255 || link>255 || object>1) throw std::invalid_argument("invalid secret-door cell");
                c.kind=kind;c.room_id=room;c.door_flags=door;c.stair_link=link;occupied[i]=object!=0;
            }
            DungeonLayout layout(width,height,std::move(cells));GameRandom random;random.seed(seed);std::size_t calls=0;
            const auto doors=generate_secret_door_plan(layout,random,blocked,occupied,[&](GameRandom& stream)->std::optional<InventoryItem> {
                const auto r=rolls.at(calls++);for (unsigned i=0;i<r.mt;++i) stream.mt.u32();for (unsigned i=0;i<r.libc;++i) stream.libc.next();
                if (!r.present) return std::nullopt;InventoryItem item;item.instance=calls;return item;
            });
            std::cout<<doors.size();
            for (const auto& door:doors) std::cout<<' '<<door.wall.x<<' '<<door.wall.y<<' '<<door.pocket.x<<' '<<door.pocket.y<<' '<<bool(door.treasure)<<' '<<(door.lever?door.lever->x:-1)<<' '<<(door.lever?door.lever->y:-1);
            for (const auto& c:layout.cells()) std::cout<<' '<<unsigned(c.kind)<<' '<<unsigned(c.room_id)<<' '<<unsigned(c.door_flags)<<' '<<c.flags<<' '<<unsigned(c.stair_link);
            std::cout<<' '<<random.libc.state();const auto state=random.mt.state();std::cout<<' '<<state.index;for (auto word:state.words) std::cout<<' '<<word;
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
