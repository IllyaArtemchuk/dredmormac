#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
dredmor::Actor actor(std::uint64_t id) {
    dredmor::Actor result;result.id=id;result.name=id==1?"Player":"Fixture monster";
    if (!(std::cin>>result.health>>result.mana)) throw std::invalid_argument("missing maneuver actor resources");
    for (auto& n:result.primary) if (!(std::cin>>n)) throw std::invalid_argument("missing primary");
    for (auto& n:result.secondary) if (!(std::cin>>n)) throw std::invalid_argument("missing secondary");
    for (auto& n:result.resistance) if (!(std::cin>>n)) throw std::invalid_argument("missing resistance");return result;
}
int main() {
    try {
        std::uint32_t seed;int id,anchored,facing;dredmor::Position origin,target;bool player_caster;std::size_t count;
        while (std::cin>>seed>>id>>anchored>>facing>>origin.x>>origin.y>>target.x>>target.y>>player_caster>>count) {
            if (count>1000) throw std::invalid_argument("too many maneuver templates");std::vector<dredmor::ManeuverTemplate> templates(count);
            for (auto& t:templates) {
                if (!(std::cin>>t.width>>t.height>>t.id>>t.affects_player) || t.width<1 || t.height<1 || t.width>256 || t.height>256) throw std::invalid_argument("invalid maneuver template");
                t.cells.resize(std::size_t(t.width)*t.height);for (auto& cell:t.cells) {unsigned n;if (!(std::cin>>n) || n>255) throw std::invalid_argument("invalid maneuver cell");cell=n;}
            }
            auto player=actor(1),monster=actor(2);monster.position={4,5};
            auto root=dredmor::read_spell(dredmor::parse_xml("<spell name='Root' type='template'><effect type='damage' slashing='7' affectsCaster='1'/><effect type='heal' amount='2'/><effect type='spellpoints' amount='3'/><effect type='trigger' spell='Child' percent='50'/></spell>"));
            root.source.attributes["templateID"]=std::to_string(id);root.source.attributes["anchored"]=std::to_string(anchored);
            root.maneuvers=std::make_shared<const std::vector<dredmor::ManeuverTemplate>>(std::move(templates));
            auto child=dredmor::read_spell(dredmor::parse_xml("<spell name='Child' type='targetfloor'><effect type='damage' slashing='2' affectsCaster='1'/></spell>"));
            std::istringstream rows("#########\n#.......#\n#.......#\n#.......#\n#...@...#\n#.......#\n#.......#\n#.......#\n#########\n");
            dredmor::Session session(dredmor::World::read(rows),player,seed);session.spawn_enemy(monster);session.player_facing(facing);
            session.spell_profile(std::make_shared<const std::vector<dredmor::SpellDefinition>>(std::vector<dredmor::SpellDefinition>{root,child}));
            const auto accepted=session.cast_spell(root,origin,target,player_caster?1:2,false);
            std::cout<<accepted<<' '<<session.player().health<<' '<<session.enemies()[0].health<<' '<<session.player().mana<<' '<<session.enemies()[0].mana;
            std::size_t count=0;for (const auto& e:session.events()) if (e.kind==dredmor::EventKind::spell_damage) ++count;std::cout<<' '<<count;
            for (const auto& e:session.events()) if (e.kind==dredmor::EventKind::spell_damage) std::cout<<' '<<e.actor<<' '<<e.target<<' '<<e.amount<<' '<<e.critical<<' '<<e.blocked;
            std::ostringstream out;session.save(out);std::cout<<' '<<dredmor::parse_xml(out.str()).first("random")->attribute("libc")<<'\n';
        }
        return std::cin.eof()?0:1;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
