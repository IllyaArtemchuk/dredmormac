#include "dredmor/stats.hpp"
#include <cstring>
#include <iostream>

int main() {
    std::array<std::int32_t,7> classes,ranks;
    dredmor::PrimaryStats flat_primary,percent_primary;
    dredmor::SecondaryStats flat_secondary,percent_secondary;
    while (std::cin>>classes[0]) {
        for (std::size_t i=1;i<classes.size();++i) if (!(std::cin>>classes[i])) return 1;
        for (auto& value:ranks) if (!(std::cin>>value)) return 1;
        for (auto& value:flat_primary) if (!(std::cin>>value)) return 1;
        for (auto& value:percent_primary) if (!(std::cin>>value)) return 1;
        for (auto& value:flat_secondary) if (!(std::cin>>value)) return 1;
        for (auto& value:percent_secondary) if (!(std::cin>>value)) return 1;
        std::array<std::uint32_t,3> sums{};
        for (std::size_t i=0;i<classes.size();++i) {
            if (classes[i]<0 || classes[i]>2) return 1;
            sums[classes[i]]+=static_cast<std::uint32_t>(ranks[i]);
        }
        dredmor::ClassRanks totals;
        std::memcpy(totals.data(),sums.data(),sizeof(totals));
        auto primary=dredmor::derive_primary(totals,flat_primary,percent_primary);
        auto secondary=dredmor::derive_secondary(primary,flat_secondary,percent_secondary);
        for (auto value:primary) std::cout<<value<<' ';
        for (std::size_t i=0;i<secondary.size();++i) std::cout<<(i?" ":"")<<secondary[i];
        std::cout<<'\n';
    }
    return std::cin.eof()?0:1;
}
