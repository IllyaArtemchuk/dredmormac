#include "dredmor/generation.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        int mode,width,height;std::uint32_t seed;Position center,minimum,maximum;
        while (std::cin>>mode>>seed>>width>>height>>center.x>>center.y>>minimum.x>>minimum.y>>maximum.x>>maximum.y) {
            if (mode<0 || mode>1 || width<1 || height<1 || width>512 || height>512) throw std::invalid_argument("invalid spawn location fixture");
            std::vector<DungeonCell> cells(std::size_t(width)*height);std::vector<bool> occupied(cells.size());
            for (std::size_t i=0;i<cells.size();++i) {unsigned kind,object;std::cin>>kind>>object;if (!std::cin || kind>255 || object>1) throw std::invalid_argument("invalid spawn cell");cells[i].kind=kind;occupied[i]=object;}
            DungeonLayout map(width,height,std::move(cells));DarwinRandom random(seed);
            const auto result=mode?find_good_location_near(map,center,occupied,random):find_good_location_bounded(map,minimum,maximum,occupied,random);
            std::cout<<result.found<<' '<<result.position.x<<' '<<result.position.y<<' '<<random.state()<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
