#include "dredmor/session.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

dredmor::Actor read_actor(std::uint64_t id) {
    dredmor::Actor actor;actor.id=id;actor.name=id==1?"Player":"Fixture monster";
    if (!(std::cin>>actor.health)) throw std::invalid_argument("missing trigger actor HP");
    for (auto& n:actor.primary) if (!(std::cin>>n)) throw std::invalid_argument("missing primary stat");
    for (auto& n:actor.secondary) if (!(std::cin>>n)) throw std::invalid_argument("missing secondary stat");
    for (auto& n:actor.resistance) if (!(std::cin>>n)) throw std::invalid_argument("missing resistance");
    return actor;
}
dredmor::SpellEffectDefinition damage(int amount) {
    dredmor::SpellEffectDefinition effect;effect.damage[1]=amount;effect.affects_caster=1;return effect;
}
dredmor::SpellEffectDefinition heal(int amount) {
    dredmor::SpellEffectDefinition effect;effect.type="heal";effect.amount=amount;return effect;
}
dredmor::SpellEffectDefinition trigger(std::string name,int percent,int self) {
    dredmor::SpellEffectDefinition effect;effect.type="trigger";effect.spell=std::move(name);effect.percent=percent;effect.self=self;return effect;
}
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::SpellDefinition root,child,leaf;root.name="Root";child.name="Child";leaf.name="Leaf";
            bool player_caster,allow;dredmor::Position origin,target;int percent,child_percent,present,self,child_self;
            if (!(std::cin>>std::quoted(root.type)>>std::quoted(child.type)>>std::quoted(leaf.type)>>player_caster>>origin.x>>origin.y>>target.x>>target.y
                >>allow>>percent>>child_percent>>present>>self>>child_self)) throw std::invalid_argument("incomplete trigger fixture");
            auto player=read_actor(1),monster=read_actor(2);monster.position={4,5};
            root.effects={damage(1),trigger("Child",percent,self),heal(3)};
            child.effects={trigger("Leaf",child_percent,child_self),damage(4)};leaf.effects={damage(7),heal(2)};
            std::vector<dredmor::SpellDefinition> profile;
            if (present&1) profile.push_back(child);if (present&2) profile.push_back(leaf);
            if (present&1) {auto duplicate=child;duplicate.effects={damage(99)};profile.push_back(std::move(duplicate));}
            auto definitions=std::make_shared<const std::vector<dredmor::SpellDefinition>>(std::move(profile));
            std::istringstream map("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            dredmor::Session session(dredmor::World::read(map),player,seed);session.spawn_enemy(monster);session.spell_profile(definitions);
            const auto accepted=session.cast_spell(root,origin,target,player_caster?1:2,allow);
            std::cout<<accepted<<' '<<session.player().health<<' '<<session.enemies()[0].health;
            std::size_t count=0;for (const auto& event:session.events()) if (event.kind==dredmor::EventKind::spell_damage) ++count;
            std::cout<<' '<<count;
            for (const auto& event:session.events()) if (event.kind==dredmor::EventKind::spell_damage)
                std::cout<<' '<<event.actor<<' '<<event.target<<' '<<event.amount<<' '<<event.critical<<' '<<event.blocked;
            std::ostringstream out;session.save(out);std::cout<<' '<<dredmor::parse_xml(out.str()).first("random")->attribute("libc")<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
