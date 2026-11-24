#include "dredmor/spawn_locations.hpp"
#include <iostream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("spawn position assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        std::vector<DungeonCell> cells(6);for (auto& cell:cells) cell.kind=2;cells[4].kind=14;
        DungeonLayout layout(3,2,cells);std::vector<bool> occupied(6,false);DarwinRandom random(7);
        auto chosen=select_monster_spawn_position(layout,occupied,{}, {-1,99},0,random);check(chosen && chosen->x==1 && chosen->y==1);
        const auto fixed_rng=random.state();auto fixed=select_monster_spawn_position(layout,{}, {},{-2,99},0,random);check(fixed && fixed->x==-2 && fixed->y==99 && random.state()==fixed_rng);
        auto expected=random;expected.next();expected.next();
        check(!select_monster_spawn_position(layout,{}, {},{-1,-1},1,random) && random.state()==expected.state());
        layout.at(0,0).kind=7;occupied.assign(6,true);
        auto special=select_monster_spawn_position(layout,occupied,{{1,1},{0,0}}, {-1,-1},1,random);check(special && special->x==0 && special->y==0);
        const auto before=random.state();bool rejected=false;
        try {select_monster_spawn_position(layout,occupied,{}, {-1,-1},0,random,4);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && random.state()==before);
        std::cout<<"ordinary respawn searches preserve fixed coordinates, empty floor14 selection, special site occupancy and RNG failure policy\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
