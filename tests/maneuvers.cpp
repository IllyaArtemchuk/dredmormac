#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("maneuver assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
Session session_with(std::shared_ptr<const std::vector<SpellDefinition>> spells,bool enemy=true) {
    std::istringstream rows("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
    auto player=make_player({3,2,2});player.secondary.fill(0);player.health=player.secondary[0]=100;player.mana=10;player.secondary[1]=30;
    Session session(World::read(rows),player,1);session.spell_profile(std::move(spells));
    if (enemy) {Actor target;target.name="Fixture target";target.position={4,5};target.health=target.secondary[0]=100;session.spawn_enemy(target);}
    return session;
}
int main() {
    try {
        const auto t=read_maneuver_template(parse_xml("<template width='1' height='2' name='7' affectsPlayer='1'><row text='#'/><row text='@'/></template>"));
        DarwinRandom random(1);auto unchanged=random;
        auto plan=plan_maneuver({t},7,0,2,{1,1},{4,4},random);check(plan.accepted && plan.targets.size()==2 && plan.targets[0].x==4 && plan.targets[0].y==5 && plan.targets[1].y==4 && random.state()==1);
        plan=plan_maneuver({t},7,1,0,{4,4},{1,1},random);check(plan.targets[0].x==4 && plan.targets[0].y==3);
        auto duplicate=t;duplicate.cells[1]=0;plan=plan_maneuver({t,duplicate},7,0,2,{1,1},{4,4},random);check(plan.targets.size()==1);
        auto invalid=t;invalid.cells.clear();bool rejected=false;
        try {plan_maneuver({invalid},-1,0,2,{1,1},{4,4},random);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && random.state()==unchanged.state());
        const auto padded=read_maneuver_template(parse_xml("<template width='3' height='1' name='8'><row text='#@'/></template>"));
        check(padded.cells==std::vector<std::uint8_t>({2,1,0}) && !padded.affects_player);
        auto area=read_spell(parse_xml("<spell name='Area' type='template' templateID='7'><effect type='damage' slashing='7' affectsCaster='1' unblockable='1'/></spell>"));
        area.maneuvers=std::make_shared<const std::vector<ManeuverTemplate>>(std::vector<ManeuverTemplate>{t});
        auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{area});auto session=session_with(profile);
        check(session.cast_spell(area,{1,1},{4,4}) && session.player().health==93 && session.enemies()[0].health==93);
        check(session.events().size()==2 && session.events()[0].target==2 && session.events()[1].target==1);
        session.player_facing(1);const auto before=saved(session);check(before.find(" facing=\"1\"")!=std::string::npos && before.find(" maneuvers=\"")!=std::string::npos);
        std::istringstream input(before);auto resumed=Session::load(input,{},{},profile);check(resumed.player_facing()==1 && saved(resumed)==before);
        session.cast_spell(area,{1,1},{4,4});resumed.cast_spell(area,{1,1},{4,4});check(saved(session)==saved(resumed));
        auto changed=*profile;changed[0].maneuvers=std::make_shared<const std::vector<ManeuverTemplate>>(std::vector<ManeuverTemplate>{duplicate});rejected=false;
        try {std::istringstream invalid_save(before);Session::load(invalid_save,{},{},std::make_shared<const std::vector<SpellDefinition>>(changed));} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
        auto bad_facing=before;const auto at=bad_facing.find(" facing=\"1\"");bad_facing.replace(at,11," facing=\"9\"");rejected=false;
        try {std::istringstream bad(bad_facing);Session::load(bad,{},{},profile);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected);
        auto atomic=session_with(profile);auto mixed=area;mixed.effects.push_back(read_spell_effect(parse_xml("<effect type='teleport'/>")));const auto untouched=saved(atomic);rejected=false;
        try {atomic.cast_spell(mixed,{1,1},{4,4});} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(atomic)==untouched);
        auto movement=session_with(profile,false);movement.step(Action::north);check(movement.player_facing()==2);movement.step(Action::east);check(movement.player_facing()==3);
        movement.step(Action::south);check(movement.player_facing()==0);movement.step(Action::west);check(movement.player_facing()==1);
        auto outside=session_with(profile);check(outside.cast_spell(area,{1,1},{-1,-1}) && outside.player().health==100 && outside.enemies()[0].health==100);
        std::cout<<"maneuver order/facing/lookup, missing-cell policy, area damage, atomic failures and rule/facing save identity passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
