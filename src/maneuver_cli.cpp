#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc==3 && std::string(argv[1])=="--content") {
            const auto content=dredmor::GameContent::load(argv[2]);
            for (const auto& spell:content.spells()) if (spell.maneuvers) {
                for (const auto& t:*spell.maneuvers) {
                    std::cout<<t.id<<' '<<t.width<<' '<<t.height<<' '<<t.affects_player;
                    for (auto cell:t.cells) std::cout<<' '<<unsigned(cell);std::cout<<'\n';
                }
                return 0;
            }
            return 0;
        }
        if (argc!=1) throw std::invalid_argument("expected --content DATA_ROOT or no arguments");
        std::uint32_t seed;int id,anchored,facing;dredmor::Position origin,target;std::size_t count;
        while (std::cin>>seed>>id>>anchored>>facing>>origin.x>>origin.y>>target.x>>target.y>>count) {
            if (count>1000) throw std::invalid_argument("too many maneuver templates");std::vector<dredmor::ManeuverTemplate> entries(count);
            for (auto& t:entries) {
                if (!(std::cin>>t.width>>t.height>>t.id>>t.affects_player) || t.width<1 || t.height<1 || t.width>256 || t.height>256) throw std::invalid_argument("invalid maneuver dimensions");
                t.cells.resize(std::size_t(t.width)*t.height);for (auto& cell:t.cells) {unsigned n;if (!(std::cin>>n) || n>255) throw std::invalid_argument("invalid maneuver cell");cell=n;}
            }
            dredmor::DarwinRandom random(seed);const auto result=dredmor::plan_maneuver(entries,id,anchored,facing,origin,target,random);
            std::cout<<result.accepted<<' '<<result.targets.size();for (auto p:result.targets) std::cout<<' '<<p.x<<' '<<p.y;std::cout<<' '<<random.state()<<'\n';
        }
        return std::cin.eof()?0:1;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
