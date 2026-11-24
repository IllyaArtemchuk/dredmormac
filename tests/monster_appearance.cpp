#include "dredmor/content.hpp"
#include "dredmor/floors.hpp"
#include "dredmor/sprite.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("monster appearance assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
std::string save(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
struct TemporaryRoot {
    std::filesystem::path path;
    ~TemporaryRoot() {std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::invalid_argument("synthetic content fixture root required");
        TemporaryRoot root{std::filesystem::temp_directory_path()/("dredmor-monster-colour-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        std::filesystem::copy(argv[1],root.path,std::filesystem::copy_options::recursive);
        std::ofstream(root.path/"game/monDB.xml")<<R"(<monDB><monster name="Fixture monster"><stats numFig="2"/>
            <idleSprite down="fixture.spr"/><palette name="original.pal"/>
            <monster name="Fixture variant"><palette tint="120"/></monster>
            <monster name="Random variant"><palette tint="-1"/></monster>
            </monster></monDB>)";
        const auto content=GameContent::load(root.path);
        std::ofstream(root.path/"original.pal",std::ios::binary)<<std::string(768,'P');
        const auto colours=read_sprite_palette(root.path/"original.pal");check(colours.front()==80 && colours.back()==80);
        std::ofstream(root.path/"short.pal",std::ios::binary)<<std::string(767,'P');
        bool truncated=false;try {read_sprite_palette(root.path/"short.pal");} catch(const std::invalid_argument&) {truncated=true;}
        check(truncated);
        const auto plain=content.make_monster("Fixture monster",{2,1},Difficulty::medium);
        const auto tinted=content.make_monster("Fixture variant",{3,1},Difficulty::medium);
        check(plain.sprite_palette=="original.pal" && plain.sprite_tint==-1);
        check(tinted.sprite_palette.empty() && tinted.sprite_tint==120 && tinted.health==plain.health);
        bool needs_rng=false;try {content.make_monster("Random variant",{4,1},Difficulty::medium);} catch(const std::invalid_argument&) {needs_rng=true;}
        check(needs_rng);
        DarwinRandom random(83),reference(83);
        monster_creation_ranks(content.monster("Random variant").ranks,content.tweaks().integer(Difficulty::medium,"monster level adder"),reference);
        const auto angle=std::int32_t((std::uint64_t(reference.next())*0x59ffff5dull)>>53);
        const auto generated=content.create_monster("Random variant",{4,1},Difficulty::medium,0,random);
        check(random.state()==reference.state() && generated.sprite_tint==angle && generated.sprite_palette.empty());
        std::istringstream rows("########\n#@.....#\n#......#\n########\n");
        Session session(World::read(rows),make_player({3,2,2}),17);session.spawn_enemy(plain);session.spawn_enemy(tinted);session.spawn_enemy(generated);
        const auto original=save(session);check(original.find("version=\"23\"")!=std::string::npos);
        std::istringstream input(original);auto loaded=Session::load(input);check(save(loaded)==original);
        for (int i=0;i<3;++i) {session.step(Action::wait);loaded.step(Action::wait);check(save(session)==save(loaded));}
        auto rejects=[](const std::string& data) {try {std::istringstream in(data);Session::load(in);return false;} catch(const std::runtime_error&) {return true;}};
        auto invalid=original;auto position=invalid.find("tint=\"120\"");check(position!=std::string::npos);invalid.replace(position,10,"tint=\"360001\"");check(rejects(invalid));
        invalid=original;position=invalid.find("<sprite-colour");const auto end=invalid.find("/>",position)+2;invalid.insert(end,invalid.substr(position,end-position));check(rejects(invalid));
        invalid=original;position=invalid.find("version=\"23\"");invalid.replace(position,12,"version=\"22\"");check(rejects(invalid));
        // The floor cache owns complete actors, including a resolved random
        // tint. Returning or loading cannot draw another colour.
        std::istringstream fresh(original);auto travelling=Session::load(fresh);
        std::istringstream empty_rows("########\n#@.....#\n#......#\n########\n");
        SessionFloor empty(World::read(empty_rows));empty.depth.depth=1;
        auto old=travelling.swap_floor(std::move(empty),{1,1});FloorRegistry registry;registry.add_floor(travelling,std::move(old));
        std::ostringstream combined;registry.save(travelling,combined);std::istringstream combined_input(combined.str());auto restored=FloorRegistry::load(combined_input);
        check(restored.second.cached().size()==1 && restored.second.cached()[0].enemies.size()==3);
        check(restored.second.cached()[0].enemies[0].sprite_palette=="original.pal" && restored.second.cached()[0].enemies[2].sprite_tint==angle);
        std::ostringstream roundtrip;restored.second.save(restored.first,roundtrip);check(roundtrip.str()==combined.str());
        std::cout<<"monster fixed/random colour construction, v23 continuation/guards and cached-floor appearance passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
