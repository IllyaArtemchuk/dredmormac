#include "dredmor/selection.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        dredmor::ClassRanks ranks;int category,always;
        while (std::cin>>ranks[0]) {
            if (!(std::cin>>ranks[1]>>ranks[2]>>category>>always)) throw std::invalid_argument("invalid loadout input");
            std::cout<<dredmor::loadout_allowed(ranks,category,always!=0)<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
