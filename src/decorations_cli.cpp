#include "dredmor/decorations.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--read") {
            std::string line;while (std::getline(std::cin,line)) {
                const auto table=read_room_decorations(parse_xml(line));std::cout<<table.size();
                for (const auto& d:table) std::cout<<' '<<std::quoted(d.subject)<<' '<<int(d.kind)<<' '<<std::quoted(d.sprite);std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--plan") {
            RoomStamp room;Position origin;std::string subject;std::size_t count;
            while (std::cin>>room.width>>room.height>>origin.x>>origin.y>>std::quoted(subject)>>count) {
                if (room.width<1 || room.width>128 || room.height<1 || room.height>128 || count>10000) throw std::invalid_argument("invalid decoration fixture");
                std::vector<RoomDecorationDefinition> table(count);
                for (auto& d:table) {int kind;std::cin>>std::quoted(d.subject)>>kind>>std::quoted(d.sprite);d.kind=RoomDecorationKind(kind);}
                room.cells.resize(std::size_t(room.width)*room.height);for (auto& c:room.cells) {unsigned marker;std::cin>>marker;if (!std::cin || marker>255) throw std::invalid_argument("invalid decoration marker");c.wall_marker=marker;}
                const auto plan=plan_room_decorations(room,origin,subject,table);std::cout<<plan.size();for (const auto& d:plan) std::cout<<' '<<int(d.kind)<<' '<<d.position.x<<' '<<d.position.y<<' '<<std::quoted(d.sprite);std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--construct") {
            int kind;std::string sprite;while (std::cin>>kind>>std::quoted(sprite)) {
                const auto object=make_room_decoration({RoomDecorationKind(kind),{0,0},sprite});
                std::cout<<object.blocks_movement<<' '<<object.blocks_vision<<' '<<object.frame<<' '<<object.playing<<' '<<std::quoted(object.sprite.image)<<'\n';
            }
        } else throw std::invalid_argument("expected --read, --plan or --construct");
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
