#include "dredmor/room_records.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::string prefix;
        while (std::cin>>std::quoted(prefix)) {
            std::size_t count;if (!(std::cin>>count) || count>100) throw std::invalid_argument("invalid point attributes");
            dredmor::RoomAttributes attributes;
            for (std::size_t i=0;i<count;++i) {std::string key,value;std::cin>>std::quoted(key)>>std::quoted(value);attributes.emplace(key,value);}
            dredmor::RoomStamp room{7,7,std::vector<dredmor::TemplateCell>(49)};
            const auto point=dredmor::take_room_point(attributes,prefix,{{'3',{2,4}}},room);
            std::cout<<bool(point)<<' '<<(point?point->x:71)<<' '<<(point?point->y:72)<<' '<<attributes.size();
            for (auto cell:room.cells) std::cout<<' '<<unsigned(cell.kind);
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
