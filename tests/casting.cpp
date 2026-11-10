#include "dredmor/spells.hpp"
#include "dredmor/abilities.hpp"
#include "dredmor/selection.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("casting assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        const auto spell=read_spell(parse_xml("<spell name='Fixture' type='self' downtime='3' multiAttack='2' noAnimation='1' attack='1'><requirements mp='10' mincost='2' hp='5' level='4' booze='6' weapon='0' shield='1' zorkmids='3' magPowBonus='0.5' savvyBonus='0.25' zorkmidScaleF='0.5'/></spell>"));
        const auto& casting=spell.casting;
        check(casting.mana==10 && casting.minimum_mana==2 && casting.health==5 && casting.level==4 && casting.booze==6 && casting.weapon==0 && casting.shield==1 && casting.gold==3);
        check(casting.cooldown && casting.no_animation && casting.attack && casting.multi_attack==2 && casting.downtime==3);
        check(spell_mana_cost(casting,4,5)==7 && spell_gold_cost(casting,5,{2,false,-1})==3);
        check(spell_gold_cost(casting,0,{-1,false,-1})==1 && buff_gold_upkeep(3,.5f,{-1,false,-1})==24);
        auto pathological=casting;pathological.magic_bonus=std::numeric_limits<float>::quiet_NaN();check(spell_mana_cost(pathological,4,5)==2);
        const auto ability=read_ability(parse_xml("<ability name='Fixture' skill='0' level='0'><spell name='First'/><Spell NAME='Second'/><triggerOnCast name='Child'/></ability>"));
        check(ability.spells==std::vector<std::string>({"First","Second"}) && ability.cast_trigger=="Child" && ability.cast_trigger_percent==5);
        const auto zero=read_ability(parse_xml("<ability name='Fixture'><triggerOnCast name='Child' percentage='0'/></ability>"));check(zero.cast_trigger_percent==0);
        const auto defaults=read_spell(parse_xml("<spell name='Fixture' type='self' downtime='-1'/>"));check(!defaults.casting.cooldown && defaults.casting.weapon==-1 && defaults.casting.gold_scale==1);
        std::cout<<"spell casting requirements, scaling, gold depth, cooldown metadata and ability references passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
