#include "dredmor/names.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::RoomNameWords words;
            const char* labels[]={"A","S","P","N"};int number=0;
            for (auto* pool:{&words.adjectives,&words.architecture,&words.architecture_plural,&words.subjects}) {
                std::size_t count;
                if (!(std::cin>>count) || !count || count>1000) throw std::invalid_argument("invalid word pool size");
                for (std::size_t i=0;i<count;++i) pool->push_back(std::string(labels[number])+std::to_string(i));
                ++number;
            }
            dredmor::GameRandom random;random.seed(seed);const auto name=dredmor::generate_room_name(words,random);
            std::cout<<std::quoted(name.display)<<' '<<std::quoted(name.subject)<<' '<<random.libc.state();
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
