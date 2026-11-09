#include "dredmor/selection.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
int main() {
    try {
        std::string mode;
        while (std::cin>>mode) {
            std::uint32_t seed;std::size_t count;
            if (!(std::cin>>seed>>count) || count<1 || count>10000) throw std::invalid_argument("invalid food selection input");
            std::vector<dredmor::FoodCandidate> pool(count);
            for (auto& item:pool) if (!(std::cin>>item.mana>>item.level>>item.special>>item.debug))
                throw std::invalid_argument("invalid food candidate");
            dredmor::DarwinRandom random(seed);
            std::size_t index;
            if (mode=="edible") index=dredmor::select_edible(pool,random);
            else if (mode=="booze") index=dredmor::select_booze(pool,random);
            else throw std::invalid_argument("invalid food selection mode");
            std::cout<<index<<' '<<random.state()<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
