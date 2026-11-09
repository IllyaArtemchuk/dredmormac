#include "dredmor/random.hpp"
#include <cstdlib>
#include <iostream>
int main() {
    for (auto seed:{0u,1u,2u,5489u,0x7ffffffeu,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu}) {
        std::srand(seed);dredmor::DarwinRandom replacement(seed);
        for (int i=0;i<4096;++i) if (std::rand()!=replacement.next()) {
            std::cerr<<"Darwin libc rand mismatch for seed "<<seed<<" at draw "<<i<<'\n';return 1;
        }
    }
    std::cout<<"36,864 actual Darwin libc rand outputs match\n";
}
