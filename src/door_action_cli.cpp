#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        bool horizontal,open,locked,remote;unsigned selected,mask;
        while (std::cin>>horizontal>>open>>locked>>selected>>remote>>mask) {
            dredmor::DungeonLayout layout(9,9,std::vector<dredmor::DungeonCell>(81));
            dredmor::DoorAssembly door{{4,4},horizontal,locked,open,{}};
            for (std::size_t i=0;i<3;++i) {
                door.parts[i]={horizontal?int(3+i):4,horizontal?4:int(3+i)};
                unsigned flags;
                if (!(std::cin>>flags) || flags>255) throw std::invalid_argument("invalid door flags");
                const auto p=door.parts[i];layout.at(p.x,p.y).door_flags=flags;
            }
            std::vector<unsigned> checks;
            const auto occupied=[&](dredmor::Position position) {
                for (unsigned i=0;i<3;++i) if (door.parts[i].x==position.x && door.parts[i].y==position.y) {checks.push_back(i);return (mask&(1U<<i))!=0;}
                throw std::runtime_error("unexpected obstruction query");
            };
            const auto result=remote?dredmor::activate_remote_door(layout,door,selected,occupied):dredmor::toggle_door(layout,door,selected,occupied);
            std::cout<<unsigned(result.outcome)<<' '<<result.handled<<' '<<door.open;
            for (auto position:door.parts) std::cout<<' '<<unsigned(layout.at(position.x,position.y).door_flags);
            std::cout<<' '<<checks.size();for (auto check:checks) std::cout<<' '<<check;
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
