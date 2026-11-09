#include "dredmor/names.hpp"
#include <iomanip>
#include <iostream>
#include <iterator>
int main() {
    try {
        const std::string xml{std::istreambuf_iterator<char>(std::cin),std::istreambuf_iterator<char>()};
        const auto words=dredmor::read_room_name_words(dredmor::parse_xml(xml));
        for (const auto* pool:{&words.adjectives,&words.architecture,&words.architecture_plural,&words.subjects}) {
            std::cout<<pool->size();for (const auto& word:*pool) std::cout<<' '<<std::quoted(word);std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
