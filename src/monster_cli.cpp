#include "dredmor/monsters.hpp"
#include "dredmor/content.hpp"
#include <iostream>
void print_tables(const dredmor::MonsterTables& tables) {
    for (const auto& pool:tables.named) {std::cout<<pool.size();for (auto index:pool) std::cout<<' '<<index;std::cout<<' ';}
    for (const auto& level:tables.levels) for (const auto& pool:level) {std::cout<<pool.size();for (auto index:pool) std::cout<<' '<<index;std::cout<<' ';}
    std::cout<<tables.diggle_hell.size();for (auto index:tables.diggle_hell) std::cout<<' '<<index;std::cout<<'\n';
}
int main(int argc,char** argv) {
    if (argc==3 && std::string(argv[1])=="--tables-content") {print_tables(dredmor::GameContent::load(argv[2]).monster_tables());return 0;}
    if (argc==2 && std::string(argv[1])=="--tables") {
        std::size_t count;while (std::cin>>count) {
            if (count>10000) return 1;dredmor::MonsterTables tables;
            for (std::size_t i=0;i<count;++i) {
                dredmor::MonsterDefinition definition;std::uint32_t mask;
                if (!(std::cin>>definition.level>>definition.special>>definition.diggle_hell>>mask)) return 1;
                dredmor::register_monster(tables,i,definition,mask);
            }
            print_tables(tables);
        }
        return std::cin.eof()?0:1;
    }
    const bool creation=argc==2 && std::string(argv[1])=="--creation";
    if (argc!=1 && !creation) return 1;
    dredmor::MonsterStatInput input;
    std::uint32_t seed=1;int adder=0;
    while (creation?bool(std::cin>>seed>>adder>>input.ranks[0]):bool(std::cin>>input.ranks[0])) {
        for (std::size_t i=1;i<3;++i) if (!(std::cin>>input.ranks[i])) return 1;
        for (auto* values:{&input.definition_primary,&input.primary_flat,&input.primary_percent})
            for (auto& value:*values) if (!(std::cin>>value)) return 1;
        for (auto* values:{&input.definition_secondary,&input.secondary_flat,&input.secondary_percent})
            for (auto& value:*values) if (!(std::cin>>value)) return 1;
        if (!(std::cin>>input.dungeon_level>>input.health_scale>>input.health_percent_per_level>>input.health_per_level)) return 1;
        dredmor::DarwinRandom random(seed);
        if (creation) {
            input.ranks=dredmor::monster_creation_ranks(input.ranks,adder,random);
            for (auto value:input.ranks) std::cout<<value<<' ';
        }
        auto result=dredmor::derive_monster(input);
        for (auto value:result.primary) std::cout<<value<<' ';
        for (std::size_t i=0;i<24;++i) std::cout<<(i?" ":"")<<result.secondary[i];
        if (creation) std::cout<<' '<<random.state();std::cout<<'\n';
    }
    return std::cin.eof()?0:1;
}
