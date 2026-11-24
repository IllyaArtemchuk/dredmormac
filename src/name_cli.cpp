#include "dredmor/names.hpp"
#include "dredmor/artifact.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc==2 && (std::string(argv[1])=="--monster" || std::string(argv[1])=="--platoon")) {
            std::uint32_t seed;
            while (std::cin>>seed) {
                dredmor::ArtifactWords words;
                for (auto* pool:{&words.starts,&words.phonemes,&words.titles,&words.nouns,&words.plurals,&words.adjectives}) {
                    std::size_t count;std::cin>>count;if (count>1000) throw std::invalid_argument("invalid name word count");
                    for (std::size_t i=0;i<count;++i) {std::string text;std::cin>>std::quoted(text);pool->push_back(text);}
                }
                if (!std::cin) throw std::runtime_error("truncated name fixture");
                dredmor::GameRandom random;random.seed(seed);const auto name=std::string(argv[1])=="--monster"?dredmor::generate_monster_name(words,random):dredmor::generate_platoon_name(words,random);
                std::cout<<std::quoted(name)<<' '<<random.libc.state();const auto mt=random.mt.state();std::cout<<' '<<mt.index;for (auto value:mt.words) std::cout<<' '<<value;
                for (unsigned i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
            }
            return 0;
        }
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
