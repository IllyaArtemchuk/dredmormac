#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("trigger assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
Session session_with(std::shared_ptr<const std::vector<SpellDefinition>> profile,int maximum_mana=0) {
    std::istringstream map("#######\n#@....#\n#.....#\n#######\n");auto player=make_player({3,2,2});player.secondary.fill(0);player.health=player.secondary[0]=100;player.mana=0;
    player.secondary[1]=maximum_mana;player.mana=maximum_mana/2;
    Session session(World::read(map),player,1);Actor enemy;enemy.name="Fixture target";enemy.position={2,1};enemy.health=enemy.secondary[0]=100;session.spawn_enemy(enemy);
    session.spell_profile(std::move(profile));return session;
}
int main() {
    try {
        auto root=read_spell(parse_xml("<spell name='Root' type='target'><effect type='damage' slashing='1' affectsCaster='1'/><effect type='trigger' spell='Child'/><effect type='heal' amount='3'/></spell>"));
        auto child=read_spell(parse_xml("<spell name='Child' type='adjacent'><effect type='trigger' spell='Leaf'/><effect type='damage' slashing='4' affectsCaster='1'/></spell>"));
        auto leaf=read_spell(parse_xml("<spell name='Leaf' type='self'><effect type='damage' slashing='7' affectsCaster='1'/><effect type='heal' amount='2'/></spell>"));
        auto duplicate=read_spell(parse_xml("<spell name='Child' type='target'><effect type='damage' slashing='99' affectsCaster='1'/></spell>"));
        auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{child,leaf,duplicate});
        auto session=session_with(profile);check(session.cast_spell(root,{4,1},{2,1},0,false));
        check(session.player().health==95 && session.enemies()[0].health==98 && session.world().turn()==0);
        check(session.events().size()==5 && session.events()[0].target==2 && session.events()[1].target==1 && session.events()[3].target==2);
        auto missing=session_with(std::make_shared<const std::vector<SpellDefinition>>());missing.cast_spell(root,{4,1},{2,1});check(missing.enemies()[0].health==99);
        auto partial=session_with(std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{child}));partial.cast_spell(root,{4,1},{2,1});
        check(partial.enemies()[0].health==100 && partial.player().health==100);
        std::istringstream input(saved(session));auto resumed=Session::load(input,{},{},profile);
        for (int i=0;i<4;++i) {session.cast_spell(root,{4,1},{2,1});resumed.cast_spell(root,{4,1},{2,1});check(saved(session)==saved(resumed));}
        auto chance=read_spell(parse_xml("<spell name='Chance' type='target'><effect type='trigger' spell='Leaf' percent='0'/></spell>"));
        auto zero=session_with(profile);zero.cast_spell(chance,{4,1},{2,1},0,false);check(zero.player().health==95); // libc bucket0 passes percent0.
        auto cycle=read_spell(parse_xml("<spell name='Cycle' type='target'><effect type='trigger' spell='Cycle'/></spell>"));
        auto cyclic=session_with(std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{cycle}));const auto before=saved(cyclic);bool rejected=false;
        try {cyclic.cast_spell(cycle,{1,1},{2,1});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && saved(cyclic)==before && cyclic.events().empty());
        auto invalid_child=child;invalid_child.effects.push_back(read_spell_effect(parse_xml("<effect type='teleport'/>")));
        auto atomic=session_with(std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{invalid_child,leaf}));const auto untouched=saved(atomic);rejected=false;
        try {atomic.cast_spell(root,{4,1},{2,1});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(atomic)==untouched && atomic.events().empty());
        auto delayed=root;delayed.effects[1].amount=2;rejected=false;
        try {atomic.cast_spell(delayed,{4,1},{2,1});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(atomic)==untouched);
        auto mana=read_spell(parse_xml("<spell name='Mana' type='target'><effect type='spellpoints' amount='5'/></spell>"));
        auto resources=session_with(profile,30);resources.cast_spell(mana,{4,1},{3,2},2);
        check(resources.player().mana==20 && resources.enemies()[0].mana==0 && resources.events().size()==1 &&
            resources.events()[0].kind==EventKind::spell_mana && resources.events()[0].actor==2 && resources.events()[0].target==1);
        mana.effects[0].amount=-100;resources.cast_spell(mana,{4,1},{3,2});check(resources.player().mana==0);
        mana.effects[0].amount=500;resources.cast_spell(mana,{4,1},{3,2});check(resources.player().mana==30);
        auto mana_leaf=leaf;mana_leaf.effects.push_back(read_spell_effect(parse_xml("<effect type='spellpoints' amount='5'/>")));
        auto mana_profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{mana_leaf});
        auto chain=session_with(mana_profile,30);auto mana_root=read_spell(parse_xml("<spell name='Nested mana' type='target'><effect type='trigger' spell='Leaf'/></spell>"));
        chain.cast_spell(mana_root,{4,1},{3,2},2);check(chain.player().mana==20 && chain.enemies()[0].health==95);
        std::istringstream mana_input(saved(chain));auto mana_copy=Session::load(mana_input,{},{},mana_profile);
        chain.cast_spell(mana_root,{4,1},{3,2},2);mana_copy.cast_spell(mana_root,{4,1},{3,2},2);check(saved(chain)==saved(mana_copy));
        check(resolve_spellpoints(0,30,5,-1,20)==5 && resolve_spellpoints(20,30,-50,0,0)==0);
        std::cout<<"nested trigger lookup/origin/order, missing-child returns, zero chance, save/resume and cycle/unsupported rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
