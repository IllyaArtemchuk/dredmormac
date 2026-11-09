#include "dredmor/stats.hpp"
#include <iostream>
int main() {
    dredmor::MonsterStatInput input;
    while (std::cin>>input.ranks[0]) {
        for (std::size_t i=1;i<3;++i) if (!(std::cin>>input.ranks[i])) return 1;
        for (auto* values:{&input.definition_primary,&input.primary_flat,&input.primary_percent})
            for (auto& value:*values) if (!(std::cin>>value)) return 1;
        for (auto* values:{&input.definition_secondary,&input.secondary_flat,&input.secondary_percent})
            for (auto& value:*values) if (!(std::cin>>value)) return 1;
        if (!(std::cin>>input.dungeon_level>>input.health_scale>>input.health_percent_per_level>>input.health_per_level)) return 1;
        auto result=dredmor::derive_monster(input);
        for (auto value:result.primary) std::cout<<value<<' ';
        for (std::size_t i=0;i<24;++i) std::cout<<(i?" ":"")<<result.secondary[i];
        std::cout<<'\n';
    }
    return std::cin.eof()?0:1;
}
