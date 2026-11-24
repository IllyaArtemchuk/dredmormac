#include "dredmor/spawn_locations.hpp"
#include <stdexcept>

namespace dredmor {
std::optional<Position> select_monster_spawn_position(const DungeonLayout& layout,const std::vector<bool>& occupied,
    const std::vector<Position>& special_sites,Position requested,std::int32_t special,DarwinRandom& random,std::size_t budget) {
    if (requested.x!=-1 && requested.y!=-1) return requested;
    if (!budget || (special!=1 && occupied.size()!=layout.cells().size()) || special_sites.size()>0x7ffffffeU)
        throw std::invalid_argument("invalid monster spawn position context");
    auto candidate=random;
    for (std::size_t attempt=0;attempt<budget;++attempt) {
        Position point{int(candidate.bounded(std::uint32_t(layout.width()))),int(candidate.bounded(std::uint32_t(layout.height())))};
        if (special==1) {
            if (special_sites.empty()) {random=candidate;return std::nullopt;}
            point=special_sites[candidate.bounded(std::uint32_t(special_sites.size()))];
            if (point.x<0 || point.y<0 || point.x>=layout.width() || point.y>=layout.height()) throw std::invalid_argument("special monster site outside level");
            if (layout.at(point.x,point.y).kind!=7) continue;
        } else {
            const auto kind=layout.at(point.x,point.y).kind;
            if ((kind!=1 && kind!=14) || occupied[std::size_t(point.y)*layout.width()+point.x]) continue;
        }
        random=candidate;return point;
    }
    throw std::runtime_error("monster spawn position search budget exhausted");
}
}
