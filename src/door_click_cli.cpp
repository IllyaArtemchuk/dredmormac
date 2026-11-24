#include "dredmor/doors.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::uint32_t seed;bool horizontal,opened,locked;std::size_t selected;unsigned mask;
        while (std::cin>>seed>>horizontal>>opened>>locked>>selected) {
            DoorState door;door.assembly={{4,4},horizontal,locked,opened,{}};DoorClickContext context;
            std::cin>>context.player.x>>context.player.y>>context.free_pick>>context.burglary>>context.female
                    >>context.depth>>context.lockpick_xp>>context.stealth>>mask;
            for (auto& kicks:door.kicks) std::cin>>kicks;
            DungeonLayout layout(9,9,std::vector<DungeonCell>(81));
            for (int i=0;i<3;++i) {
                auto p=Position{horizontal?3+i:4,horizontal?4:3+i};door.assembly.parts[i]=p;
                unsigned kind,flags;std::cin>>kind>>flags;layout.at(p.x,p.y).kind=kind;layout.at(p.x,p.y).door_flags=flags;
            }
            for (auto& pick:context.belt_picks) std::cin>>pick;
            for (auto& pick:context.bag_picks) std::cin>>pick;
            std::size_t count;std::cin>>count;context.listeners.resize(count);
            for (auto& actor:context.listeners) {unsigned kind;std::cin>>actor.position.x>>actor.position.y>>actor.flags>>kind;actor.tile_kind=kind;}
            if (!std::cin) throw std::runtime_error("truncated door click fixture");
            context.attack_sound="kick";GameRandom random;random.seed(seed);std::vector<std::size_t> checks;
            auto result=click_door(layout,door,selected,context,random,[&](Position p) {
                for (std::size_t i=0;i<3;++i) if (p.x==door.assembly.parts[i].x && p.y==door.assembly.parts[i].y) {checks.push_back(i);return (mask&(1U<<i))!=0;}
                throw std::runtime_error("unexpected obstruction point");
            });
            auto emit=[](auto value) {std::cout<<value<<' ';};
            emit(result.handled);emit(door.assembly.open);emit(door.assembly.locked);emit(door.destroyed);
            for (auto kicks:door.kicks) emit(kicks);
            for (auto p:door.assembly.parts) {emit(unsigned(layout.at(p.x,p.y).kind));emit(unsigned(layout.at(p.x,p.y).door_flags));}
            for (auto picks:context.belt_picks) emit(picks);
            for (auto picks:context.bag_picks) emit(picks);
            for (auto actor:context.listeners) emit(actor.flags);
            emit(result.animation_facing.value_or(-1));emit(result.eye_candy);emit(bool(result.experience));emit(result.experience.value_or(0));
            emit(bool(result.damage));emit(std::quoted(result.damage?result.damage->death_reason:""));
            emit(result.destroyed_parts.size());for (auto part:result.destroyed_parts) emit(part);
            emit(result.destroyed_pick?int(*result.destroyed_pick):-1);
            emit(result.messages.size());for (const auto& message:result.messages) emit(std::quoted(message));
            emit(result.sounds.size());for (const auto& sound:result.sounds) emit(std::quoted(sound));
            emit(checks.size());for (auto part:checks) emit(part);
            emit(random.libc.state());const auto mt=random.mt.state();emit(mt.index);for (auto word:mt.words) emit(word);
            for (int i=0;i<8;++i) emit(random.mt.u32());std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
