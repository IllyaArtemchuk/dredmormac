#include "dredmor/room_records.hpp"
#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc>1) {
            if ((argc!=3 && argc!=4) || std::string(argv[1])!="--content" || (argc==4 && std::string(argv[3])!="--expansions"))
                throw std::invalid_argument("Usage: dredmor-room-element [--content DATA_ROOT [--expansions]]");
            const auto content=dredmor::GameContent::load(argv[2],argc==4);
            for (std::size_t i=0;i<content.rooms().size();++i) for (const auto& element:content.rooms()[i].elements()) {
                const auto& place=element.placement;
                std::cout<<i<<' '<<std::quoted(element.type)<<' '<<place.relative.x<<' '<<place.relative.y<<' '<<place.percent<<' '
                    <<unsigned(place.can_push)<<' '<<std::quoted(place.id)<<' '<<std::quoted(element.name)<<' '<<std::quoted(element.contents)<<'\n';
            }
            return 0;
        }
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::RoomElementDefinition definition;dredmor::Position origin;
            if (!(std::cin>>definition.placement.percent>>origin.x>>origin.y>>definition.placement.relative.x>>definition.placement.relative.y
                >>std::quoted(definition.type)>>std::quoted(definition.name)>>std::quoted(definition.contents))) throw std::invalid_argument("invalid element fixture");
            dredmor::DarwinRandom random(seed);const auto plan=dredmor::plan_room_element(definition,origin,random);
            std::cout<<plan.accepted<<' '<<unsigned(plan.kind)<<' '<<plan.variant<<' '<<plan.position.x<<' '<<plan.position.y<<' '
                <<std::quoted(plan.name)<<' '<<std::quoted(plan.contents)<<' '<<random.state()<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
