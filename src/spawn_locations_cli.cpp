#include "dredmor/spawn_locations.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t seed;int width,height,special;dredmor::Position point;
        while (std::cin>>seed>>width>>height>>point.x>>point.y>>special) {
            if (width<1 || height<1 || width>32 || height>32) throw std::invalid_argument("invalid spawn fixture dimensions");
            std::size_t count;std::cin>>count;if (count>4096) throw std::invalid_argument("too many special spawn sites");
            std::vector<dredmor::Position> sites(count);for (auto& site:sites) std::cin>>site.x>>site.y;
            std::vector<dredmor::DungeonCell> cells(std::size_t(width)*height);std::vector<bool> occupied(cells.size());
            for (std::size_t i=0;i<cells.size();++i) {unsigned kind,full;std::cin>>kind>>full;if (kind>255 || full>1) throw std::invalid_argument("invalid spawn cell");cells[i].kind=kind;occupied[i]=full;}
            if (!std::cin) throw std::runtime_error("truncated spawn position fixture");
            dredmor::DarwinRandom random(seed);const auto result=dredmor::select_monster_spawn_position(dredmor::DungeonLayout(width,height,std::move(cells)),occupied,sites,point,special,random);
            std::cout<<bool(result)<<' '<<(result?result->x:0)<<' '<<(result?result->y:0)<<' '<<random.state()<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
