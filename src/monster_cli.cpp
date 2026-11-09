#include "dredmor/monsters.hpp"
#include "dredmor/content.hpp"
#include <iostream>
void print_tables(const dredmor::MonsterTables& tables) {
    for (const auto& pool:tables.named) {std::cout<<pool.size();for (auto index:pool) std::cout<<' '<<index;std::cout<<' ';}
    for (const auto& level:tables.levels) for (const auto& pool:level) {std::cout<<pool.size();for (auto index:pool) std::cout<<' '<<index;std::cout<<' ';}
    std::cout<<tables.diggle_hell.size();for (auto index:tables.diggle_hell) std::cout<<' '<<index;std::cout<<'\n';
}
int main(int argc,char** argv) {
    if (argc==2 && std::string(argv[1])=="--spawn") {
        std::uint32_t seed;while (std::cin>>seed) {
            dredmor::MonsterSpawnContext context;int boost,adder;std::size_t count;
            if (!(std::cin>>context.depth.depth>>context.depth.alternate>>context.depth.alternate_depth>>boost>>context.expansion>>context.level_set>>adder>>count) || count>10000) return 1;
            std::vector<dredmor::MonsterDefinition> definitions(count);dredmor::MonsterTables tables;dredmor::MonsterSpawnState state;
            for (std::size_t i=0;i<count;++i) {
                auto& def=definitions[i];def.name="Fixture"+std::to_string(i);std::uint32_t mask;int previous;
                if (!(std::cin>>def.level>>mask>>def.max_spawns>>previous>>def.ranks[0]>>def.ranks[1]>>def.ranks[2])) return 1;
                dredmor::register_monster(tables,i,def,mask);if (previous) state.counts.emplace(def.name,previous);
            }
            dredmor::GameRandom random;random.seed(seed);const auto selected=dredmor::select_room_monster(tables,definitions,context,boost,state,random);
            if (!selected) return 1;const auto& def=definitions[*selected];dredmor::MonsterStatInput input;
            input.ranks=dredmor::monster_creation_ranks(def.ranks,adder,random.libc);input.dungeon_level=context.depth.depth;
            const auto stats=dredmor::derive_monster(input);const auto flags=dredmor::finish_random_monster_spawn(def,state,random.mt);
            std::cout<<*selected<<' '<<flags;for (auto value:input.ranks) std::cout<<' '<<value;
            for (auto value:stats.primary) std::cout<<' '<<value;for (auto value:stats.secondary) std::cout<<' '<<value;
            for (const auto& definition:definitions) {auto found=state.counts.find(definition.name);std::cout<<' '<<(found==state.counts.end()?0:found->second);}
            std::cout<<' '<<random.libc.state();for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
        return std::cin.eof()?0:1;
    }
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
