#include "dredmor/session.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            int delayed,require_gate,forbid_gate,present,percent;std::uint64_t owner,caster;std::string required,forbidden;
            if (!(std::cin>>delayed>>owner>>caster>>require_gate>>forbid_gate>>std::quoted(required)>>std::quoted(forbidden)>>present>>percent)) throw std::invalid_argument("incomplete trigger gate fixture");
            auto items=std::make_shared<const std::vector<ItemDefinition>>();
            auto active=read_spell(parse_xml("<spell name='Active' type='self'><buff time='100' icon='fixture'/></spell>"));
            auto child=read_spell(parse_xml("<spell name='Child' type='target'><effect type='spellpoints' amount='5'/></spell>"));
            auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{active,child});
            auto player=make_player({20,20,20});player.mana=0;std::istringstream map("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            Session session(World::read(map),player,seed);session.equipment_profile({20,20,20},items);session.spell_profile(profile);
            MonsterStatInput monsterInput;monsterInput.ranks={20,20,20};monsterInput.definition_secondary[0]=1000;const auto stats=derive_monster(monsterInput);
            Actor enemy;enemy.position={4,5};enemy.health=1000;enemy.primary=stats.primary;enemy.secondary=stats.secondary;enemy.stat_profile=monsterInput;session.spawn_enemy(enemy);
            if (present&1) session.cast_spell(active,{4,4},{4,4});
            if (present&2) session.cast_spell(active,{4,5},{4,5},2);
            SpellDefinition root;root.name="Root";root.type="target";SpellEffectDefinition effect;effect.type="trigger";effect.spell="Child";effect.amount=delayed?2:0;effect.percent=percent;
            effect.source.tag="effect";effect.source.attributes={{"requirebuffontrigger",std::to_string(require_gate)},{"requirebuffonnottrigger",std::to_string(forbid_gate)},{"requirebuffontriggername",required},{"requirebuffonnottriggername",forbidden}};root.effects.push_back(effect);
            session.cast_spell(root,{4,4},owner==1?Position{4,4}:Position{4,5},caster);
            // Exercise port persistence before either scheduled update.
            std::ostringstream out;session.save(out);std::istringstream input(out.str());session=Session::load(input,items,{},profile);
            for (int step=0;step<3;++step) {
                if (step) session.update_triggers(owner);
                const auto& actor=owner==1?session.player():session.enemies()[0];std::ostringstream state;session.save(state);
                std::cout<<session.player().mana<<' '<<actor.triggers.size()<<' '<<parse_xml(state.str()).first("random")->attribute("libc")<<' ';
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
