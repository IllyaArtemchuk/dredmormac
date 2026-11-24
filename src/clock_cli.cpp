#include "dredmor/clocks.hpp"
#include "dredmor/room_records.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--metadata") {
            std::string xml;
            while (std::cin>>std::quoted(xml)) {
                RoomStamp room;room.width=room.height=7;room.cells.resize(49);
                const auto definition=read_room_clock(parse_xml(xml),{{'3',{2,4}}},room);const auto& p=definition.placement;const auto& sprite=definition.sprite;
                std::cout<<p.relative.x<<' '<<p.relative.y<<' '<<p.percent<<' '<<unsigned(p.can_push)<<' '<<std::quoted(p.id)<<' '
                    <<std::quoted(definition.name)<<' '<<std::quoted(definition.description)<<' '
                    <<definition.passable.has_value()<<' '<<definition.passable.value_or(false)<<' '
                    <<definition.vis_block.has_value()<<' '<<definition.vis_block.value_or(false)<<' '
                    <<std::quoted(sprite.image)<<' '<<std::quoted(sprite.series)<<' '<<sprite.first<<' '<<sprite.count<<' '<<sprite.rate<<' '
                    <<definition.initial<<' '<<definition.remaining.has_value()<<' '<<definition.remaining.value_or(-1)<<' '<<definition.started;
                for (const auto& cell:room.cells) std::cout<<' '<<unsigned(cell.kind);
                std::cout<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--activation") {
            ActivationState state;int action;bool value;
            while (std::cin>>state.current>>state.previous>>state.older>>action>>value) {
                if (action==1) state.activate(value);
                else if (action==2) state.advance_history();
                else if (action!=0) throw std::invalid_argument("invalid activation action");
                std::cout<<state.current<<' '<<state.previous<<' '<<state.older<<' '<<state.activated()<<' '<<state.changed()<<'\n';
            }
            return 0;
        }
        ClockState state;int action;std::int32_t amount;bool same_room;
        while (std::cin>>state.duration>>state.remaining>>state.step>>action>>amount>>same_room) {
            const auto result=advance_clock(state,ClockAction(action),amount,same_room);
            std::cout<<result.state.duration<<' '<<result.state.remaining<<' '<<result.state.step<<' '<<result.state.stopped()<<' '<<!result.state.stopped()<<' '<<result.room_queried<<' '<<result.events.size();
            for (const auto& event:result.events) std::cout<<' '<<std::quoted(event.name)<<' '<<event.state.duration<<' '<<event.state.remaining<<' '<<event.state.step;
            std::cout<<' '<<bool(result.message)<<' '<<std::quoted(result.message.value_or(""))<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
