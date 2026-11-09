#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            int direct,dot,count,percent,resistable,corpses,health,resist,steps;std::uint64_t caster,owner;std::string kind;
            if (!(std::cin>>direct>>dot>>count>>percent>>resistable>>corpses>>health>>resist>>steps>>caster>>owner>>std::quoted(kind))) throw std::runtime_error("incomplete scheduled fixture");
            std::istringstream map("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            Actor player;player.id=1;player.name="Player";player.health=health;player.secondary[0]=2000;player.secondary[1]=2000;player.secondary[11]=resist;
            Actor enemy;enemy.position={4,5};enemy.health=1000;enemy.secondary[0]=2000;enemy.secondary[11]=resist;
            Session session(World::read(map),player,seed);session.spawn_enemy(enemy);
            SpellDefinition child;child.name="Child";child.type=kind;
            SpellEffectDefinition heal;heal.type="heal";heal.amount=3;
            SpellEffectDefinition mana;mana.type="spellpoints";mana.amount=5;child.effects={heal,mana};
            auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{child});session.spell_profile(profile);
            if (direct) {
                std::ostringstream out;session.save(out);auto xml=out.str();const auto end=xml.find("</dredmor-session>");
                xml.replace(xml.find("version=\"7\""),11,"version=\"8\"");
                xml.insert(end,"<scheduled-spells><trigger owner='"+std::to_string(owner)+"' caster='"+std::to_string(caster)+"' spell='0' remaining='"+std::to_string(count)+"' percent='"+std::to_string(percent)+"' repeat='"+std::to_string(dot)+"' parent='Root'/></scheduled-spells>");
                std::istringstream in(xml);session=Session::load(in,{},{},profile);
            } else {
                SpellDefinition root;root.name="Root";root.type="target";
                SpellEffectDefinition trigger;trigger.type=dot?"dot":"trigger";trigger.spell="Child";trigger.amount=count;trigger.percent=percent;
                trigger.resistable=resistable;trigger.affects_corpses=corpses;root.effects={trigger};
                session.cast_spell(root,{4,4},owner==1?Position{4,4}:Position{4,5},caster);
            }
            for (int step=0;step<=steps;++step) {
                if (step) session.update_triggers(owner);
                const auto& actor=owner==1?session.player():session.enemies()[0];
                std::cout<<session.player().health<<' '<<session.enemies()[0].health<<' '<<session.player().mana<<' '<<actor.triggers.size()<<' ';
                for (const auto& trigger:actor.triggers) std::cout<<trigger.remaining<<' '<<trigger.repeat<<' '<<trigger.percent<<' '<<trigger.caster<<' ';
                std::ostringstream out;session.save(out);std::cout<<parse_xml(out.str()).first("random")->attribute("libc")<<' ';
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
