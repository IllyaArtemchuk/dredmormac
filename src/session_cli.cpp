#include "dredmor/content.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    if (argc<3 || argc>4) {
        std::cerr<<"Usage: dredmor-session DATA_ROOT ACTIONS [DEVELOPER_ENCOUNTER_MONSTER]\nActions: NSWE. and G for pickup\n";return 2;
    }
    try {
        auto content=dredmor::GameContent::load(argv[1]);
        std::vector<int> selected;
        for (const auto& skill:content.skills()) if (selected.size()<7) selected.push_back(skill.id);
        const auto ranks=content.starting_ranks(selected);
        auto world=content.room("Starting Room").instantiate();
        auto hero=dredmor::make_player(ranks);
        dredmor::Session session(std::move(world),hero,0xD0ED,content.tweaks().integer(dredmor::Difficulty::medium,"experience n"));
        session.equipment_profile(ranks,std::make_shared<const std::vector<dredmor::ItemDefinition>>(content.items()));
        if (argc==4) {
            bool spawned=false;
            for (int y=0;y<session.world().height() && !spawned;++y) for (int x=0;x<session.world().width() && !spawned;++x) {
                auto hero_position=session.player().position;
                if (session.world().passable({x,y}) && std::abs(x-hero_position.x)+std::abs(y-hero_position.y)==1) {
                    session.spawn_enemy(content.make_monster(argv[3],{x,y},dredmor::Difficulty::medium));spawned=true;
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
