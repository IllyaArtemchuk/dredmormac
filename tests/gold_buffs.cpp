#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("gold buff assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
Session fixture(std::shared_ptr<const std::vector<SpellDefinition>> spells,int wallet=10) {
    std::istringstream map("#####\n#.@.#\n#####\n");auto player=make_player({20,20,20});player.mana=0;player.zorkmids=wallet;
    Session session(World::read(map),player,1);session.equipment_profile({20,20,20},std::make_shared<const std::vector<ItemDefinition>>());session.spell_profile(std::move(spells));session.depth_context({2,false,-1});return session;
}
int main() {
    try {
        auto tax=read_spell(parse_xml("<spell name='Tax' type='self'><buff useTimer='0' time='10' icon='tax' zorkmidUpkeep='3' zorkmidScaleF='0.5' insufficientFunds='Fallback'/></spell>"));
        auto fallback=read_spell(parse_xml("<spell name='Fallback' type='self'><effect type='spellpoints' amount='5'/><buff time='1' icon='gift' zorkmidUpkeep='1'/></spell>"));
        auto manaTax=tax;manaTax.name="Mana tax";manaTax.buffs[0].mana_upkeep=1;
        auto income=tax;income.name="Income";income.buffs[0].zorkmid_scale=-.5f;
        auto missing=tax;missing.name="Missing";missing.buffs[0].insufficient_funds="Absent";
        auto loop=read_spell(parse_xml("<spell name='Loop' type='self'><effect type='trigger' spell='Loop'/></spell>"));
        auto cyclic=tax;cyclic.name="Cyclic";cyclic.buffs[0].insufficient_funds="Loop";
        const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{tax,fallback,manaTax,income,missing,loop,cyclic});
        auto session=fixture(profile);auto cast=[&](const SpellDefinition& spell){session.cast_spell(spell,session.player().position,session.player().position);};
        cast(tax);session.update_player_buffs();check(session.player().zorkmids==6 && session.player_buffs().size()==1);
        const auto pending=saved(session);check(pending.find("version=\"14\"")!=std::string::npos);std::istringstream input(pending);
        auto resumed=Session::load(input,std::make_shared<const std::vector<ItemDefinition>>(),{},profile);check(resumed.depth_context().depth==2 && saved(resumed)==pending);
        session.update_player_buffs();resumed.update_player_buffs();check(saved(session)==saved(resumed) && session.player().zorkmids==2);
        session.update_player_buffs();check(session.player().zorkmids==2 && session.player().mana==5 && session.player_buffs().size()==1 && session.player_buffs()[0].spell==1 && session.player_buffs()[0].remaining==0);
        // The callback's new buff participates in timers now; its upkeep starts next pass.
        session.update_player_buffs();check(session.player().zorkmids==2 && session.player_buffs().empty());
        session=fixture(profile);cast(manaTax);session.update_player_buffs();check(session.player().mana==0 && session.player().zorkmids==6 && session.player_buffs().empty());
        session=fixture(profile,0);cast(manaTax);session.update_player_buffs();check(session.player().mana==5 && session.player_buffs().size()==1 && session.player_buffs()[0].spell==1);
        session=fixture(profile,0);cast(income);session.update_player_buffs();check(session.player().zorkmids==5 && session.player_buffs().size()==1);
        session=fixture(profile,0);cast(missing);session.update_player_buffs();check(session.player().mana==0 && session.player_buffs().empty());
        check(buff_gold_upkeep(3,.5f,{9,true,-1})==24 && buff_gold_upkeep(3,.5f,{9,true,0})==1);
        session=fixture(profile,0);cast(cyclic);const auto before=saved(session);bool rejected=false;
        try {session.step(Action::wait);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==before);
        auto bad=pending;auto start=bad.find("alternate=\"0\"");bad.replace(start,13,"alternate=\"2\"");std::istringstream invalid(bad);rejected=false;
        try {Session::load(invalid,std::make_shared<const std::vector<ItemDefinition>>(),{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        std::cout<<"gold upkeep costs, ordered payment, callback timing, depth saves and atomic rejection passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
