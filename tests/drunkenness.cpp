#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("drunkenness assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        const auto mixed=read_spell(parse_xml("<spell name='Mixed' type='self'><buff/><requirements booze='3'/><buff/><requirements booze='0'/><buff/></spell>"));
        check(mixed.casting.booze==0 && mixed.buffs[0].drunken && mixed.buffs[1].drunken && !mixed.buffs[2].drunken);
        auto items=std::make_shared<std::vector<ItemDefinition>>();items->push_back(read_item(parse_xml("<item name='Drink'><food mp='8'/></item>")));
        auto spells=std::make_shared<std::vector<SpellDefinition>>();
        for (int i=1;i<=3;++i) spells->push_back(read_spell(parse_xml("<spell name='Drunk "+std::to_string(i)+"' type='self'><requirements booze='"+std::to_string(i*2)+"'/><buff useTimer='0' allowstacking='0' stacksize='1' icon='icon"+std::to_string(i)+"'><primarybuff id='0' amount='"+std::to_string(i)+"'/><damagebuff slashing='"+std::to_string(i)+"'/></buff></spell>")));
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({20,20,20}),1);session.equipment_profile({20,20,20},items);session.spell_profile(spells);session.regeneration_profile(1000,1000);
        const auto primary=session.player().primary[0],damage=session.player().damage[1];const auto drink=session.create_ground_item(0,{1,1});session.pickup();check(session.eat_food(drink));
        check(session.regeneration()->booze==7 && session.player_buffs().size()==1 && session.player_buffs()[0].spell==2 && session.player_buffs()[0].remaining==0 && session.player().primary[0]==primary+3);
        const auto pending=saved(session);std::istringstream input(pending);auto resumed=Session::load(input,items,{},spells);check(saved(resumed)==pending);
        for (int booze=6;booze>=0;--booze) {
            session.step(Action::wait);resumed.step(Action::wait);check(saved(session)==saved(resumed) && session.regeneration()->booze==booze);
            const auto tier=booze>=6?3:booze>=4?2:booze>=2?1:0;
            check(session.player().primary[0]==primary+tier && session.player_buffs().size()==std::size_t(tier?1:0));
            if (tier) check(session.player_buffs()[0].remaining==0 && session.player().damage[1]==damage+tier);
            else check(session.player().damage[1]==damage+(booze==1?1:0));
        }
        auto changed=std::make_shared<std::vector<SpellDefinition>>(*spells);changed->at(0).buffs[0].drunken=false;std::istringstream mismatch(pending);bool rejected=false;try {Session::load(mismatch,items,{},changed);} catch(const std::exception&) {rejected=true;}check(rejected);
        std::cout<<"booze threshold selection, parser flag propagation, tier replacement, expiry residue and saved status identity passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
