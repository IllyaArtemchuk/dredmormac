#include "dredmor/selection.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        using namespace dredmor;
        std::uint32_t seed;
        while (std::cin>>seed) {
            bool weapon,artifact,available;SpawnDepthContext depth;float deviation;int offset;std::uint32_t bits;std::size_t count;
            std::cin>>weapon>>depth.depth>>depth.alternate>>depth.alternate_depth>>deviation>>artifact>>offset>>available>>bits>>count;
            if (count>10000) throw std::invalid_argument("too many gear entries");
            std::vector<GearCandidate> entries(count);
            for (auto& item:entries) std::cin>>item.weapon_type>>item.craft_output>>item.artifact>>item.artifact_spawned>>item.special>>item.debug;
            GearBuckets buckets(22);
            for (auto& pool:buckets) {
                std::cin>>count;if (count>10000) throw std::invalid_argument("gear pool too large");
                pool.resize(count);for (auto& index:pool) std::cin>>index;
            }
            if (!std::cin) throw std::invalid_argument("incomplete gear selection fixture");
            GameRandom random;random.seed(seed);float spare;std::memcpy(&spare,&bits,4);random.normal.restore({available,spare});
            auto result=weapon?select_weapon(entries,buckets,random,depth,deviation,artifact,offset):select_armour(entries,buckets,random,depth,deviation,offset);
            const auto cache=random.normal.state();std::memcpy(&bits,&cache.spare,4);
            std::cout<<result.index<<' '<<random.libc.state()<<' '<<cache.available<<' '<<bits;
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();
            for (const auto& item:entries) std::cout<<' '<<item.artifact_spawned;
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
