#include "dredmor/generation.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("location assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        DungeonLayout open(9,9,std::vector<DungeonCell>(81,{1}));std::vector<bool> empty(81);DarwinRandom random(1);
        auto found=find_good_location_near(open,{4,4},empty,random);check(found.found && found.position.x==1 && found.position.y==1 && random.state()==1);
        found=find_good_location_near(open,{0,0},empty,random);check(found.found && found.position.x==0 && found.position.y==0 && random.state()==1);
        auto singleton=random;singleton.next();singleton.next();found=find_good_location_bounded(open,{3,3},{4,4},empty,random);check(found.found && found.position.x==3 && found.position.y==3 && random.state()==singleton.state());
        DungeonLayout shop(9,9,std::vector<DungeonCell>(81,{13}));random.seed(1);auto expected=random;Position last;
        for (int i=0;i<51;++i) {last.x=-6+int(expected.bounded(20));last.y=-6+int(expected.bounded(20));}
        found=find_good_location_near(shop,{4,4},empty,random);check(!found.found && found.position.x==last.x && found.position.y==last.y && random.state()==expected.state());
        std::vector<bool> occupied(81,true);random.seed(1);found=find_good_location_near(open,{4,4},occupied,random);check(!found.found && random.state()==expected.state());
        random.seed(1);found=find_good_location_bounded(open,{4,4},{3,3},empty,random);check(!found.found && found.position.x==4 && found.position.y==4 && random.state()==1);
        bool rejected=false;try {find_good_location_near(open,{4,4},{},random);} catch(const std::invalid_argument&) {rejected=true;}check(rejected && random.state()==1);
        std::cout<<"nearby scan order, clipped edges, singleton draws, shop-floor rejection, bounded retries/final refs and invalid occupancy rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
