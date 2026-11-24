#include "dredmor/secret_doors.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("secret-door assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
DungeonLayout floor() {
    DungeonLayout layout(32,24,std::vector<DungeonCell>(32*24));
    for (int y=0;y<24;++y) for (int x=0;x<32;++x) layout.at(x,y)={std::uint8_t(x%10<4?0:x%10==4?2:1),0,23,0xabcdef,83};
    return layout;
}
bool same(const DungeonLayout& a,const DungeonLayout& b) {
    for (std::size_t i=0;i<a.cells().size();++i) {const auto& x=a.cells()[i];const auto& y=b.cells()[i];if (x.kind!=y.kind || x.room_id!=y.room_id || x.door_flags!=y.door_flags || x.flags!=y.flags || x.stair_link!=y.stair_link) return false;}
    return true;
}
int main() {
    try {
        auto layout=floor();GameRandom random;random.seed(1);std::vector<bool> occupied(layout.cells().size());unsigned calls=0;
        auto doors=generate_secret_door_plan(layout,random,{false},occupied,[&](GameRandom& stream)->std::optional<InventoryItem> {
            ++calls;stream.mt.u32();stream.libc.next();InventoryItem item;item.instance=calls;return item;
        });
        check(doors.size()==3 && calls==3);
        for (std::size_t i=0;i<doors.size();++i) {
            const auto& d=doors[i];check(d.treasure && d.treasure->instance==i+1 && d.lever);
            check(layout.at(d.wall.x,d.wall.y).kind==2 && layout.at(d.pocket.x,d.pocket.y).kind==1 && layout.at(d.pocket.x,d.pocket.y+1).kind==1);
            check(layout.at(d.lever->x,d.lever->y).kind==1);
            for (std::size_t j=0;j<i;++j) check(d.lever->x!=doors[j].lever->x || d.lever->y!=doors[j].lever->y);
        }
        for (const auto& cell:layout.cells()) check(cell.room_id==0 && cell.door_flags==23 && cell.flags==0xabcdef && cell.stair_link==83);
        const auto absent=[](GameRandom&)->std::optional<InventoryItem> {throw std::runtime_error("unexpected treasure request");};
        // A blocked room uses one rejection per draw; eligible walls without
        // an oriented pocket use two. This changes all later libc draws.
        for (bool blocked:{false,true}) {
            layout=DungeonLayout(32,24,std::vector<DungeonCell>(32*24));for (int y=0;y<24;++y) for (int x=0;x<32;++x) layout.at(x,y).kind=2;
            random.seed(1);DarwinRandom expected(1);for (int i=0;i<(blocked?300:150);++i) expected.next();
            check(generate_secret_door_plan(layout,random,{blocked},occupied,absent).empty() && random.libc.state()==expected.state());
        }
        // A fully occupied floor still gets pockets and treasure requests,
        // but no lever can be placed. Occupancy is not part of site selection.
        layout=floor();random.seed(1);occupied.assign(occupied.size(),true);
        doors=generate_secret_door_plan(layout,random,{false},occupied,[](GameRandom&)->std::optional<InventoryItem> {return std::nullopt;});
        check(doors.size()==3);for (const auto& door:doors) check(!door.lever && !door.treasure);
        // The provisional random state includes the normal-sample cache.
        layout=floor();const auto before=layout;random.seed(1);random.normal.restore({true,7.25f});const auto state=random.mt.state();occupied.assign(occupied.size(),false);bool rejected=false;
        try {generate_secret_door_plan(layout,random,{false},occupied,[](GameRandom& stream)->std::optional<InventoryItem> {stream.mt.u32();stream.libc.next();stream.normal.restore({false,0});throw std::runtime_error("treasure failure");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(layout,before) && random.libc.state()==1 && random.mt.state().words==state.words && random.mt.state().index==state.index && random.normal.state().available && random.normal.state().spare==7.25f);
        std::cout<<"secret pockets/levers, rejection budgets, occupancy, metadata and atomic RNG/cache rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
