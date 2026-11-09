#include "dredmor/generation.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("generation assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    try {
        dredmor::TileGrid empty(10,10,std::vector<std::uint8_t>(100));
        dredmor::TileGrid floor(1,1,{1}),wall(1,1,{2}),door(1,1,{6}),nothing(1,1,{0});
        check(empty.accepts(floor,3,3) && empty.accepts(floor,6,6));
        check(!empty.accepts(floor,2,3) && !empty.accepts(floor,7,6));
        check(!empty.accepts(nothing,2,3) && !empty.accepts(nothing,6,7));
        check(!empty.accepts(floor,std::numeric_limits<int>::max(),3));
        for (int incoming=0;incoming<256;++incoming) for (int existing=0;existing<256;++existing) {
            std::vector<std::uint8_t> cells(100);cells[44]=existing;
            const auto expected=!incoming || !existing || (incoming==existing && (incoming==2 || incoming==6));
            check(dredmor::TileGrid(10,10,cells).accepts(dredmor::TileGrid(1,1,{static_cast<std::uint8_t>(incoming)}),4,4)==expected);
        }
        auto cells=std::vector<std::uint8_t>(100);cells[55]=1;
        dredmor::TileGrid occupied(10,10,cells);
        check(!occupied.accepts(dredmor::TileGrid(2,2,{0,0,0,1}),4,4));
        check(occupied.accepts(dredmor::TileGrid(2,2,{1,0,0,0}),4,4));
        bool rejected=false;try {dredmor::TileGrid(512,512,{0});} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        dredmor::DungeonLayout layout(3,3,std::vector<dredmor::DungeonCell>(9,{0,99,255,0x100}));
        dredmor::RoomStamp stamp{2,2,{{1,0,0},{6,1,0},{2,0,4},{15,0,0}},true,true};
        dredmor::Random random(123);layout.floor_count(UINT32_MAX);
        layout.write_template(stamp,1,1,42,0,random);
        check(layout.floor_count()==0 && layout.new_doors().size()==1);
        check(layout.new_doors()[0].x==2 && layout.new_doors()[0].y==1);
        check(layout.cells()[4].kind==13 && layout.cells()[4].room_id==42);
        check(layout.cells()[7].flags==0x180 && layout.cells()[8].flags==0x101);
        check(layout.cells()[8].kind==13 && layout.cells()[0].room_id==99);
        auto state=random.state();rejected=false;
        try {layout.write_template(stamp,2,2,43,0,random);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && state.words==random.state().words && state.index==random.state().index);
        std::cout<<"room overlap matrix, reserved margins and asymmetric footprints passed\n";
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
