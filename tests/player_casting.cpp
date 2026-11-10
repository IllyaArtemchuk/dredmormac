#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("player casting assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>();
        auto abilities=std::make_shared<std::vector<AbilityDefinition>>();
        abilities->push_back(read_ability(parse_xml("<ability name='Learner' skill='0' level='0'><spell name='Paid'/><spell name='Cooldown'/><spell name='Target'/><spell name='Unsupported'/><spell name='Missing'/><spell name='Paid'/><triggeroncast name='Child' percent='101'/></ability>")));
        abilities->push_back(read_ability(parse_xml("<ability name='Later' skill='0' level='1'><spell name='Child'/></ability>")));
        auto spells=std::make_shared<std::vector<SpellDefinition>>();
        for (const auto& xml:{
            "<spell name='Paid' type='self' multiAttack='2'><requirements mp='3' zorkmids='2'/><effect type='spellpoints' amount='-1'/></spell>",
            "<spell name='Cooldown' type='self' downtime='3'><requirements mp='9999' zorkmids='9999'/><effect type='spellpoints' amount='-1'/></spell>",
            "<spell name='Child' type='self'><effect type='spellpoints' amount='-2'/></spell>",
            "<spell name='Target' type='targetfloor'><effect type='spellpoints' amount='-1'/></spell>",
            "<spell name='Unsupported' type='self'><effect type='summon'/></spell>",
            "<spell name='Silence' type='self'><buff time='5' icon='mute'><mute/></buff></spell>"}) spells->push_back(read_spell(parse_xml(xml)));
        auto fixture=[&] {
            std::istringstream map("#####\n#@..#\n#...#\n#####\n");auto player=make_player({20,20,20});player.position={1,1};player.zorkmids=20;
            Session session(World::read(map),player,1);std::vector<SkillRank> skills;for (int i=0;i<7;++i) skills.push_back({i,1});
            session.character_profile({20,20,20},definitions,skills,abilities);session.spell_profile(spells);return session;
        };
        auto session=fixture();check(session.learned_spells()==std::vector<std::size_t>({0,1,3,4}));
        check(session.use_spell(2,{1,1})==SpellUse::unknown && session.world().turn()==0);
        const auto mana=session.player().mana;check(session.use_spell(0,{0,0})==SpellUse::cast);
        check(session.player().mana==mana-7 && session.player().zorkmids==18 && session.world().turn()==1);
        check(session.use_spell(1,{1,1})==SpellUse::cast && session.spell_cooldown(1)==3 && session.player().zorkmids==18 && session.player().mana==mana-10);
        const auto pending=saved(session);check(pending.find("version=\"15\"")!=std::string::npos);std::istringstream input(pending);
        auto resumed=Session::load(input,definitions,abilities,spells);check(saved(resumed)==pending);
        check(session.use_spell(1,{1,1})==SpellUse::cooling_down && saved(session)==pending);
        check(session.use_spell(3,{0,0})==SpellUse::invalid_target && saved(session)==pending);
        session.step(Action::wait);resumed.step(Action::wait);check(session.spell_cooldown(1)==2 && saved(session)==saved(resumed));
        const auto before=saved(session);bool rejected=false;try {session.use_spell(4,{1,1});} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==before);
        auto changed=std::make_shared<std::vector<SpellDefinition>>(*spells);changed->at(1).casting.downtime=4;std::istringstream mismatch(pending);rejected=false;
        try {Session::load(mismatch,definitions,abilities,changed);} catch(const std::exception&) {rejected=true;}check(rejected);
        auto invalid=pending;auto at=invalid.find("remaining=\"3\"");invalid.replace(at,13,"remaining=\"0\"");std::istringstream malformed(invalid);rejected=false;
        try {Session::load(malformed,definitions,abilities,spells);} catch(const std::exception&) {rejected=true;}check(rejected);
        session=fixture();session.cast_spell(spells->at(5),{1,1},{1,1});check(session.use_spell(0,{1,1})==SpellUse::muted && session.world().turn()==0);
        check(session.use_spell(1,{1,1})==SpellUse::cast && session.spell_cooldown(1)==3);
        session=fixture();session.take_currency(100);const auto failed=saved(session);check(session.use_spell(0,{1,1})==SpellUse::insufficient_gold && session.world().turn()==0 && saved(session)!=failed);
        session=fixture();check(session.use_spell(3,{3,2})==SpellUse::cast && session.player_facing()==3 && session.world().turn()==1);
        std::cout<<"learned spells, resources, multi-cast, strict on-cast triggers, silence, cooldown turns/saves and rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
