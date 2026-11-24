#include "dredmor/secret_doors.hpp"
#include <stdexcept>

namespace dredmor {
std::vector<SecretDoorPlacement> generate_secret_door_plan(DungeonLayout& layout,
    GameRandom& random,const std::vector<bool>& room_no_blockers,const std::vector<bool>& occupied,
    const std::function<std::optional<InventoryItem>(GameRandom&)>& treasure_factory) {
    if (layout.width()<6 || layout.height()<6 || occupied.size()!=layout.cells().size() ||
        room_no_blockers.empty() || !treasure_factory) throw std::invalid_argument("invalid secret-door inputs");
    auto candidate=layout;auto stream=random;auto objects=occupied;
    std::vector<SecretDoorPlacement> result;
    const int width=candidate.width(),height=candidate.height();
    auto wall_if_void=[&](int x,int y) {
        if (x>=0 && y>=0 && x<width && y<height && candidate.at(x,y).kind==0) candidate.at(x,y).kind=2;
    };
    auto surrounded=[&](int x,int y) {
        for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx) {
            const int nx=x+dx,ny=y+dy;
            if (nx>=0 && ny>=0 && nx<width && ny<height && candidate.at(nx,ny).kind!=1) return false;
        }
        return true;
    };
    for (int round=0;round<3;++round) {
        for (int rejected=0;rejected<50;++rejected) {
            const int x=int(stream.libc.bounded(std::uint32_t(width-6)))+3;
            const int y=int(stream.libc.bounded(std::uint32_t(height-6)))+3;
            if (candidate.at(x,y).kind!=2 || candidate.at(x,y-1).kind!=2 || candidate.at(x,y+1).kind!=2) continue;
            if (room_no_blockers.at(candidate.at(x,y).room_id)) continue;
            int pocket;
            if (candidate.at(x-1,y).kind==0 && candidate.at(x-2,y).kind==0 && candidate.at(x+1,y).kind==1) pocket=x-1;
            else if (candidate.at(x+1,y).kind==0 && candidate.at(x+2,y).kind==0 && candidate.at(x-1,y).kind==1) pocket=x+1;
            else {++rejected;continue;}
            SecretDoorPlacement door;door.wall={x,y};door.pocket={pocket,y};
            candidate.at(x,y).kind=2;candidate.at(pocket,y).kind=1;candidate.at(pocket,y+1).kind=1;
            for (int dx=-1;dx<=1;++dx) wall_if_void(pocket+dx,y-1);
            wall_if_void(pocket-1,y);wall_if_void(pocket+1,y);
            wall_if_void(pocket-1,y+1);
            wall_if_void(pocket-1,y+2);wall_if_void(pocket+1,y+2);wall_if_void(pocket,y+2);
            wall_if_void(pocket+1,y+1);
            door.treasure=treasure_factory(stream);
            if (door.treasure) objects[std::size_t(y)*width+pocket]=true;
            for (int dx=-5;dx<5 && !door.lever;++dx) for (int dy=-5;dy<5;++dy) {
                const int lx=pocket+dx,ly=y+dy;
                if (lx<=0 || ly<=0 || lx>=width || ly>=height || !surrounded(lx,ly) || objects[std::size_t(ly)*width+lx]) continue;
                door.lever=Position{lx,ly};objects[std::size_t(ly)*width+lx]=true;break;
            }
            result.push_back(std::move(door));break;
        }
    }
    layout=std::move(candidate);random=std::move(stream);return result;
}
}
