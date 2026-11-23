#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--sites") {
            std::uint32_t seed;int width,height;StairSiteOptions options;
            while (std::cin>>seed>>width>>height>>options.depth>>options.expanded_campaign>>options.count) {
                if (width<3 || width>512 || height<3 || height>512) throw std::invalid_argument("invalid stair layout");
                std::vector<DungeonCell> cells(std::size_t(width)*height);std::vector<unsigned> occupancy(cells.size());
                for (std::size_t i=0;i<cells.size();++i) {
                    unsigned kind,room,flags,link;auto& cell=cells[i];
                    std::cin>>kind>>room>>flags>>cell.flags>>link>>occupancy[i];
                    if (!std::cin || kind>255 || room>255 || flags>255 || link>255 || occupancy[i]>3) throw std::invalid_argument("invalid stair cell");
                    cell.kind=kind;cell.room_id=room;cell.door_flags=flags;cell.stair_link=link;
                }
                DungeonLayout layout(width,height,std::move(cells));Random random(seed);std::vector<Position> queries;
                const auto sites=place_stair_sites(layout,random,options,[&](Position p) {queries.push_back(p);return occupancy[std::size_t(p.y)*width+p.x]&2;},[&](Position p) {return occupancy[std::size_t(p.y)*width+p.x]&1;});
                std::cout<<sites.size();
                for (const auto& site:sites) std::cout<<' '<<site.position.x<<' '<<site.position.y<<' '<<site.up<<' '<<unsigned(site.link);
                for (const auto& cell:layout.cells()) std::cout<<' '<<unsigned(cell.kind)<<' '<<unsigned(cell.room_id)<<' '<<unsigned(cell.door_flags)<<' '<<cell.flags<<' '<<unsigned(cell.stair_link);
                const auto state=random.state();std::cout<<' '<<state.index;for (auto word:state.words) std::cout<<' '<<word;
                for (int i=0;i<8;++i) std::cout<<' '<<random.u32();
                std::cout<<' '<<queries.size();for (const auto& p:queries) std::cout<<' '<<p.x<<' '<<p.y;
                std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--build") {
            int width,height;std::int32_t depth;bool expanded;
            while (std::cin>>width>>height>>depth>>expanded) {
                if (width<1 || width>512 || height<1 || height>512) throw std::invalid_argument("invalid stair layout");
                std::vector<DungeonCell> cells(std::size_t(width)*height);
                for (auto& cell:cells) {unsigned kind,room,flags,link;std::cin>>kind>>room>>flags>>cell.flags>>link;if (!std::cin || kind>255 || room>255 || flags>255 || link>255) throw std::invalid_argument("invalid stair cell");cell.kind=kind;cell.room_id=room;cell.door_flags=flags;cell.stair_link=link;}
                DungeonLayout layout(width,height,std::move(cells));const auto stairs=build_stair_plan(layout,depth,expanded);std::cout<<stairs.size();
                for (const auto& cell:layout.cells()) std::cout<<' '<<unsigned(cell.kind)<<' '<<unsigned(cell.room_id)<<' '<<unsigned(cell.door_flags)<<' '<<cell.flags<<' '<<unsigned(cell.stair_link);
                for (const auto& stair:stairs) std::cout<<' '<<stair.position.x<<' '<<stair.position.y<<' '<<stair.up<<' '<<unsigned(stair.link);
                std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--destination") {
            bool up;unsigned link;std::int32_t branch,depth;
            while (std::cin>>up>>link>>branch>>depth) {if (link>255) throw std::invalid_argument("invalid stair link");const auto target=stair_destination(up,link,branch,depth);std::cout<<unsigned(target.link)<<' '<<target.branch<<' '<<target.depth<<'\n';}
        } else if (argc==2 && std::string(argv[1])=="--arrival") {
            bool ascending;unsigned link;std::size_t count;
            while (std::cin>>ascending>>link>>count) {
                if (link>255 || count>4096) throw std::invalid_argument("invalid arrival fixture");std::vector<StairPlacement> stairs(count);
                for (auto& stair:stairs) {unsigned value;std::cin>>stair.position.x>>stair.position.y>>stair.up>>value;if (value>255) throw std::invalid_argument("invalid stair link");stair.link=value;}
                const auto point=stair_arrival(stairs,ascending,link);if (point) std::cout<<point->x<<' '<<point->y<<'\n';else std::cout<<"-1 -1\n";
            }
        } else throw std::invalid_argument("Usage: dredmor-stairs --sites | --build | --destination | --arrival");
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
