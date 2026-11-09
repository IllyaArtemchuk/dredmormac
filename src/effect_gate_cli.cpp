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
            std::string kind;Position origin,target;int count,eligible,present,percent;
            if (!(std::cin>>std::quoted(kind)>>origin.x>>origin.y>>target.x>>target.y>>count>>eligible>>present>>percent) || count<0 || count>2) throw std::invalid_argument("incomplete effect gate fixture");
            auto active=read_spell(parse_xml("<spell name='Active' type='self'><buff time='100' icon='fixture'/></spell>"));
            SpellDefinition root;root.name="Root";root.type=kind;
            for (int i=0;i<count;++i) {BuffDefinition buff;buff.icon=i?"other":"fixture";buff.apply_to_player=eligible;root.buffs.push_back(buff);}
            SpellEffectDefinition first;first.type="spellpoints";first.amount=5;first.percent=percent;first.source.tag="effect";first.source.attributes["requirebuff"]="1";
            SpellEffectDefinition last;last.type="spellpoints";last.amount=7;root.effects={first,last};
            const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{active,root});const auto items=std::make_shared<const std::vector<ItemDefinition>>();
            auto player=make_player({20,20,20});player.mana=0;std::istringstream map("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            Session session(World::read(map),player,seed);session.equipment_profile({20,20,20},items);session.spell_profile(profile);
            Actor enemy;enemy.position={4,5};enemy.health=enemy.secondary[0]=1000;session.spawn_enemy(enemy);
            if (present) session.cast_spell(active,{4,4},{4,4});
            const auto accepted=session.cast_spell(root,origin,target);std::ostringstream out;session.save(out);
            std::cout<<accepted<<' '<<session.player().mana<<' '<<parse_xml(out.str()).first("random")->attribute("libc")<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
