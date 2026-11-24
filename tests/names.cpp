#include "dredmor/names.hpp"
#include "dredmor/artifact.hpp"
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
        ArtifactWords monster_words;monster_words.starts={"S"};monster_words.phonemes={"p"};monster_words.titles={"Title"};
        random.seed(7);const auto failed_before=random.mt.state();bool missing_plural=false;
        try {generate_monster_name(monster_words,random);} catch(const std::invalid_argument&) {missing_plural=true;}
        check(missing_plural && random.libc.state()==7 && random.mt.state().index==failed_before.index && random.mt.state().words==failed_before.words);
        // The alternate platoon branch uses adjective/plural index0 when its
        // source title/noun counts are empty, without those two libc draws.
        ArtifactWords alternate;alternate.adjectives={"Adjective"};alternate.plurals={"Plural"};random.seed(0);
        check(generate_platoon_name(alternate,random).find("Adjective Plural")!=std::string::npos);
        random.seed(1);const auto missing_before=random.mt.state();bool missing_title=false;
        try {generate_platoon_name(alternate,random);} catch(const std::invalid_argument&) {missing_title=true;}
        check(missing_title && random.libc.state()==1 && random.mt.state().words==missing_before.words && random.mt.state().index==missing_before.index);
        std::cout<<"room name words, original loader bug, forced suffixes and both stream advances passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
