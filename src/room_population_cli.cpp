#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <type_traits>
int main(int argc,char** argv) {
    try {
        if (argc>1) {
            if (argc!=3 || std::string(argv[1])!="--content") throw std::invalid_argument("Usage: dredmor-room-population [--content DATA_ROOT]");
            const auto content=dredmor::GameContent::load(argv[2],true);
            for (std::size_t i=0;i<content.rooms().size();++i) for (const auto& definition:content.rooms()[i].population()) {
                std::visit([&](const auto& record) {
                    const auto& p=record.placement;
                    std::cout<<i<<' '<<definition.index()<<' '<<p.relative.x<<' '<<p.relative.y<<' '<<p.percent<<' '
                        <<unsigned(p.can_push)<<' '<<std::quoted(p.id);
                    using Record=std::decay_t<decltype(record)>;
                    if constexpr (std::is_same_v<Record,dredmor::RoomLootRecord>) std::cout<<' '<<std::quoted(record.type)<<' '<<std::quoted(record.subtype)<<' '<<record.amount<<' '<<record.artifact;
                    else {
                        std::cout<<' '<<std::quoted(record.name);
                        if constexpr (std::is_same_v<Record,dredmor::RoomHordeRecord>) std::cout<<' '<<record.minimum<<' '<<record.additional;
                    }
                    std::cout<<'\n';
                },definition);
            }
            return 0;
        }
        std::uint32_t seed;
        while (std::cin>>seed) {
            unsigned kind;dredmor::RoomObjectPlacement placement;dredmor::Position origin;
            std::string name,type,subtype;int amount,minimum,additional;bool artifact;
            if (!(std::cin>>kind>>placement.percent>>origin.x>>origin.y>>placement.relative.x>>placement.relative.y
                >>std::quoted(name)>>std::quoted(type)>>std::quoted(subtype)>>amount>>minimum>>additional>>artifact))
                throw std::invalid_argument("invalid population fixture");
            dredmor::RoomPopulationRecord record;
            if (kind==0) record=dredmor::RoomMonsterRecord{placement,name};
            else if (kind==1) record=dredmor::RoomTrapRecord{placement,name};
            else if (kind==2) record=dredmor::RoomLootRecord{placement,type,subtype,amount,artifact};
            else if (kind==3) record=dredmor::RoomHordeRecord{placement,name,minimum,additional};
            else throw std::invalid_argument("invalid population kind");
            dredmor::DarwinRandom random(seed);const auto plan=dredmor::plan_room_population(record,origin,random);
            std::cout<<unsigned(plan.request)<<' '<<plan.position.x<<' '<<plan.position.y<<' '<<std::quoted(plan.name)<<' '
                <<std::quoted(plan.type)<<' '<<std::quoted(plan.subtype)<<' '<<plan.amount<<' '<<plan.minimum<<' '<<plan.additional<<' '
                <<plan.artifact<<' '<<random.state()<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
