#include "dredmor/monster_sprites.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
template<class Definition,class F> void paths(Definition& d,F function) {
    for (auto* group:{&d.idle_sprites,&d.attack_sprites,&d.hit_sprites,&d.beam_sprites})
        for (auto& path:*group) function(path);
    for (auto* path:{&d.die_sprite,&d.cast_sprite,&d.dig_up_sprite,&d.dig_down_sprite}) function(*path);
}
template<class F> void handles(dredmor::MonsterSpriteState& s,F function) {
    for (auto* group:{&s.idle,&s.attack,&s.hit,&s.beam}) for (auto& value:*group) function(value);
    for (auto* value:{&s.die,&s.cast,&s.dig_up,&s.dig_down}) function(*value);
}
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--horde-metadata") {
            std::string xml;
            while (std::cin>>std::quoted(xml)) {
                const auto definition=dredmor::read_monster(dredmor::parse_xml(xml));
                std::cout<<definition.horde<<' '<<definition.max_spawns<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--eligibility") {
            bool horde,force;std::int32_t depth,limit;
            while (std::cin>>horde>>force>>depth>>limit) {
                dredmor::MonsterDefinition definition;definition.horde=horde;definition.max_spawns=limit;
                std::size_t count;if (!(std::cin>>std::quoted(definition.name)>>count) || count>1000)
                    throw std::invalid_argument("invalid eligibility fixture");
                dredmor::MonsterSpawnState state;
                for (std::size_t i=0;i<count;++i) {
                    std::string name;std::int32_t previous;
                    if (!(std::cin>>std::quoted(name)>>previous) || !state.counts.emplace(name,previous).second)
                        throw std::invalid_argument("invalid spawn counter");
                }
                std::cout<<dredmor::can_spawn_monster(definition,depth,state,force)<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--records") {
            std::string parent_xml,child_xml;
            while (std::cin>>std::quoted(parent_xml)>>std::quoted(child_xml)) {
                const auto parent=dredmor::read_monster(dredmor::parse_xml(parent_xml));
                const auto child=dredmor::read_monster(dredmor::parse_xml(child_xml),&parent);
                paths(child,[](const auto& path){std::cout<<std::quoted(path)<<' ';});std::cout<<'\n';
            }
            return 0;
        }
        if (argc!=1) throw std::invalid_argument("Usage: dredmor-monster-sprites [--records]");
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::MonsterDefinition definition;dredmor::MonsterSpriteState state;
            std::int32_t angle;unsigned flag;
            if (!(std::cin>>angle>>state.current>>state.preserve_current>>flag>>state.frame>>state.phase) || flag>255)
                throw std::invalid_argument("invalid monster sprite state");
            state.animation_flag=static_cast<std::uint8_t>(flag);
            paths(definition,[](auto& path){if (!(std::cin>>std::quoted(path))) throw std::invalid_argument("missing monster path");});
            std::uint64_t next=2000;handles(state,[&](auto& value){value=next++;});
            std::vector<std::pair<std::string,std::int32_t>> requests;dredmor::DarwinRandom random(seed);
            const auto result=dredmor::tint_monster_sprites(definition,state,angle,random,[&](const auto& path,auto tint){
                requests.emplace_back(path,tint);return 999+requests.size();
            });
            std::cout<<result<<' '<<random.state()<<' '<<state.current<<' '<<state.preserve_current<<' '
                <<unsigned(state.animation_flag)<<' '<<state.frame<<' '<<state.phase<<' ';
            handles(state,[](auto value){std::cout<<value<<' ';});
            std::cout<<requests.size();for (const auto& [path,tint]:requests) std::cout<<' '<<std::quoted(path)<<' '<<tint;
            std::cout<<'\n';
        }
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
