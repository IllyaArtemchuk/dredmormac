#include "dredmor/artifact.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("artifact assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string state(GameRandom random) {
    std::ostringstream out;out<<random.libc.state()<<' '<<random.normal.state().available<<' '<<random.normal.state().spare;
    for (int i=0;i<16;++i) out<<' '<<random.mt.u32();return out.str();
}
int main() {
    try {
        const auto words=read_artifact_words(parse_xml("<text><StartPhoneme Text='A'/><phoneme text='b'/><title text='Keeper'/><noun text='Thing' plural='Things'/><adjective text='Bright'/><quality text='polished'/><material text='tin'/><decor text='dots'/></text>"));
        GameRandom random;random.seed(123);ArtifactRequest request;request.quality=10;request.base_description="Base";
        // Expected values from the original full routine over these synthetic pools.
        const auto artifact=create_artifact(words,request,random);
        check(artifact.name=="Ab, the Bright Thing" && artifact.quality==10);
        check(artifact.description=="It is studded with polished tin and is decorated with polished dots and polished dots.\n\n");
        check(artifact.buff_description=="Base "+artifact.description);
        SecondaryStats secondary{};secondary[13]=3;Damage damage{},resist{};damage[12]=damage[14]=2;resist[1]=2;resist[4]=1;
        check(artifact.modifiers.primary==PrimaryStats{} && artifact.modifiers.secondary==secondary);
        check(artifact.modifiers.damage==damage && artifact.modifiers.resistance==resist && artifact.weapon_damage==damage);
        check(random.libc.state()==1184067167 && random.mt.u32()==2694306016U);
        GameRandom generated,fixed;generated.seed(123);fixed.seed(123);
        create_artifact(words,request,generated);request.fixed_name="Fixed fixture name";const auto named=create_artifact(words,request,fixed);
        check(named.name==*request.fixed_name && state(generated)==state(fixed)); // Generation still occurs before fixed-name replacement.
        request.kind=ItemKind::armour;fixed.seed(123);const auto armour=create_artifact(words,request,fixed);
        check(armour.weapon_damage==Damage{} && armour.modifiers.damage==damage && state(generated)==state(fixed));
        const auto before=state(fixed);bool rejected=false;
        try {create_artifact(words,request,fixed,0);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && state(fixed)==before);
        auto incomplete=words;incomplete.materials.clear();rejected=false;
        try {create_artifact(incomplete,request,fixed);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && state(fixed)==before);
        request.quality=-1;const auto empty=create_artifact(words,request,fixed);
        check(empty.quality==-1 && empty.modifiers.primary==PrimaryStats{} && empty.modifiers.secondary==SecondaryStats{} && empty.modifiers.damage==Damage{});
        std::cout<<"original artifact fixture, fixed-name RNG, armour damage distinction and atomic resource guards passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
