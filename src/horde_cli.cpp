#include "dredmor/hordes.hpp"
#include "dredmor/names.hpp"
#include "dredmor/artifact.hpp"
#include "dredmor/monster_sprites.hpp"
#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==3 && std::string(argv[1])=="--content") {
            const auto content=GameContent::load(argv[2]);std::uint32_t seed;
            while (std::cin>>seed) {
                HordeOptions options;std::int32_t constructor_depth;unsigned difficulty;bool same,named,quest;
                if (!(std::cin>>options.depth>>constructor_depth>>difficulty>>same>>named>>quest>>options.minimum>>options.additional>>options.position.x>>options.position.y>>options.expansion) || difficulty>2)
                    throw std::invalid_argument("invalid content horde fixture");
                if (named) options.named="D0_2";if (quest) options.quest=HordeQuest{13,"Quest"};
                std::vector<DungeonCell> cells(25);for (auto& cell:cells) cell.kind=1;
                DungeonLayout argument(5,5,cells),current(5,5,cells);std::vector<bool> occupied(25,false);
                std::vector<HordeRoomBounds> rooms={{{0,0},{5,5}}};
                HordeWorld world{argument,same?argument:current,occupied,occupied,rooms};GameRandom random;random.seed(seed);
                const auto population=content.create_horde(options,world,static_cast<Difficulty>(difficulty),constructor_depth,{},random);
                const auto& result=population.result;
                std::cout<<result.accepted<<' '<<result.depth<<' '<<result.table_level<<' '<<result.selected<<' '
                    <<result.quest_count.has_value()<<' '<<result.quest_count.value_or(0)<<' '<<random.libc.state();
                const auto mt=random.mt.state();std::cout<<' '<<mt.index;for (auto value:mt.words) std::cout<<' '<<value;
                for (unsigned i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<' '<<population.actors.size();
                for (std::size_t i=0;i<population.actors.size();++i) {
                    const auto& actor=population.actors[i];const auto& member=result.members.at(i);
                    std::cout<<' '<<actor.id<<' '<<member.definition<<' '<<member.leader<<' '<<member.quest<<' '<<member.horde;
                    for (auto value:member.primary_percent) std::cout<<' '<<value;
                    std::cout<<' '<<std::quoted(member.moniker)<<' '<<std::quoted(member.platoon)<<' '<<member.followers.size();
                    for (auto follower:member.followers) std::cout<<' '<<follower;
                    std::cout<<' '<<actor.health<<' '<<actor.mana;
                    for (auto value:actor.stat_profile->ranks) std::cout<<' '<<value;
                    for (auto value:actor.primary) std::cout<<' '<<value;
                    for (auto value:actor.secondary) std::cout<<' '<<value;
                    std::cout<<' '<<actor.sprite_tint<<' '<<std::quoted(actor.sprite_palette);
                }
                std::cout<<' '<<result.teleports.size();for (const auto& t:result.teleports) std::cout<<' '<<t.object<<' '<<t.level<<' '<<t.position.x<<' '<<t.position.y;
                std::cout<<' '<<result.searches.size();for (const auto& s:result.searches) std::cout<<' '<<s.seed.x<<' '<<s.seed.y<<' '<<s.position.x<<' '<<s.position.y<<' '<<s.found<<' '<<s.accepted;
                std::cout<<'\n';
            }
            return 0;
        }
        const bool real_tint=argc==2 && std::string(argv[1])=="--names-tint";
        const bool real_names=real_tint || (argc==2 && std::string(argv[1])=="--names");
        ArtifactWords name_words;std::size_t label=0;
        for (auto* pool:{&name_words.starts,&name_words.phonemes,&name_words.titles,&name_words.nouns,&name_words.plurals,&name_words.adjectives}) {
            const std::size_t sizes[]={3,4,3,5,5,6};const char* labels[]={"S","P","T","N","L","A"};
            for (std::size_t i=0;i<sizes[label];++i) pool->push_back(std::string(labels[label])+std::to_string(i));
            ++label;
        }
        std::uint32_t seed;
        while (std::cin>>seed) {
            HordeOptions options;int default_min,default_extra,chance,ctor_draws,mon_draws,platoon_draws;bool named,quest;std::uint32_t named_index;
            std::cin>>options.depth>>options.expansion>>options.level_set>>options.position.x>>options.position.y>>options.minimum>>options.additional>>default_min>>default_extra>>chance>>named>>named_index>>quest>>ctor_draws>>mon_draws>>platoon_draws;
            if (ctor_draws<0 || ctor_draws>10 || mon_draws<0 || mon_draws>10 || platoon_draws<0 || platoon_draws>10) throw std::invalid_argument("invalid callback draw counts");
            if (named) options.named="named";if (quest) options.quest=HordeQuest{13,"Quest"};
            PrimaryStats percentages;for (auto& value:percentages) std::cin>>value;
            std::size_t count;std::cin>>count;if (!count || count>64) throw std::invalid_argument("invalid horde definitions");std::vector<MonsterDefinition> definitions(count);std::vector<bool> allowed(count);
            for (std::size_t i=0;i<count;++i) {bool allow;std::cin>>definitions[i].special>>allow;allowed[i]=allow;definitions[i].horde=allow;}
            std::cin>>count;if (!count || count>64) throw std::invalid_argument("invalid horde pool");std::vector<std::uint32_t> base_pool(count);for (auto& value:base_pool) std::cin>>value;
            MonsterTables tables;for (std::size_t level=0;level<15;++level) for (std::size_t set=0;set<4;++set) for (std::size_t i=0;i<base_pool.size();++i) tables.levels[level][set].push_back(base_pool[(i+level+set)%base_pool.size()]);
            if (real_tint) for (std::size_t i=0;i<definitions.size();++i) {
                auto& d=definitions[i];std::size_t slot=0;
                auto path=[&](auto& value) {if ((i+slot)%3) value="M"+std::to_string(i)+"/"+std::to_string(slot);++slot;};
                for (auto* group:{&d.idle_sprites,&d.attack_sprites,&d.hit_sprites,&d.beam_sprites}) for (auto& value:*group) path(value);
                for (auto* value:{&d.die_sprite,&d.cast_sprite,&d.dig_up_sprite,&d.dig_down_sprite}) path(*value);
            }
            std::vector<HordeRoomBounds> rooms(4);for (auto& room:rooms) std::cin>>room.minimum.x>>room.minimum.y>>room.maximum.x>>room.maximum.y;
            std::vector<DungeonCell> argument_cells(25),current_cells(25);std::vector<bool> argument_occupied(25),current_occupied(25);
            for (std::size_t i=0;i<25;++i) {unsigned kind;bool occupied;std::cin>>kind>>occupied;if (kind>255) throw std::invalid_argument("invalid argument cell");argument_cells[i].kind=kind;argument_occupied[i]=occupied;}
            for (std::size_t i=0;i<25;++i) {unsigned kind,room;bool occupied;std::cin>>kind>>room>>occupied;if (kind>255 || room>255) throw std::invalid_argument("invalid current cell");current_cells[i].kind=kind;current_cells[i].room_id=room;current_occupied[i]=occupied;}
            if (!std::cin) throw std::runtime_error("truncated horde fixture");
            DungeonLayout argument(5,5,std::move(argument_cells)),current(5,5,std::move(current_cells));HordeWorld world{argument,current,argument_occupied,current_occupied,rooms};GameRandom random;random.seed(seed);
            std::vector<std::array<std::int64_t,5>> trace;std::uint64_t constructed=0;
            auto event=[&](int stage,std::int64_t a=0,std::int64_t b=0,std::int64_t c=0,std::int64_t d=0) {trace.push_back({stage,a,b,c,d});};
            auto draws=[](GameRandom& rng,int n) {for (int i=0;i<n;++i) rng.libc.next();};
            MonsterSpriteState tinted;std::int32_t chosen_tint=0;std::vector<std::pair<std::string,std::int32_t>> tint_requests;
            auto handles=[&](auto function) {
                for (auto* group:{&tinted.idle,&tinted.attack,&tinted.hit,&tinted.beam}) for (auto& value:*group) function(value);
                for (auto* value:{&tinted.die,&tinted.cast,&tinted.dig_up,&tinted.dig_down}) function(*value);
            };
            std::uint64_t initial=2000;handles([&](auto& value){value=initial++;});tinted.current=76;tinted.frame=1;tinted.phase=2;tinted.animation_flag=3;
            HordeHooks hooks;
            hooks.tweak=[&](std::string_view name) {
                if (name=="additional horde size") {event(0,default_extra);return default_extra;}
                if (name=="minimum horde size") {event(1,default_min);return default_min;}
                if (name!="horde is overpowered change") throw std::runtime_error("unknown horde tweak");event(2,chance);return chance;
            };
            hooks.can_spawn=[&](std::uint32_t index,std::int32_t depth,bool force) {event(3,index,depth,force);return real_tint?can_spawn_monster(definitions.at(index),depth,MonsterSpawnState{},force):allowed.at(index);};
            hooks.find_named=[&](std::string_view name) {if (name!="named") throw std::runtime_error("wrong named horde lookup");event(4,named_index);return named_index;};
            hooks.construct=[&](std::uint32_t index,std::int32_t depth,GameRandom& rng) {HordeMember member;member.identity=++constructed;member.primary_percent=percentages;event(5,index,depth,member.identity);draws(rng,ctor_draws);return member;};
            hooks.monster_name=[&](GameRandom& rng) {event(6,constructed);if (real_names) return generate_monster_name(name_words,rng);draws(rng,mon_draws);return "Mon "+std::to_string(rng.libc.state());};
            hooks.platoon_name=[&](GameRandom& rng) {event(7,constructed);if (real_names) return generate_platoon_name(name_words,rng);draws(rng,platoon_draws);return "Platoon "+std::to_string(rng.libc.state());};
            hooks.tint=[&](HordeMember& member,std::int32_t tint,GameRandom& rng) {event(8,member.identity,tint);
                if (real_tint) chosen_tint=tint_monster_sprites(definitions.at(member.definition),tinted,tint,rng.libc,[&](const auto& path,auto angle) {
                    tint_requests.emplace_back(path,angle);return 999+tint_requests.size();
                });
                else draws(rng,1);};
            hooks.rebuild=[&](HordeMember& member,GameRandom&) {event(9,member.identity);};
            hooks.queue=[&](const RoomTeleport& entry,const HordeMember& member) {event(10,entry.object,member.horde,member.leader,member.quest);};
            const auto result=spawn_horde(tables,definitions,options,world,random,hooks);
            std::cout<<result.accepted<<' '<<result.depth<<' '<<result.table_level<<' '<<result.minimum<<' '<<result.additional<<' '<<result.selected<<' '<<result.leader_position.x<<' '<<result.leader_position.y<<' '<<result.quest_count.has_value()<<' '<<result.quest_count.value_or(0)<<' '<<random.libc.state();
            const auto mt=random.mt.state();std::cout<<' '<<mt.index;for (auto value:mt.words) std::cout<<' '<<value;for (unsigned i=0;i<8;++i) std::cout<<' '<<random.mt.u32();
            std::cout<<' '<<result.members.size();for (const auto& member:result.members) {
                std::cout<<' '<<member.identity<<' '<<member.definition<<' '<<member.leader<<' '<<member.quest<<' '<<member.horde;
                for (auto value:member.primary_percent) std::cout<<' '<<value;
                std::cout<<' '<<std::quoted(member.moniker)<<' '<<std::quoted(member.platoon)<<' '<<member.followers.size();for (auto follower:member.followers) std::cout<<' '<<follower;
            }
            std::cout<<' '<<result.teleports.size();for (const auto& entry:result.teleports) std::cout<<' '<<entry.object<<' '<<entry.level<<' '<<entry.position.x<<' '<<entry.position.y;
            std::cout<<' '<<result.searches.size();for (const auto& search:result.searches) std::cout<<' '<<search.seed.x<<' '<<search.seed.y<<' '<<search.position.x<<' '<<search.position.y<<' '<<search.found<<' '<<search.accepted;
            std::cout<<' '<<trace.size();for (const auto& entry:trace) for (auto value:entry) std::cout<<' '<<value;
            if (real_tint) {
                std::cout<<' '<<!tint_requests.empty();
                if (!tint_requests.empty()) {
                    std::cout<<' '<<chosen_tint<<' '<<tinted.current<<' '<<unsigned(tinted.animation_flag)<<' '<<tinted.frame<<' '<<tinted.phase;
                    handles([](auto value){std::cout<<' '<<value;});std::cout<<' '<<tint_requests.size();
                    for (const auto& [path,angle]:tint_requests) std::cout<<' '<<std::quoted(path)<<' '<<angle;
                }
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
