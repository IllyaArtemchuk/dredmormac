#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("monster buff assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto buff=read_spell(parse_xml("<spell name='Buff' type='target'><buff time='1' self='0' allowstacking='0' stacksize='1' manaUpkeep='1' icon='fixture'><primarybuff id='0' amount='1'/><secondarybuff id='0' amount='2'/><damagebuff slashing='2'/><resistbuff toxic='3'/></buff></spell>"));
        auto self=buff;self.name="Self";self.type="self";
        auto parent=buff;parent.name="Parent";auto trigger=read_spell_effect(parse_xml("<effect type='trigger' spell='Leaf' amount='3'/>"));parent.effects.push_back(trigger);
        auto leaf=read_spell(parse_xml("<spell name='Leaf' type='target'><effect type='spellpoints' amount='-1'/></spell>"));
        auto gate=buff;gate.name="Gate";auto mana=read_spell_effect(parse_xml("<effect type='spellpoints' amount='-1' requireBuff='1'/>"));gate.effects.push_back(mana);
        const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{buff,self,parent,leaf,gate});
        MonsterStatInput input;input.ranks={4,2,1};input.definition_secondary[0]=100;input.definition_secondary[1]=20;input.dungeon_level=3;input.health_scale=1.25f;input.health_percent_per_level=.1f;input.health_per_level=2;
        const auto stats=derive_monster(input);Actor enemy;enemy.name="Fixture";enemy.position={5,2};enemy.primary=stats.primary;enemy.secondary=stats.secondary;enemy.health=stats.secondary[0]-10;enemy.mana=0;enemy.stat_profile=input;enemy.damage[0]=2;
        std::istringstream map("#######\n#.@...#\n#.....#\n#######\n");Session base(World::read(map),make_player({20,20,20}),1);base.spell_profile(profile);base.spawn_enemy(enemy);
        auto session=base;session.cast_spell(buff,{2,1},{5,2});auto expected=input;expected.primary_flat[0]=1;expected.secondary_flat[0]=2;
        check(session.enemies()[0].buffs.size()==1 && session.enemies()[0].secondary==derive_monster(expected).secondary && session.enemies()[0].health==enemy.health+2);
        session.cast_spell(buff,{2,1},{5,2});check(session.enemies()[0].buffs.size()==1 && session.enemies()[0].health==enemy.health+2);
        session.update_buffs(2);check(session.enemies()[0].buffs[0].remaining==0 && session.enemies()[0].buffs[0].upkeep_remaining==1 && session.enemies()[0].mana==0);
        std::istringstream data(saved(session));auto resumed=Session::load(data,{},{},profile);
        session.update_buffs(2);resumed.update_buffs(2);check(saved(session)==saved(resumed));
        check(session.enemies()[0].buffs.empty() && session.enemies()[0].primary==enemy.primary && session.enemies()[0].secondary==enemy.secondary && session.enemies()[0].health==enemy.health && session.enemies()[0].damage==enemy.damage && session.enemies()[0].resistance==enemy.resistance);
        session=base;session.cast_spell(self,{5,2},{5,2},2);session.cast_spell(self,{5,2},{5,2},2);check(session.enemies()[0].buffs.size()==2); // Hittable self application ignores player stack caps.
        session=base;session.cast_spell(parent,{2,1},{5,2});check(session.enemies()[0].triggers.size()==1);
        session.update_buffs(2);session.update_buffs(2);check(session.enemies()[0].triggers.empty());
        session=base;session.cast_spell(buff,{2,1},{5,2});const auto manaBefore=session.player().mana;
        session.cast_spell(gate,{2,1},{5,2});check(session.player().mana==manaBefore); // Generic icon gate sees owned monster buffs.
        session=base;session.cast_spell(gate,{2,1},{5,2});check(session.player().mana==manaBefore-1);
        auto corrupt=saved(session);check(corrupt.find("version=\"12\"")!=std::string::npos);auto start=corrupt.find("hp-scale=\"");check(start!=std::string::npos);start+=10;corrupt.replace(start,corrupt.find('"',start)-start,"2143289344");
        std::istringstream invalid(corrupt);bool rejected=false;try {Session::load(invalid,{},{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        auto legacy=saved(session);legacy.replace(legacy.find("version=\"12\""),12,"version=\"11\"");std::istringstream downgraded(legacy);rejected=false;try {Session::load(downgraded,{},{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        auto kill=read_spell(parse_xml("<spell name='Kill' type='target'><effect type='damage' existential='100000' unblockable='1'/></spell>"));
        session=base;session.cast_spell(buff,{2,1},{5,2});session.cast_spell(kill,{2,1},{5,2});check(!session.enemies()[0].alive());
        session.update_buffs(2);session.update_buffs(2);check(session.enemies()[0].health==1 && session.enemies()[0].corpse && !session.enemies()[0].alive());
        const auto corpse=saved(session);check(corpse.find("version=\"13\"")!=std::string::npos);std::istringstream corpseInput(corpse);resumed=Session::load(corpseInput,{},{},profile);
        check(saved(resumed)==corpse && !resumed.enemies()[0].alive());session.step(Action::wait);resumed.step(Action::wait);check(saved(session)==saved(resumed) && session.enemies()[0].health==0 && !session.enemies()[0].alive());
        std::cout<<"monster profile, targeting/caps, upkeep distinction, stat removal, gates, parent cancellation and save validation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
