#include "dredmor/names.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("name assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    try {
        using namespace dredmor;
        auto words=read_room_name_words(parse_xml("<text><Adjective Text='Quiet'/><architecture text='Hall' plural='Halls'/><noun text='Thing' plural='Things'/></text>"));
        check(words.adjectives==std::vector<std::string>({"Quiet"}));
        check(words.architecture==std::vector<std::string>({"Hall"}) && words.architecture_plural==words.architecture);
        check(words.subjects==std::vector<std::string>({"Things"}));
        words.architecture_plural={"Halls"};
        GameRandom random;random.seed(1);auto name=generate_room_name(words,random);
        check(name.display=="The Quiet Halls" && name.subject.empty());
        check(random.libc.state()==282475249 && random.mt.u32()==4005303368U);
        random.seed(0);name=generate_room_name(words,random);
        check(name.display=="The Halls of Things" && name.subject=="Things" && random.mt.u32()==3626093760U);
        words.subjects.clear();random.seed(1);const auto before=random.mt.state();bool rejected=false;
        try {generate_room_name(words,random);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && random.libc.state()==1 && random.mt.state().words==before.words && random.mt.state().index==before.index);
        std::cout<<"room name words, original loader bug, forced suffixes and both stream advances passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
