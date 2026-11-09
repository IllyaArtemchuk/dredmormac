#include "dredmor/selection.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::SpawnDepthContext depth;std::uint32_t deviation_bits,spare_bits;unsigned available;std::size_t count;
            if (!(std::cin>>depth.depth>>depth.alternate>>depth.alternate_depth>>deviation_bits>>available>>spare_bits>>count) || available>1 || !count || count>10000)
                throw std::invalid_argument("invalid trap selection fixture");
            std::vector<dredmor::TrapCandidate> pool(count);
            for (auto& item:pool) if (!(std::cin>>item.level>>item.special>>item.debug)) throw std::invalid_argument("invalid trap candidate");
            float deviation,spare;std::memcpy(&deviation,&deviation_bits,4);std::memcpy(&spare,&spare_bits,4);
            dredmor::GameRandom random;random.seed(seed);random.normal.restore({available!=0,spare});
            const auto result=dredmor::select_trap(pool,random,depth,deviation);const auto state=random.normal.state();
            std::memcpy(&spare_bits,&state.spare,4);
            std::cout<<result.index<<' '<<result.target_level<<' '<<result.attempts<<' '<<random.libc.state()<<' '<<state.available<<' '<<spare_bits;
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
