#include "dredmor/session.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
dredmor::Actor actor(std::uint64_t id) {
    dredmor::Actor result;result.id=id;result.name=id==1?"Player":"Fixture monster";
    if (!(std::cin>>result.health)) throw std::invalid_argument("missing spell actor health");
    for (auto& n:result.primary) if (!(std::cin>>n)) throw std::invalid_argument("missing spell actor primary");
    for (auto& n:result.secondary) if (!(std::cin>>n)) throw std::invalid_argument("missing spell actor secondary");
    for (auto& n:result.resistance) if (!(std::cin>>n)) throw std::invalid_argument("missing spell actor resistance");
    return result;
}
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::SpellDefinition spell;dredmor::Position origin,target;bool player_caster,allow;std::size_t count;
            if (!(std::cin>>std::quoted(spell.type)>>player_caster>>origin.x>>origin.y>>target.x>>target.y>>allow>>count) || count>10000) throw std::invalid_argument("invalid cast fixture");
            auto player=actor(1),monster=actor(2);monster.position={4,5};
            for (std::size_t i=0;i<count;++i) {
                dredmor::SpellEffectDefinition effect;
                if (!(std::cin>>effect.percent>>effect.affects_caster>>effect.primary_scale>>effect.secondary_scale>>effect.unblockable)) throw std::invalid_argument("invalid cast effect");
                for (auto& n:effect.damage) if (!(std::cin>>n)) throw std::invalid_argument("missing spell damage");
                for (auto& n:effect.scaling) {std::uint32_t bits;if (!(std::cin>>bits)) throw std::invalid_argument("missing spell scaling");std::memcpy(&n,&bits,4);}
                spell.effects.push_back(std::move(effect));
            }
            std::istringstream rows("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            dredmor::Session session(dredmor::World::read(rows),player,seed);session.spawn_enemy(monster);
            const auto accepted=session.cast_damage_spell(spell,origin,target,player_caster?1:2,allow);
            std::cout<<accepted<<' '<<session.player().health<<' '<<session.enemies()[0].health;
            std::size_t events=0;for (const auto& event:session.events()) if (event.kind==dredmor::EventKind::spell_damage) ++events;
            std::cout<<' '<<events;
            for (const auto& event:session.events()) if (event.kind==dredmor::EventKind::spell_damage)
                std::cout<<' '<<event.actor<<' '<<event.target<<' '<<event.amount<<' '<<event.critical<<' '<<event.blocked;
            std::ostringstream output;session.save(output);const auto xml=dredmor::parse_xml(output.str());
            std::cout<<' '<<xml.first("random")->attribute("libc")<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
