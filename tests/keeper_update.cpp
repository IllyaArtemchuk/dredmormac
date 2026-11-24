#include "dredmor/shops.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("keeper update assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
bool same(const GameRandom& a,const GameRandom& b) {return a.libc.state()==b.libc.state() && a.mt.state().index==b.mt.state().index && a.mt.state().words==b.mt.state().words && a.normal.state().available==b.normal.state().available && a.normal.state().spare==b.normal.state().spare;}
int main() {
    try {
        GameRandom random;random.seed(7);auto before=random;const auto calm=update_shopkeeper(false,false,random,{},{},{});
        check(!calm.angry && !calm.monster_updated && !calm.location_queried && calm.flavour_text=="You hear the sounds of a cash register." && same(random,before));
        std::vector<int> order;
        const auto angry=update_shopkeeper(false,true,random,[&](GameRandom& stream){order.push_back(1);stream.libc.next();},[&](GameRandom& stream)->std::optional<Position>{order.push_back(2);stream.libc.next();return Position{1,2};},[&](Position point,GameRandom& stream){check(point.x==1 && point.y==2);order.push_back(3);stream.libc.next();});
        auto expected=before;expected.libc.next();check(expected.mt.bounded(4)==0);expected.libc.next();expected.libc.next();
        check(angry.angry && angry.monster_updated && angry.location_queried && angry.collector_spawned && angry.collector_position->x==1 && order==std::vector<int>({1,2,3}) && same(random,expected));
        random=before;bool rejected=false;
        try {update_shopkeeper(true,false,random,[](GameRandom& r){r.libc.next();},[](GameRandom& r)->std::optional<Position>{r.mt.u32();return Position{2,3};},[](Position,GameRandom& r){r.libc.next();throw std::runtime_error("construction failed");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && same(random,before));
        const auto failed_site=update_shopkeeper(true,false,random,[](GameRandom&){},[](GameRandom&)->std::optional<Position>{return std::nullopt;},[](Position,GameRandom&){throw std::logic_error("unexpected collector");});
        check(failed_site.location_queried && !failed_site.collector_spawned && !failed_site.collector_position && random.mt.state().index==1);
        random=before;rejected=false;try {update_shopkeeper(true,false,random,{},{},{});} catch(const std::invalid_argument&) {rejected=true;}check(rejected && same(random,before));
        std::cout<<"keeper calm/level anger, base-update/quarter/site/collector ordering, failed placement and RNG rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
