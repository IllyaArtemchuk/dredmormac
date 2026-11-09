#include "dredmor/room_records.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::RoomPedestalRecord definition;dredmor::Position origin;unsigned push;auto& p=definition.placement;
            std::cin>>p.percent>>origin.x>>origin.y>>p.relative.x>>p.relative.y>>push>>std::quoted(definition.name)>>std::quoted(definition.text)>>std::quoted(p.id);
            if (!std::cin || push>255) throw std::invalid_argument("invalid pedestal fixture");p.can_push=push;
            dredmor::GameRandom random;random.seed(seed);const auto object=dredmor::create_room_pedestal(definition,origin,random.libc);
            std::cout<<bool(object)<<' '<<random.libc.state();
            if (object) std::cout<<' '<<object->position.x<<' '<<object->position.y<<' '<<unsigned(object->can_push)<<' '
                <<object->blocks_movement<<' '<<object->blocks_vision<<' '<<object->frame<<' '<<object->playing<<' '<<object->looping<<' '
                <<std::quoted(object->sprite.image)<<' '<<std::quoted(object->name)<<' '<<std::quoted(object->description)<<' '<<std::quoted(object->room_local_id);
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
