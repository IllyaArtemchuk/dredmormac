#include "dredmor/factory.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <cstring>

int main() {
    try {
        using namespace dredmor;
        std::uint32_t seed;
        while (std::cin>>seed) {
            ItemFactoryContext context;std::string type,subtype;int quantity;bool artifact;std::size_t count;
            std::cin>>context.level_set>>context.depth.depth>>context.depth.alternate>>context.depth.alternate_depth
                >>context.gold_per_level>>context.trap_deviation>>std::quoted(type)>>std::quoted(subtype)>>quantity>>artifact>>count;
            if (count>10000) throw std::invalid_argument("too many fixture definitions");
            std::vector<ItemDefinition> definitions(count);
            for (auto& item:definitions) {
                unsigned kind;std::cin>>std::quoted(item.name)>>kind>>item.builtin_subtype>>item.weapon_type>>item.level>>item.special>>item.debug;
                item.kind=static_cast<ItemKind>(kind);item.wand_min_charge=item.wand_max_charge=1;
            }
            ItemFactoryTables tables;
            auto pool=[&](auto& result) {
                std::size_t size;std::cin>>size;if (size>10000) throw std::invalid_argument("too many fixture entries");
                result.resize(size);for (auto& entry:result) std::cin>>entry;
            };
            for (auto& result:tables.named) pool(result);
            for (auto* sets:{&tables.potions,&tables.wands,&tables.bolts,&tables.traps}) for (auto& result:*sets) pool(result);
            pool(tables.reagents);
            if (!std::cin) throw std::invalid_argument("incomplete factory fixture");
            GameRandom random;random.libc.seed(seed);random.mt.seed(0xd0ed);
            const auto result=make_loot_item(definitions,tables,context,type,subtype,quantity,artifact,1,random);
            if (!result) std::cout<<"0 -1 0 0 0";
            else std::cout<<unsigned(result->kind)<<' '<<result->definition<<' '<<result->quantity<<' '<<result->stackable<<' '<<result->currency_value.value_or(0);
            const auto cache=random.normal.state();std::uint32_t bits;std::memcpy(&bits,&cache.spare,4);
            std::cout<<' '<<random.libc.state()<<' '<<cache.available<<' '<<bits;
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
