#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("content assertion at line "+std::to_string(__LINE__)); } while(false)
int main(int argc,char** argv) {
    try {
        check(argc==2);auto content=dredmor::GameContent::load(argv[1]);
        check(content.items().size()==1 && content.monsters().size()==2 && content.skills().size()==7);
        auto world=content.room("Starting Room").instantiate();
        check(world.width()==7 && world.height()==5 && !world.passable({3,2}));
        check(world.player().x==3 && world.player().y==1);
        check(content.starting_ranks({0,1,2,3,4,5,6})==dredmor::ClassRanks({3,2,2}));
        auto enemy=content.make_monster("Fixture variant",{2,1},dredmor::Difficulty::medium);
        check(enemy.health==11 && enemy.damage[0]==1 && enemy.experience_reward==10);
        check(content.tweaks().integer(dredmor::Difficulty::medium,"monster additional HP per level")==0);
        bool rejected=false;try {content.starting_ranks({0,0,1,2,3,4,5});} catch (const std::invalid_argument&) {rejected=true;}
        check(rejected);
        std::cout<<"content-to-session definitions, selected classes, variant stats and room collision passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
