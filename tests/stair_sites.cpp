#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("stair-site assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
bool same(const Random::State& a,const Random::State& b) {return a.index==b.index && a.words==b.words;}
DungeonLayout corridor(int width=16) {
    DungeonLayout layout(width,3,std::vector<DungeonCell>(std::size_t(width)*3));
    for (int x=0;x<width;++x) {layout.at(x,1)={2,19,23,0xabcdef,83};layout.at(x,2).kind=1;}
    return layout;
}
int main() {
    try {
        const auto empty=[](Position) {return false;};
        // A narrow corridor forces the original spacing relaxation. Geometry
        // failures do not count toward its 4,001 close-site rejections.
        auto layout=corridor();Random random(5489);StairSiteOptions options;options.count=2;
        std::size_t queries=0,lists=0;
        auto sites=place_stair_sites(layout,random,options,[&](Position p) {check(p.y==2);++queries;return false;},[&](Position) {++lists;return false;});
        check(sites.size()==2 && queries==4002 && lists==12006);
        for (std::size_t i=0;i<sites.size();++i) {
            const auto& s=sites[i];const auto& cell=layout.at(s.position.x,s.position.y);
            check(!s.up && s.link==i && cell.kind==4 && cell.stair_link==i);
            check(cell.room_id==19 && cell.door_flags==23 && cell.flags==0xabcdef);
            check(layout.at(s.position.x,0).kind==0);
        }
        const auto built=build_stair_plan(layout,0,false);check(built.size()==2);
        for (const auto& s:sites) check(layout.at(s.position.x,0).kind==2);
        // Zero stairs consumes no randomness and performs no occupancy query.
        options.count=0;options.depth=1;layout=corridor();const auto before=random.state();
        check(place_stair_sites(layout,random,options,[](Position)->bool {throw std::runtime_error("unexpected query");},empty).empty());
        check(same(before,random.state()) && layout.at(1,1).stair_link==83);
        // Impossible geometry and callback failures roll back both outputs.
        options.count=2;options.max_candidate_draws=1;layout=corridor();bool rejected=false;
        try {place_stair_sites(layout,random,options,empty,empty);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random.state()));
        for (int x=0;x<layout.width();++x) check(layout.at(x,1).kind==2 && layout.at(x,1).stair_link==83);
        options.max_candidate_draws=1000000;rejected=false;
        try {place_stair_sites(layout,random,options,[](Position)->bool {throw std::runtime_error("occupancy unavailable");},empty);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random.state()) && layout.at(1,1).stair_link==83);
        // Final-floor direction depends on the expansion flag, at exact depth.
        options.count=1;options.depth=9;sites=place_stair_sites(layout,random,options,empty,empty);check(sites.size()==1 && sites[0].up);
        layout=corridor();options.expanded_campaign=true;sites=place_stair_sites(layout,random,options,empty,empty);check(sites.size()==2 && sites[0].up && !sites[1].up && sites[1].link==0);
        layout=corridor();options.depth=14;sites=place_stair_sites(layout,random,options,empty,empty);check(sites.size()==1 && sites[0].up);
        std::cout<<"stair-site spacing relaxation, links, phase separation, final-floor policy and atomic failures passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
