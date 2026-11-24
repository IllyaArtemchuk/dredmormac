#include "dredmor/hordes.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("horde assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        std::vector<DungeonCell> cells(25);for (auto& cell:cells) cell.kind=2;cells[12].kind=1;
        DungeonLayout map(5,5,cells);std::vector<bool> occupied(25,false);std::vector<HordeRoomBounds> rooms={{{2,2},{3,3}}};
        HordeWorld world{map,map,occupied,occupied,rooms};MonsterTables tables;tables.levels[5][0]={0};tables.levels[4][0]={1};
        std::vector<MonsterDefinition> definitions(2);definitions[1].special=1;
        HordeOptions options;options.position={2,2};options.depth=5;options.minimum=0;options.additional=0;options.named="named";options.quest=HordeQuest{99,"Quest group"};
        std::uint64_t constructed=0;unsigned names=0,platoons=0,lookups=0;std::vector<HordeMember> queued;
        HordeHooks hooks;hooks.tweak=[](auto key) {check(key=="horde is overpowered change");return 2;};
        hooks.can_spawn=[](auto index,auto depth,bool forced) {check(index==0 && depth==4 && forced);return true;};
        hooks.find_named=[&](auto name) {check(name=="named");++lookups;return 1;};
        hooks.construct=[&](auto index,auto depth,GameRandom& random) {check(index==1 && depth==4);random.libc.next();random.libc.next();HordeMember member;member.identity=++constructed;return member;};
        hooks.monster_name=[&](GameRandom& random) {++names;random.libc.next();return "Leader name";};
        hooks.platoon_name=[&](GameRandom&) {++platoons;return "Generated group";};
        hooks.tint=[](HordeMember& member,auto tint,GameRandom& random) {check(member.primary_percent[0]==120 && tint==-1 && !member.horde);random.libc.next();};
        hooks.rebuild=[](HordeMember& member,GameRandom&) {check(member.platoon=="Quest group" && !member.horde && !member.quest);};
        hooks.queue=[&](const auto& entry,const HordeMember& member) {check(entry.position.x==2 && entry.position.y==2 && !member.horde && !member.leader && !member.quest);queued.push_back(member);};
        GameRandom random;random.seed(0);const auto result=spawn_horde(tables,definitions,options,world,random,hooks);
        check(result.accepted && result.depth==4 && result.table_level==5 && result.selected==0 && result.members.size()==2 && result.quest_count==2);
        check(result.searches.size()==1 && !result.searches[0].found && result.searches[0].accepted && result.teleports.size()==2);
        check(result.members[0].definition==1 && result.members[0].moniker=="Leader name" && result.members[0].platoon=="Quest group" && result.members[0].primary_percent[1]==110);
        check(result.members[1].definition==1 && result.members[1].primary_percent[0]==100 && result.members[1].leader==1 && result.members[1].quest==99 && result.members[1].horde);
        check(result.members[0].followers==std::vector<std::uint64_t>{2} && queued.size()==2 && lookups==2 && names==1 && !platoons);
        // An invalid mixed sentinel returns before tweaks, selection or RNG.
        auto invalid=options;invalid.position={-1,2};const auto before_mt=random.mt.state();const auto before_libc=random.libc.state();
        check(!spawn_horde(tables,definitions,invalid,world,random,{}).accepted && before_libc==random.libc.state() && before_mt.index==random.mt.state().index && before_mt.words==random.mt.state().words);
        // A supplied name does not bypass random eligibility. An all-special
        // pool reaches the port's budget guard before any named constructor.
        auto unusable=tables;unusable.levels[5][0]={1};options.budget=2;constructed=lookups=0;random.seed(0);bool exhausted=false;
        try {spawn_horde(unusable,definitions,options,world,random,hooks);} catch(const std::runtime_error&) {exhausted=true;}
        check(exhausted && !constructed && !lookups);
        std::cout<<"horde selection uses original depth before constructor-depth reduction, named overrides retain random eligibility, failed search coordinates can spawn duplicate locations and quest/group metadata follows queue timing\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
