#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>
#define check(v) do {if (!(v)) throw std::runtime_error("regeneration assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        RegenerationState state{2,3,0,0,2,2,false};std::int32_t hp=5,mp=5;
        regenerate(state,hp,10,mp,10,0,0);check(hp==6 && mp==6 && state.food==1 && state.booze==1);
        regenerate(state,hp,10,mp,10,0,0);check(hp==8 && mp==7 && state.health_clock==0 && state.mana_clock==2);
        regenerate(state,hp,10,mp,10,0,0);check(hp==8 && mp==8 && state.mana_clock==0);
        state={1,1,0,0,1,1,true};regenerate(state,hp,10,mp,10,0,0);check(hp==9 && mp==10);
        state={2,2,2,2,1,1,false};regenerate(state,hp,10,mp,10,0,0,false);check(hp==10 && mp==10 && state.food==1 && state.booze==1 && state.health_clock==0);
        state={2,2,std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max(),0,0,false};hp=0;mp=0;regenerate(state,hp,10,mp,10,0,0);check(hp==0 && mp==0 && state.health_clock==std::numeric_limits<std::int32_t>::min());
        auto items=std::make_shared<const std::vector<ItemDefinition>>();auto spells=std::make_shared<std::vector<SpellDefinition>>();
        spells->push_back(read_spell(parse_xml("<spell name='Drain' type='self'><effect type='spellpoints' amount='-5'/><effect type='heal' amount='-5'/></spell>")));
        spells->push_back(read_spell(parse_xml("<spell name='Cooldown' type='self' downtime='2'/>")));
        spells->push_back(read_spell(parse_xml("<spell name='Residue' type='self'><buff time='0' icon='residue'><damagebuff slashing='2'/></buff></spell>")));
        auto abilities=std::make_shared<std::vector<AbilityDefinition>>();abilities->push_back(read_ability(parse_xml("<ability name='Caster' skill='0'><spell name='Drain'/><spell name='Cooldown'/></ability>")));
        auto fixture=[&](bool vampire=false) {
            std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({2,2,2}),1);std::vector<SkillRank> skills;for (int i=0;i<7;++i) skills.push_back({i,1});session.character_profile({2,2,2},items,skills,abilities);session.spell_profile(spells);session.regeneration_profile(3+session.player().secondary[13],4+session.player().secondary[14],vampire);return session;
        };
        auto session=fixture();const auto hp_max=session.player().health,mp_max=session.player().mana;
        session.cast_spell(spells->at(0),{1,1},{1,1});session.step(Action::wait);session.step(Action::wait);
        check(session.player().health==hp_max-5 && session.player().mana==mp_max-5);
        const auto pending=saved(session);check(pending.find("version=\"16\"")!=std::string::npos);std::istringstream input(pending);auto resumed=Session::load(input,items,abilities,spells);check(saved(resumed)==pending);
        session.step(Action::wait);resumed.step(Action::wait);check(session.player().health==hp_max-4 && session.player().mana==mp_max-5 && saved(session)==saved(resumed));
        const auto before=saved(session);session.step(Action::north);check(saved(session)==before);
        session.step(Action::wait);check(session.player().mana==mp_max-4);
        session=fixture(true);session.cast_spell(spells->at(0),{1,1},{1,1});for (int i=0;i<4;++i) session.step(Action::wait);check(session.player().health==hp_max-5 && session.player().mana==mp_max-4);
        session=fixture();session.step(Action::wait);session.step(Action::wait);session.step(Action::wait);check(session.regeneration()->mana_clock==3);
        check(session.use_spell(0,{1,1})==SpellUse::cast && session.regeneration()->mana_clock==1 && session.player().mana==mp_max-5);
        session.depth_context({2,false,-1});check(session.use_spell(1,{1,1})==SpellUse::cast && session.spell_cooldown(1)==2);
        std::istringstream combined(saved(session));auto loaded=Session::load(combined,items,abilities,spells);check(saved(loaded)==saved(session) && loaded.depth_context().depth==2 && loaded.regeneration()->mana_clock==1);
        session=fixture();const auto damage=session.player().damage[1];session.cast_spell(spells->at(2),{1,1},{1,1});session.step(Action::wait);
        check(session.player_buffs().empty() && session.player().damage[1]==damage+2);session.step(Action::wait);check(session.player().damage[1]==damage);
        auto invalid=pending;auto start=invalid.find("vampire=\"0\"");invalid.replace(start,11,"vampire=\"2\"");std::istringstream bad(invalid);bool rejected=false;try {Session::load(bad,items,abilities,spells);} catch(const std::exception&) {rejected=true;}check(rejected);
        std::cout<<"food/booze arithmetic, natural clocks, time gate, Vampirism, spell reset, blocked actions and session saves passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
