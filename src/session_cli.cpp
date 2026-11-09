#include "dredmor/content.hpp"
#include "dredmor/character.hpp"
#include <fstream>
#include <iostream>
#include <charconv>
#include <sstream>
namespace {
template<class T> T integer(std::string_view text) {
    T value;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if (parsed.ec!=std::errc() || parsed.ptr!=text.data()+text.size()) throw std::invalid_argument("invalid CLI integer");
    return value;
}
}
int main(int argc,char** argv) {
    if (argc<3) {
        std::cerr<<"Usage: dredmor-session DATA_ROOT ACTIONS [DEVELOPER_ENCOUNTER_MONSTER] [--skills ID,ID,...] [--seed UINT32]\nActions: NSWE. and G for pickup\n";return 2;
    }
    try {
        auto content=dredmor::GameContent::load(argv[1]);
        std::vector<int> selected;
        std::string encounter;std::uint32_t seed=0xD0ED;bool explicit_skills=false;
        for (int i=3;i<argc;++i) {
            const std::string option=argv[i];
            if (option=="--skills") {
                if (++i==argc || explicit_skills) throw std::invalid_argument("missing or repeated selected skills");
                explicit_skills=true;std::istringstream input(argv[i]);std::string part;
                while (std::getline(input,part,',')) selected.push_back(integer<int>(part));
            } else if (option=="--seed") {
                if (++i==argc) throw std::invalid_argument("missing seed");seed=integer<std::uint32_t>(argv[i]);
            } else if (!option.empty() && option[0]!='-' && encounter.empty()) encounter=option;
            else throw std::invalid_argument("unknown or repeated encounter/option");
        }
        if (!explicit_skills) for (const auto& skill:content.skills()) if (selected.size()<7) selected.push_back(skill.id);
        auto world=content.room("Starting Room").instantiate();
        auto session=dredmor::create_character(content,std::move(world),selected,seed);
        if (!encounter.empty()) {
            bool spawned=false;
            for (int y=0;y<session.world().height() && !spawned;++y) for (int x=0;x<session.world().width() && !spawned;++x) {
                auto hero_position=session.player().position;
                if (session.world().passable({x,y}) && std::abs(x-hero_position.x)+std::abs(y-hero_position.y)==1) {
                    session.spawn_enemy(content.make_monster(encounter,{x,y},dredmor::Difficulty::medium));spawned=true;
                }
            }
            if (!spawned) throw std::runtime_error("no adjacent developer encounter spawn");
        }
        std::cout<<session.snapshot()<<'\n';
        for (char action:std::string(argv[2])) {
            if (action=='G') session.pickup();else session.step(dredmor::parse_action(action));
            std::cout<<session.snapshot()<<'\n';
        }
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
