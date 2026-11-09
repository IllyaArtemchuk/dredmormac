#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc>1) {
            if ((argc!=3 && argc!=4) || std::string(argv[1])!="--content" || (argc==4 && std::string(argv[3])!="--expansions"))
                throw std::invalid_argument("Usage: dredmor-custom-object [--content DATA_ROOT [--expansions]]");
            const auto content=dredmor::GameContent::load(argv[2],argc==4);
            for (std::size_t i=0;i<content.rooms().size();++i) for (const auto& object:content.rooms()[i].custom_objects()) {
                const auto& p=object.placement;const auto& s=object.sprite;
                std::cout<<i<<' '<<unsigned(object.kind)<<' '<<p.relative.x<<' '<<p.relative.y<<' '<<p.percent<<' '<<unsigned(p.can_push)<<' '
                    <<object.passable.has_value()<<' '<<object.passable.value_or(false)<<' '<<object.vis_block.has_value()<<' '<<object.vis_block.value_or(false)<<' '
                    <<std::quoted(s.image)<<' '<<std::quoted(s.series)<<' '<<s.first<<' '<<s.count<<' '<<s.rate<<' '
                    <<std::quoted(object.name)<<' '<<std::quoted(object.description)<<' '<<std::quoted(p.id)<<'\n';
            }
            return 0;
        }
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::CustomRoomDefinition definition;dredmor::Position origin;unsigned kind,push;
            bool pass_set,pass,vis_set,vis;
            auto& p=definition.placement;auto& s=definition.sprite;
            if (!(std::cin>>kind>>p.percent>>origin.x>>origin.y>>p.relative.x>>p.relative.y>>push>>pass_set>>pass>>vis_set>>vis
                >>std::quoted(s.image)>>std::quoted(s.series)>>s.first>>s.count>>s.rate
                >>std::quoted(definition.name)>>std::quoted(definition.description)>>std::quoted(p.id)) || kind>1 || push>255)
                throw std::invalid_argument("invalid custom object fixture");
            definition.kind=static_cast<dredmor::CustomRoomKind>(kind);p.can_push=push;
            if (pass_set) definition.passable=pass;if (vis_set) definition.vis_block=vis;
            dredmor::DarwinRandom random(seed);const auto object=dredmor::create_custom_room_object(definition,origin,random);
            std::cout<<bool(object)<<' '<<random.state();
            if (object) {
                const auto& value=*object;const auto& sprite=value.sprite;
                std::cout<<' '<<unsigned(value.kind)<<' '<<value.position.x<<' '<<value.position.y<<' '<<unsigned(value.can_push)<<' '
                    <<value.blocks_movement<<' '<<value.blocks_vision<<' '<<value.frame<<' '<<value.playing<<' '<<value.looping<<' '
                    <<std::quoted(sprite.image)<<' '<<std::quoted(sprite.series)<<' '<<sprite.first<<' '<<sprite.count<<' '<<sprite.rate<<' '
                    <<std::quoted(value.name)<<' '<<std::quoted(value.description)<<' '<<std::quoted(value.room_local_id)<<' '
                    <<(value.kind==dredmor::CustomRoomKind::blocker);
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
