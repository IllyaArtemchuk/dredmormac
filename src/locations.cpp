#include "dredmor/generation.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
int add(int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;}
bool floor_sides(const DungeonLayout& map,Position point) {
    for (int y=point.y-1;y<=point.y+1;++y) for (int x=point.x-1;x<=point.x+1;++x)
        if (x>=0 && y>=0 && x<map.width() && y<map.height() && map.at(x,y).kind!=1) return false;
    return true;
}
bool eligible(const DungeonLayout& map,Position point,const std::vector<bool>& occupied,bool near) {
    if (point.x<0 || point.y<0 || point.x>=map.width() || point.y>=map.height()) return false;
    const auto kind=map.at(point.x,point.y).kind;
    if (kind!=1 && (!near || (kind!=13 && kind!=14))) return false;
    return !occupied[std::size_t(point.y)*map.width()+point.x] && floor_sides(map,point);
}
}
LocationSearch find_good_location_bounded(const DungeonLayout& map,Position minimum,Position maximum,
    const std::vector<bool>& occupied,DarwinRandom& random) {
    if (occupied.size()!=map.cells().size()) throw std::invalid_argument("invalid spawn-site occupancy");
    LocationSearch result;
    for (int attempt=0;attempt<51;++attempt) {
        result.position.x=add(minimum.x,int(random.bounded(std::uint32_t(maximum.x)-std::uint32_t(minimum.x))));
        result.position.y=add(minimum.y,int(random.bounded(std::uint32_t(maximum.y)-std::uint32_t(minimum.y))));
        const auto p=result.position;
        if (p.x<minimum.x || p.x>maximum.x || p.y<minimum.y || p.y>maximum.y) continue;
        if (eligible(map,p,occupied,false)) {result.found=true;break;}
    }
    return result;
}
LocationSearch find_good_location_near(const DungeonLayout& map,Position center,
    const std::vector<bool>& occupied,DarwinRandom& random) {
    if (occupied.size()!=map.cells().size()) throw std::invalid_argument("invalid spawn-site occupancy");
    for (int dx=-3;dx<=3;++dx) for (int dy=-3;dy<=3;++dy) {
        const Position point{add(center.x,dx),add(center.y,dy)};
        if (eligible(map,point,occupied,true)) return {true,point};
    }
    return find_good_location_bounded(map,{add(center.x,-10),add(center.y,-10)},
        {add(center.x,10),add(center.y,10)},occupied,random);
}
}
