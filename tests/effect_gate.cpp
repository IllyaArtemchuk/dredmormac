#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("effect gate assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        auto active=read_spell(parse_xml("<spell name='Active' type='self'><buff time='10' icon='fixture'/></spell>"));
        auto root=read_spell(parse_xml("<spell name='Root' type='self'><effect type='spellpoints' amount='5' requireBuff='1'/><effect type='spellpoints' amount='7'/><buff time='0' allowstacking='0' icon='fixture'/></spell>"));
        auto dual=root;dual.name="Dual";auto other=dual.buffs[0];other.icon="other";dual.buffs.push_back(other);
        auto ineligible=root;ineligible.name="Ineligible";ineligible.buffs[0].apply_to_player=0;
        const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{active,root,dual,ineligible});
        const auto items=std::make_shared<const std::vector<ItemDefinition>>();auto player=make_player({20,20,20});player.mana=0;
        std::istringstream map("#######\n#.@...#\n#.....#\n#######\n");Session base(World::read(map),player,1);base.equipment_profile({20,20,20},items);base.spell_profile(profile);
        Actor enemy;enemy.position={5,2};enemy.health=enemy.secondary[0]=1000;base.spawn_enemy(enemy);
        auto session=base;auto cast=[&](const SpellDefinition& spell){session.cast_spell(spell,session.player().position,session.player().position);};
        cast(root);check(session.player().mana==12 && session.player_buffs().size()==1);
        std::ostringstream out;session.save(out);std::istringstream input(out.str());session=Session::load(input,items,{},profile);
        cast(root);check(session.player().mana==12 && session.player_buffs().size()==1); // Whole effect loop returns.
        session.update_player_buffs();cast(root);check(session.player().mana==24);
        session=base;cast(active);cast(root);check(session.player().mana==0 && session.player_buffs().size()==1 && session.player_buffs()[0].spell==0);
        auto failed_chance=root;failed_chance.effects[0].percent=-1;cast(failed_chance);check(session.player().mana==7);
        session=base;cast(active);cast(dual);check(session.player().mana==12 && session.player_buffs().size()==2); // Any missing icon permits it.
        auto empty=root;empty.name="Empty";empty.buffs.clear();session=base;cast(empty);check(session.player().mana==0 && session.player_buffs().empty());
        session=base;cast(ineligible);check(session.player().mana==0 && session.player_buffs().size()==1); // Self buff application still runs.
        session=base;cast(active);auto target=root;target.type="target";session.cast_spell(target,{2,1},{5,2});check(session.player().mana==12);
        session=base;target.buffs[0].apply_to_player=0;target.name="Ineligible";session.cast_spell(target,{2,1},{5,2});check(session.player().mana==0 && session.player_buffs().empty());
        std::cout<<"effect buff gate: icons, any-missing condition, chance order, whole-loop return and subsequent buff application passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
