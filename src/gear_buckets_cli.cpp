#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
void print(const std::vector<dredmor::ItemDefinition>& items,const dredmor::ItemFactoryTables& tables) {
    for (const auto kind:{dredmor::ItemKind::weapon,dredmor::ItemKind::armour}) {
        std::array<dredmor::GearBuckets,4> buckets;
        for (std::size_t set=0;set<4;++set) buckets[set]=dredmor::build_gear_buckets(items,tables.named[set],kind);
        for (std::size_t level=0;level<20;++level) for (std::size_t set=0;set<4;++set) {
            const auto& pool=buckets[set][level];std::cout<<pool.size();for (auto index:pool) std::cout<<' '<<index;std::cout<<' ';
        }
    }
    std::cout<<'\n';
}
int main(int argc,char** argv) {
    try {
        if (argc==2) {const auto content=dredmor::GameContent::load(argv[1]);print(content.items(),content.item_factory_tables());return 0;}
        std::size_t count;
        while (std::cin>>count) {
            if (count>10000) throw std::invalid_argument("too many bucket fixtures");
            std::vector<dredmor::ItemDefinition> items(count);dredmor::ItemFactoryTables tables;
            for (std::size_t i=0;i<count;++i) {
                unsigned kind;std::uint32_t mask;auto& item=items[i];std::cin>>kind>>item.level>>item.special>>mask;
                item.kind=static_cast<dredmor::ItemKind>(kind);dredmor::register_factory_item(tables,i,item,mask);
            }
            if (!std::cin) throw std::invalid_argument("incomplete bucket fixture");print(items,tables);
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
