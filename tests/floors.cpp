#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("floor assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
World map(std::string text="#######\n#@....#\n#.....#\n#.....#\n#######\n") {std::istringstream input(text);return World::read(input);}
std::string saved(const Session& session,const FloorRegistry& floors) {std::ostringstream out;floors.save(session,out);return out.str();}
int main() {
    try {
        DungeonLayout layout(5,6,std::vector<DungeonCell>(30));layout.at(1,4)={3,8,5,19,7};layout.at(1,3)={4,9,6,20,8};layout.at(3,4)={4,10,7,21,9};
        auto stairs=build_stair_plan(layout,1,false);check(stairs.size()==2 && stairs[0].up && stairs[0].link==7 && !stairs[1].up && layout.at(1,3).kind==2 && layout.at(3,3).kind==2);
        check(layout.at(1,3).room_id==9 && layout.at(1,3).stair_link==8 && layout.at(1,3).flags==20);
        DungeonLayout top(3,4,std::vector<DungeonCell>(12));top.at(1,2).kind=3;check(build_stair_plan(top,0,false).empty() && top.at(1,2).kind==1);
        DungeonLayout bottom(3,4,std::vector<DungeonCell>(12));bottom.at(1,2).kind=4;check(build_stair_plan(bottom,9,false).empty() && bottom.at(1,2).kind==4 && bottom.at(1,1).kind==2);
        bottom.at(1,2).kind=4;check(build_stair_plan(bottom,9,true).size()==1);
        const auto wrapped=stair_destination(false,255,-3,INT32_MAX);check(wrapped.link==255 && wrapped.branch==-3 && wrapped.depth==INT32_MIN);
        std::vector<StairPlacement> arrivals={{{4,1},false,1},{{2,3},false,2},{{2,1},false,2},{{3,1},true,1}};
        check(stair_arrival(arrivals,true,2)->y==1 && stair_arrival(arrivals,true,99)->x==2 && !stair_arrival(arrivals,false,99) && stair_arrival(arrivals,false,99,true)->x==3);
        auto items=std::make_shared<std::vector<ItemDefinition>>();items->push_back(read_item(parse_xml("<item name='Potion'><potion spell='Mana'/></item>")));
        auto spells=std::make_shared<std::vector<SpellDefinition>>();
        for (const auto& xml:{"<spell name='Power' type='self' downtime='3'/>","<spell name='Buff' type='self'><buff time='10' icon='buff'><primarybuff id='0' amount='1'/></buff></spell>","<spell name='Mana' type='self'><effect type='spellpoints' amount='-1'/></spell>","<spell name='Delayed' type='target'><effect type='trigger' amount='10' spell='Mana'/></spell>"}) spells->push_back(read_spell(parse_xml(xml)));
        auto abilities=std::make_shared<std::vector<AbilityDefinition>>();abilities->push_back(read_ability(parse_xml("<ability name='Caster' skill='0'><spell name='Power'/></ability>")));
        Session session(map(),make_player({20,20,20}),1);std::vector<SkillRank> skills;for (int i=0;i<7;++i) skills.push_back({i,1});session.character_profile({20,20,20},items,skills,abilities);session.spell_profile(spells);session.floor_stairs({{{2,1},false,7}});
        session.cast_spell(spells->at(1),{1,1},{1,1});session.cast_spell(spells->at(3),{1,1},{1,1});check(session.use_spell(0,{1,1})==SpellUse::cast);
        const auto carried=session.create_ground_item(0,{1,1});session.pickup();const auto left=session.create_ground_item(0,{3,1});
        FloorRegistry floors;SessionFloor second(map("########\n#@.....#\n#......#\n#......#\n########\n"));second.depth.depth=1;second.branch=99;second.stairs={{{1,1},true,7},{{2,1},false,9}};
        Actor enemy;enemy.name="Cached";enemy.position={6,3};enemy.secondary[0]=30;enemy.health=30;second.enemies.push_back(enemy);
        DarwinRandom random(1);auto loot=make_inventory_item(items->at(0),0,1,random);loot.instance=0;second.ground.push_back({loot,{1,1}});floors.add_floor(session,second);
        const auto reserved=floors.cached()[0].ground[0].item.instance;check(reserved>left);const auto fresh=session.create_ground_item(0,{4,1});check(fresh>reserved);
        const auto before_travel=saved(session,floors);std::istringstream early(before_travel);auto early_restore=FloorRegistry::load(early,items,abilities,spells);check(saved(early_restore.first,early_restore.second)==before_travel);
        check(floors.step(session,Action::east)==FloorTravel::travelled && session.depth_context().depth==1 && session.floor_branch()==99 && session.world().width()==8 && session.world().turn()==2 && session.player().position.x==1 && session.spell_cooldown(0)==2);
        check(session.inventory().find(carried) && session.player_buffs().size()==1 && session.player_buffs()[0].remaining==8 && session.player().triggers[0].remaining==8);
        session.pickup();check(!session.ground_items().size() && session.inventory().find(carried)->quantity==2);
        const auto pending=saved(session,floors);std::istringstream input(pending);auto restored=FloorRegistry::load(input,items,abilities,spells);check(saved(restored.first,restored.second)==pending);
        check(floors.step(session,Action::east)==FloorTravel::missing_floor && session.depth_context().depth==1 && session.world().turn()==3);
        auto expected=session;expected.step(Action::west);const auto enemy_position=expected.enemies()[0].position;
        check(floors.step(session,Action::west)==FloorTravel::travelled && session.depth_context().depth==0 && session.world().turn()==4 && session.player().position.x==2 && session.ground_items().size()==2);
        check(session.ground_items()[0].item.instance==left && session.ground_items()[1].item.instance==fresh && floors.cached()[0].ground.empty() && floors.cached()[0].enemies[0].position.x==enemy_position.x && floors.cached()[0].enemies[0].position.y==enemy_position.y);
        const auto before=saved(session,floors);bool rejected=false;try {floors.add_floor(session,floors.cached()[0]);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session,floors)==before);
        SessionFloor invalid(map());invalid.depth.depth=2;invalid.stairs={{{0,0},true,0}};rejected=false;try {floors.add_floor(session,invalid);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session,floors)==before);
        auto bad=pending;const auto link=bad.find("link=\"7\"");bad.replace(link,8,"link=\"256\"");std::istringstream malformed(bad);rejected=false;try {FloorRegistry::load(malformed,items,abilities,spells);} catch(const std::exception&) {rejected=true;}check(rejected);
        auto invalid_floor=session.floor_state();invalid_floor.enemies.push_back(enemy);invalid_floor.enemies.back().id=1000;rejected=false;try {session.swap_floor(invalid_floor,session.player().position);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session,floors)==before);
        auto hostile=session;auto target=enemy;target.position={5,3};target.id=0;const auto caster=hostile.spawn_enemy(target);hostile.cast_spell(spells->at(3),target.position,hostile.player().position,caster);const auto guarded=saved(hostile,floors);rejected=false;try {floors.travel(hostile,false);} catch(const std::exception&) {rejected=true;}check(rejected && saved(hostile,floors)==guarded);
        auto collision=session;target.id=floors.cached()[0].enemies[0].id;collision.spawn_enemy(target);rejected=false;try {saved(collision,floors);} catch(const std::exception&) {rejected=true;}check(rejected);
        std::cout<<"stair construction/arrival, persistent floor travel, global item IDs, character effects/cooldowns, run saves and atomic rejection passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
