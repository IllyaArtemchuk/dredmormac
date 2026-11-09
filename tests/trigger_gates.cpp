#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("trigger gate assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        const auto items=std::make_shared<const std::vector<ItemDefinition>>();
        auto active=read_spell(parse_xml("<spell name='Active' type='self'><buff time='1' icon='fixture'/></spell>"));
        auto child=read_spell(parse_xml("<spell name='Child' type='target'><effect type='spellpoints' amount='5'/></spell>"));
        const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{active,child});
        std::istringstream map("#######\n#.@...#\n#.....#\n#######\n");auto player=make_player({20,20,20});player.mana=0;
        Session base(World::read(map),player,1);base.equipment_profile({20,20,20},items);base.spell_profile(profile);
        Actor enemy;enemy.position={5,2};enemy.health=enemy.secondary[0]=1000;base.spawn_enemy(enemy);
        auto gate=read_spell(parse_xml("<spell name='Gate' type='target'><effect type='trigger' spell='Child' requireBuffOnTrigger='1' requireBuffOnTriggerName='Active'/></spell>"));
        auto session=base;session.cast_spell(gate,{2,1},{2,1});check(session.player().mana==0);
        session.cast_spell(active,{2,1},{2,1});session.cast_spell(gate,{2,1},{2,1});check(session.player().mana==5);
        session.cast_spell(gate,{5,2},{2,1},2);check(session.player().mana==5); // Immediate gates examine the caster.
        auto forbidden=gate;forbidden.effects[0].source.attributes["requireBuffOnTrigger"]="0";
        forbidden.effects[0].source.attributes["requireBuffOnNotTrigger"]="1";forbidden.effects[0].source.attributes["requireBuffOnNotTriggerName"]="Active";
        session.cast_spell(forbidden,{2,1},{2,1});check(session.player().mana==5);
        session.cast_spell(forbidden,{5,2},{2,1},2);check(session.player().mana==10);
        gate.effects[0].amount=2;session.cast_spell(gate,{5,2},{2,1},2);check(session.player().triggers.size()==1);
        const auto pending=saved(session);check(pending.find("version=\"11\"")!=std::string::npos);
        std::istringstream data(pending);auto resumed=Session::load(data,items,{},profile);
        for (int i=0;i<2;++i) {session.update_triggers(1);resumed.update_triggers(1);check(saved(session)==saved(resumed));}
        check(session.player().mana==15 && session.player().triggers.empty()); // Scheduled gates examine the owner.
        session.cast_spell(gate,{2,1},{5,2});session.update_triggers(2);session.update_triggers(2);check(session.player().mana==15 && session.enemies()[0].triggers.empty());
        session=base;session.cast_spell(active,{2,1},{2,1});session.cast_spell(gate,{2,1},{2,1});
        session.step(Action::wait);session.step(Action::wait);check(session.player_buffs().empty() && session.player().triggers.empty() && session.player().mana==0);
        auto downgraded=pending;downgraded.replace(downgraded.find("version=\"11\""),12,"version=\"9\"");std::istringstream invalid(downgraded);bool rejected=false;
        try {Session::load(invalid,items,{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        auto bad=pending;auto start=bad.find("requires=\"1\"");check(start!=std::string::npos);bad.replace(start,12,"requires=\"2\"");std::istringstream flags(bad);rejected=false;
        try {Session::load(flags,items,{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        std::cout<<"trigger buff gates: caster/owner scope, forbidden names, expiry ordering and save validation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
