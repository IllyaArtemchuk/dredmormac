#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>
#define check(value) do {if (!(value)) throw std::runtime_error("scheduled assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
SpellDefinition spell(const std::string& xml) {return read_spell(parse_xml(xml));}
Session fixture(std::shared_ptr<const std::vector<SpellDefinition>> profile) {
    std::istringstream map("#######\n#@....#\n#.....#\n#######\n");auto player=make_player({3,2,2});player.health=player.secondary[0]=100;player.secondary.fill(0);player.secondary[0]=100;player.secondary[1]=100;
    player.mana=0;Session session(World::read(map),player,1);session.spell_profile(profile);Actor enemy;enemy.position={5,2};enemy.health=enemy.secondary[0]=100;session.spawn_enemy(enemy);return session;
}
int main() {
    try {
        auto leaf=spell("<spell name='Leaf' type='target'><effect type='damage' slashing='5' affectsCaster='1' unblockable='1'/><effect type='spellpoints' amount='3'/></spell>");
        auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{leaf});
        auto root=spell("<spell name='Root' type='target'><effect type='trigger' spell='Leaf' amount='2'/></spell>");
        auto session=fixture(profile);session.cast_spell(root,{1,1},{1,1},2);check(session.player().triggers.size()==1 && session.player().health==100);
        session.update_triggers(1);check(session.player().health==100 && session.player().triggers[0].remaining==1);
        std::istringstream input(saved(session));auto resumed=Session::load(input,{},{},profile);
        session.update_triggers(1);resumed.update_triggers(1);check(saved(session)==saved(resumed));
        check(session.player().health==95 && session.player().mana==3 && session.player().triggers.empty());
        check(session.events()[0].actor==2 && session.events()[0].target==1);
        auto dot=spell("<spell name='DoT' type='target'><effect type='dot' spell='Leaf' amount='3' resistable='0'/></spell>");
        session=fixture(profile);session.cast_spell(dot,{1,1},{1,1});
        for (int i=1;i<=3;++i) {session.step(Action::wait);check(session.player().health==100-5*i && session.player().mana==3*i);}
        check(session.player().triggers.empty() && session.world().turn()==3);
        auto infinite=dot;infinite.effects[0].amount=0;session=fixture(profile);session.cast_spell(infinite,{1,1},{1,1});
        session.update_triggers(1);check(session.player().triggers[0].remaining==-1 && session.player().health==95);
        // Newly appended triggers are decremented and fired in the same pass.
        auto middle=spell("<spell name='Middle' type='target'><effect type='trigger' spell='Leaf' amount='1'/></spell>");
        auto nested=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{middle,leaf});
        auto next=root;next.effects[0].spell="Middle";next.effects[0].amount=1;
        session=fixture(nested);session.cast_spell(next,{1,1},{1,1});session.update_triggers(1);check(session.player().health==95 && session.player().triggers.empty());
        // Scheduled self-casts use the current global player origin, even for a monster caster.
        auto self=leaf;self.type="self";auto self_profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{self});
        session=fixture(self_profile);session.cast_spell(root,{1,1},{5,2},2);session.update_triggers(2);session.update_triggers(2);
        check(session.player().health==95 && session.enemies()[0].health==100);
        auto self_middle=middle;self_middle.type="self";
        auto turn_profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{self_middle,leaf});
        session=fixture(turn_profile);session.cast_spell(next,{1,1},{5,2},2);session.step(Action::wait);
        check(session.player().health==95 && session.player().triggers.empty() && session.enemies()[0].triggers.empty());
        auto cycle=spell("<spell name='Cycle' type='target'><effect type='trigger' spell='Cycle' amount='1'/></spell>");
        auto cyclic=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{cycle});session=fixture(cyclic);session.cast_spell(cycle,{1,1},{1,1});const auto before=saved(session);bool rejected=false;
        try {session.step(Action::wait);} catch(const std::runtime_error&) {rejected=true;}check(rejected && saved(session)==before);
        // Unknown owners/casters, changed rules and bad queue payloads reject on load.
        session=fixture(profile);session.cast_spell(root,{1,1},{1,1});const auto valid=saved(session);
        for (const auto& replacement:std::vector<std::pair<std::string,std::string>>{{"owner=\"1\"","owner=\"999\""},{"caster=\"1\"","caster=\"999\""},{"spell=\"0\"","spell=\"99\""},{"repeat=\"0\"","repeat=\"2\""}}) {
            auto bad=valid;bad.replace(bad.find(replacement.first),replacement.first.size(),replacement.second);std::istringstream data(bad);rejected=false;
            try {Session::load(data,{},{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        }
        rejected=false;try {session.spell_profile(self_profile);} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(session)==valid);
        auto downgraded=valid;downgraded.replace(downgraded.find("version=\"8\""),11,"version=\"7\"");std::istringstream downgrade(downgraded);rejected=false;
        try {Session::load(downgrade,{},{},profile);} catch(const std::exception&) {rejected=true;}check(rejected);
        // A blocked move consumes no turn and leaves scheduled state untouched.
        session.step(Action::north);check(session.world().turn()==0 && session.player().triggers[0].remaining==2);
        auto wrapping=root;wrapping.effects[0].amount=std::numeric_limits<std::int32_t>::min();session=fixture(profile);session.cast_spell(wrapping,{1,1},{1,1});session.update_triggers(1);
        check(session.player().triggers[0].remaining==std::numeric_limits<std::int32_t>::max());
        std::cout<<"scheduled creation, per-turn DoT, origin, same-pass append, save/resume and atomic rejection passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
