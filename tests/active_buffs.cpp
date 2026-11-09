#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("active buff assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        const auto items=std::make_shared<const std::vector<ItemDefinition>>();
        auto root=read_spell(parse_xml("<spell name='Buff' type='self'><effect type='trigger' spell='Leaf' amount='3'/><buff time='1' manaUpkeep='2' allowstacking='0' icon='fixture'><primarybuff id='0' amount='2'/><secondarybuff id='0' amount='5'/><damagebuff slashing='7'/><resistbuff toxic='3'/></buff></spell>"));
        auto leaf=read_spell(parse_xml("<spell name='Leaf' type='target'><effect type='spellpoints' amount='-1'/></spell>"));
        auto rebuild=read_spell(parse_xml("<spell name='Rebuild' type='self'><buff time='0' icon='other'/></spell>"));
        auto stacked=root;stacked.name="Stack";stacked.effects.clear();stacked.buffs[0].allow_stacking=true;stacked.buffs[0].stack_size=2;stacked.buffs[0].icon="stack";
        auto sustained=stacked;sustained.name="Sustain";sustained.buffs[0].use_timer=false;sustained.buffs[0].mana_upkeep=1;sustained.buffs[0].icon="mana";
        auto large=stacked;large.name="Large";auto& largeBuff=large.buffs[0];largeBuff.time=0;largeBuff.stack_size=0;largeBuff.mana_upkeep=0;largeBuff.icon="large";
        largeBuff.modifiers={};largeBuff.modifiers.primary[0]=1;
        auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{root,leaf,rebuild,stacked,sustained,large});
        auto player=make_player({20,20,20});player.health-=20;player.mana-=10;
        std::istringstream map("#######\n#.@...#\n#.....#\n#######\n");Session session(World::read(map),player,1);session.equipment_profile({20,20,20},items);session.spell_profile(profile);
        const auto baseline=session;const auto initial=session.player();auto cast=[&](const SpellDefinition& spell){session.cast_spell(spell,session.player().position,session.player().position);};
        cast(root);check(session.player_buffs().size()==1 && session.player().primary[0]==initial.primary[0]+2 && session.player().secondary[0]==initial.secondary[0]+7 && session.player().health==initial.health+5);
        check(session.player().damage[1]==7 && session.player().resistance[7]==3);
        auto legacy=saved(session);legacy.replace(legacy.find("version=\"11\""),12,"version=\"9\"");
        for (const std::string tag:{"buff-primary-flat","buff-secondary-flat"}) {
            const auto first=legacy.find("<"+tag+">"),last=legacy.find("</"+tag+">",first);check(first!=std::string::npos && last!=std::string::npos);
            legacy.erase(first,last+tag.size()+3-first);
        }
        std::istringstream legacyInput(legacy);auto migrated=Session::load(legacyInput,items,{},profile);check(saved(migrated)==saved(session));
        session.step(Action::wait);check(session.player_buffs()[0].remaining==0 && session.player_buffs()[0].upkeep_remaining==1);
        std::istringstream input(saved(session));auto resumed=Session::load(input,items,{},profile);check(saved(session)==saved(resumed));
        cast(root);check(session.player_buffs().size()==1 && session.player_buffs()[0].remaining==1 && session.player_buffs()[0].upkeep_remaining==1 && session.player().health==initial.health+5);
        session.step(Action::wait);session.step(Action::wait);
        check(session.player_buffs().empty() && session.player().primary==initial.primary && session.player().secondary==initial.secondary);
        check(session.player().health==initial.health && session.player().damage[1]==7 && session.player().resistance[7]==3);
        check(session.player().triggers.empty()); // Expiry cancels triggers with this parent name.
        check(session.player().mana==initial.mana-1);
        cast(rebuild);check(session.player().damage[1]==0 && session.player().resistance[7]==0);
        cast(stacked);cast(stacked);cast(stacked);check(session.player_buffs().size()==3 && session.player().damage[1]==14);
        const auto before=saved(session);auto changed=*profile;changed[3].buffs[0].modifiers.primary[0]++;
        bool rejected=false;std::istringstream wrong(before);try {Session::load(wrong,items,{},std::make_shared<const std::vector<SpellDefinition>>(changed));} catch(const std::exception&) {rejected=true;}check(rejected);
        auto unsupported=root;unsupported.buffs[0].invisible=true;rejected=false;
        try {cast(unsupported);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==before);
        auto corrupt=before;auto start=corrupt.find("<buff spell=\"");check(start!=std::string::npos);start+=std::string("<buff spell=\"").size();corrupt.replace(start,corrupt.find('"',start)-start,"999");
        std::istringstream invalid(corrupt);rejected=false;try {Session::load(invalid,items,{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        session=baseline;cast(sustained);
        auto drain=leaf;drain.type="self";drain.effects[0].amount=-10000;cast(drain);check(session.player().mana==0);
        session.update_player_buffs();check(session.player_buffs().empty() && session.player().health==initial.health && session.player().mana==0);
        session=baseline;for (int i=0;i<24;++i) cast(large);
        check(session.player().primary[0]==initial.primary[0]+24);
        session.update_player_buffs();check(session.player_buffs().size()==12 && session.player().primary[0]==initial.primary[0]);
        std::istringstream midway(saved(session));resumed=Session::load(midway,items,{},profile);
        session.update_player_buffs();resumed.update_player_buffs();check(saved(session)==saved(resumed));
        check(session.player_buffs().empty() && session.player().primary[0]==initial.primary[0]-12);
        // Original sorting during expiry leaves an independent flat modifier
        // even after the last buff disappears. It survives saves/rebuilding.
        const auto ghost=saved(session);check(ghost.find("version=\"11\"")!=std::string::npos);
        std::istringstream ghostInput(ghost);resumed=Session::load(ghostInput,items,{},profile);cast(rebuild);resumed.cast_spell(rebuild,resumed.player().position,resumed.player().position);
        check(saved(session)==saved(resumed) && session.player().primary[0]==initial.primary[0]-12);
        std::cout<<"active stat buffs, refresh/caps, legacy expiry, parent cancellation, save/resume and rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
