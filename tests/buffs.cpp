#include "dredmor/buffs.hpp"
#include "dredmor/spells.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("buff assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        const auto defaults=read_buff(parse_xml("<buff/>"));
        check(defaults.use_timer && defaults.allow_stacking && defaults.self && defaults.time==0 && defaults.requires_weapon==-1);
        const auto buff=read_buff(parse_xml("<BuFf TIME=' 5tail' allowStacking='0' self='-1' stacksize='2' manaUpkeep='4' tag='fixture' zorkmidScaleF='0.5tail'><primaryBuff id='0' amount='3'/><secondarybuff id='18' amount='4'/><sightbuff amount='9'/><resistbuff toxic='2'/><damagebuff slashing='7'/><invisible/><mute/><halo name='retained'/></BuFf>"));
        check(buff.time==5 && !buff.allow_stacking && buff.self && buff.stack_size==2 && buff.mana_upkeep==4 && buff.tag=="fixture" && buff.zorkmid_scale==.5f);
        check(buff.modifiers.primary[0]==3 && buff.modifiers.secondary[18]==9 && buff.modifiers.resistance[7]==2 && buff.modifiers.damage[1]==7 && buff.invisible && buff.mute && buff.source.children.back().tag=="halo");
        const auto spell=read_spell(parse_xml("<spell name='Fixture' type='self'><buff time='2'/><buff time='3'/></spell>"));check(spell.buffs.size()==2 && spell.buffs[1].time==3);
        auto tick=tick_buff({1,false,true,false});check(tick.clock.remaining==0 && !tick.deapply && !tick.remove);
        tick=tick_buff(tick.clock);check(tick.clock.remaining==-1 && tick.deapply && tick.remove);
        tick=tick_buff({0,false,false,false});check(tick.clock.remaining==-1 && !tick.deapply && !tick.remove);
        tick=tick_buff({0,true,true,false});check(tick.clock.remaining==0 && !tick.deapply && !tick.remove);
        tick=tick_buff({0,true,true,true});check(tick.clock.remaining==0 && !tick.deapply && tick.remove);
        tick=tick_buff({std::numeric_limits<std::int32_t>::min(),false,true,false});check(tick.clock.remaining==std::numeric_limits<std::int32_t>::max() && !tick.remove);
        bool rejected=false;try {read_buff(parse_xml("<buff><primarybuff id='6'/></buff>"));} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        const auto order=buff_order(std::vector<std::string>(17,"Same"));
        check(order==std::vector<std::size_t>({16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}));
        std::cout<<"buff metadata, modifier overrides, retained source, timer flags and wrapping passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
