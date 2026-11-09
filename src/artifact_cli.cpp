#include "dredmor/artifact.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "dredmor/content.hpp"
#include <iterator>
std::string hex(const std::string& value) {
    if (value.empty()) return "-";
    const char* digits="0123456789abcdef";std::string result;
    for (unsigned char c:value) {result+=digits[c>>4];result+=digits[c&15];}return result;
}
void print_words(const dredmor::ArtifactWords& words) {
    for (const auto* pool:{&words.starts,&words.phonemes,&words.titles,&words.nouns,&words.plurals,&words.adjectives,&words.qualities,&words.materials,&words.decor}) {
        std::cout<<pool->size();for (const auto& word:*pool) std::cout<<' '<<std::quoted(word);std::cout<<'\n';
    }
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--words") {
            const std::string xml{std::istreambuf_iterator<char>(std::cin),std::istreambuf_iterator<char>()};
            print_words(dredmor::read_artifact_words(dredmor::parse_xml(xml)));return 0;
        }
        if (argc==3 && std::string(argv[1])=="--content") {print_words(dredmor::GameContent::load(argv[2]).artifact_words());return 0;}
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::ArtifactRequest request;unsigned kind;bool fixed;std::string name;dredmor::ArtifactWords words;
            std::cin>>request.quality>>kind>>fixed>>std::quoted(name)>>std::quoted(request.base_description);
            request.kind=static_cast<dredmor::ItemKind>(kind);if (fixed) request.fixed_name=name;
            for (auto* pool:{&words.starts,&words.phonemes,&words.titles,&words.nouns,&words.plurals,&words.adjectives,&words.qualities,&words.materials,&words.decor}) {
                std::size_t count;std::cin>>count;if (count>10000) throw std::invalid_argument("too many artifact words");
                pool->resize(count);for (auto& word:*pool) std::cin>>std::quoted(word);
            }
            if (!std::cin) throw std::invalid_argument("incomplete artifact fixture");
            dredmor::GameRandom random;random.seed(seed);const auto artifact=dredmor::create_artifact(words,request,random);
            std::cout<<hex(artifact.name)<<' '<<hex(artifact.description)<<' '<<hex(artifact.buff_description)<<' '<<artifact.quality;
            for (auto n:artifact.modifiers.primary) std::cout<<' '<<n;
            for (auto n:artifact.modifiers.secondary) std::cout<<' '<<n;
            for (auto n:artifact.modifiers.damage) std::cout<<' '<<n;
            for (auto n:artifact.modifiers.resistance) std::cout<<' '<<n;
            for (auto n:artifact.weapon_damage) std::cout<<' '<<n;
            std::cout<<' '<<random.libc.state();for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
