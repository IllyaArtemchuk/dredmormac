#include "dredmor/room_scopes.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
std::vector<RoomObjectIdentity> objects(std::size_t count) {
    if (count>4096) throw std::invalid_argument("too many registered room objects");std::vector<RoomObjectIdentity> result(count);
    for (auto& object:result) std::cin>>object.identity>>object.level>>object.assigned_room>>object.global>>std::quoted(object.id);return result;
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--assign") {
            int width,height;bool debug;RoomObjectIdentity object;std::string id;std::size_t count;
            while (std::cin>>width>>height>>debug>>object.identity>>object.level>>object.global>>object.position.x>>object.position.y>>object.assigned_room>>std::quoted(object.id)>>std::quoted(id)>>count) {
                if (width<1 || height<1 || width>512 || height>512) throw std::invalid_argument("invalid room grid");
                std::vector<DungeonCell> cells(std::size_t(width)*height);for (auto& cell:cells) {unsigned room;std::cin>>room;if (room>255) throw std::invalid_argument("invalid room byte");cell.room_id=room;}
                auto registered=objects(count);if (!std::cin) throw std::runtime_error("truncated ID assignment fixture");
                DungeonLayout layout(width,height,std::move(cells));const auto result=assign_room_object_id(object,id,object.level?&layout:nullptr,registered,debug);
                std::cout<<result.object.assigned_room<<' '<<std::quoted(result.object.id)<<' '<<result.lookup_performed<<' '<<result.lookup.tested<<' '<<bool(result.lookup.object)<<' '<<result.lookup.object.value_or(0)<<' '<<bool(result.duplicate)<<' '<<result.duplicate.value_or(0)<<' '<<result.debug_requested<<'\n';
            }
            return 0;
        }
        std::uint64_t level;std::int32_t room;std::string id;std::size_t count;
        while (std::cin>>level>>room>>std::quoted(id)>>count) {
            auto registered=objects(count);if (!std::cin) throw std::runtime_error("truncated room lookup fixture");
            const auto result=find_room_object(registered,level,room,id);std::cout<<bool(result.object)<<' '<<result.object.value_or(0)<<' '<<result.tested<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
