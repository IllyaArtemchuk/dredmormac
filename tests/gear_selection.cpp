#include "dredmor/factory.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("gear assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string state(GameRandom random) {
    std::ostringstream out;out<<random.libc.state()<<' '<<random.normal.state().available<<' '<<random.normal.state().spare;
    for (int i=0;i<16;++i) out<<' '<<random.mt.u32();return out.str();
}
int main() {
    try {
        std::vector<GearCandidate> gear(2);gear[1].craft_output=true;GearBuckets buckets(22);
        buckets[2]={1};buckets[1]={1};buckets[0]={0};GameRandom random;random.seed(42);random.normal.restore({true,0});
        auto selected=select_weapon(gear,buckets,random,{1,false,-1},0);
        check(selected.index==0 && selected.target_level==1 && selected.bucket==0 && selected.draws==202);
        gear[0].artifact=true;gear[0].artifact_spawned=true;buckets[2]={0};random.seed(42);random.normal.restore({true,0});
        selected=select_armour(gear,buckets,random,{1,false,-1},0);
        check(selected.index==0 && selected.bucket==2); // Armour ignores artifact uniqueness.
        auto previous=state(random);bool rejected=false;
        try {select_weapon(gear,buckets,random,{1,false,-1},0,true);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && state(random)==previous && gear[0].artifact_spawned);
        gear[0].artifact_spawned=false;random.seed(42);random.normal.restore({true,0});
        selected=select_weapon(gear,buckets,random,{1,false,-1},0,true);check(selected.index==0 && gear[0].artifact_spawned);
        buckets[2]={1};previous=state(random);rejected=false;
        try {select_armour(gear,buckets,random,{1,false,-1},0,0,3);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && state(random)==previous);
        std::vector<ItemDefinition> items{
            read_item(parse_xml("<item name='Weapon' level='1' type='0'><weapon/></item>")),
            read_item(parse_xml("<item name='Armour' level='1'><armour type='head'/></item>")),
            read_item(parse_xml("<item name='Special' level='1' special='1'><weapon/></item>")),
            read_item(parse_xml("<item name='High' level='20'><weapon/></item>"))};
        ItemFactoryTables tables;tables.named[0]={0,1,2,3};ItemFactoryContext context;
        const auto weapon_buckets=build_gear_buckets(items,tables.named[0],ItemKind::weapon);
        check(weapon_buckets[1]==std::vector<std::size_t>({0}) && weapon_buckets[20].empty());
        random.seed(42);auto item=make_loot_item(items,tables,context,"weapon","random",4,false,10,random);
        check(item && item->definition==0 && item->quantity==4 && item->stackable);
        item=make_loot_item(items,tables,context,"armour","random",1,false,11,random);check(item && item->definition==1);
        std::cout<<"gear rejection/downshift counts, artifact eligibility, exhaustion/budget rollback and factory construction passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
