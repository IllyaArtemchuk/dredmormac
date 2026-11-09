#include "dredmor/dungeon.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("dungeon assertion at line "+std::to_string(__LINE__)); } while(false)

namespace {
std::vector<dredmor::RoomStamp> pool(int size) {
    std::vector<dredmor::RoomStamp> rooms(9);
    for (std::size_t i=0;i<rooms.size();++i) {
        auto& room=rooms[i];room.width=room.height=size;
        for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
            std::uint8_t kind=x==0 || y==0 || x==size-1 || y==size-1?2:1;
            if (((x==0 || x==size-1) && y==size/2) || ((y==0 || y==size-1) && x==size/2)) kind=6;
            if (i==5 && x==3 && y==3) kind=15;
            if (i==6 && x==4 && y==4) kind=9;
            room.cells.push_back({kind,std::uint8_t(kind==6 && (y==0 || y==size-1)),std::uint8_t(i==7 && kind==2?4:0)});
        }
        if (i==0 || i==2) room.fixed_name="Fixed";
        room.shop=i==2;room.zoo=i==3;room.rules.quest=i==0;room.rules.unique=i==4;room.rules.diggle_god=i==5;
        if (i==8) room.rules.minimum_level=25;
    }
    return rooms;
}
bool same(const dredmor::GameRandom& a,const dredmor::GameRandom& b) {
    return a.libc.state()==b.libc.state() && a.mt.state().words==b.mt.state().words && a.mt.state().index==b.mt.state().index;
}
}
int main() {
    try {
        using namespace dredmor;
        RoomNameWords words{{"A0","A1","A2"},{"S0","S1","S2"},{"P0","P1","P2"},{"N0","N1","N2"}};
        GeometryOptions options;options.width=options.height=64;options.short_game=true;options.max_shops=2;
        GameRandom random;random.seed(0);
        auto result=generate_level_geometry(pool(11),words,random,options);
        // Golden values come from the original 0x195bac through BuildDoors,
        // with these synthetic inputs and supplied library/config boundaries.
        check(result.accepted && result.attempts==1 && result.rooms.size()==20 && result.shops==1);
        check(result.layout.floor_count()==1618 && result.initial_player->x==33 && result.initial_player->y==35);
        check(random.libc.state()==2116969294U && random.mt.u32()==2393880710U);
        std::size_t edges=0;unsigned unique=0,zoo=0,god=0;
        for (std::size_t i=0;i<result.rooms.size();++i) {
            const auto& room=result.rooms[i];edges+=room.neighbors.size();
            unique+=room.template_index==4;zoo+=room.template_index==3;god+=room.template_index==5;
            check(room.template_index!=8 && (i==0 || room.template_index!=0));
            for (auto neighbor:room.neighbors) {
                check(neighbor<result.rooms.size());bool reciprocal=false;
                for (auto back:result.rooms[neighbor].neighbors) reciprocal|=back==i;
                check(reciprocal);
            }
        }
        check(edges==2*(result.rooms.size()-1) && unique<=1 && zoo<=1 && god<=1);
        options.width=options.height=48;options.depth=3;options.starters={1,4};random.seed(5489);
        result=generate_level_geometry(pool(9),words,random,options);
        check(result.accepted && result.attempts==2 && result.rooms.size()==20 && result.shops==0 && !result.initial_player);
        check(result.layout.floor_count()==972 && random.libc.state()==1956328826U && random.mt.u32()==2088947962U);
        check(result.layout.at(38,14).kind==0 && result.layout.at(38,14).flags==128);
        random.seed(5489);auto before=random;options.max_attempts=1;bool rejected=false;
        try {generate_level_geometry(pool(9),words,random,options);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random));
        result=generate_geometry_attempt(pool(9),words,random,options);
        check(!result.accepted && !same(before,random));
        random=before;options.max_candidate_draws=1;rejected=false;
        try {generate_geometry_attempt(pool(9),words,random,options);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(before,random));
        options.starters={99};rejected=false;
        try {generate_geometry_attempt(pool(9),words,random,options);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && same(before,random));
        std::cout<<"original geometry goldens, retry flags, graph constraints and atomic guard failures passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
