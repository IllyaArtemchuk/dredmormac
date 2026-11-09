#include "dredmor/stats.hpp"
#include <iostream>
int main() {
    dredmor::PrimaryStats primary;
    dredmor::SecondaryStats flat,percent;
    while (std::cin>>primary[0]) {
        for (std::size_t i=1;i<primary.size();++i) if (!(std::cin>>primary[i])) return 1;
        for (auto& n:flat) if (!(std::cin>>n)) return 1;
        for (auto& n:percent) if (!(std::cin>>n)) return 1;
        auto derived=dredmor::derive_secondary(primary,flat,percent);
        for (std::size_t i=0;i<derived.size();++i) std::cout<<(i?" ":"")<<derived[i];
        std::cout<<'\n';
    }
    return std::cin.eof()?0:1;
}
