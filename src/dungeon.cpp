#include "dredmor/dungeon.hpp"
#include <deque>
#include <stdexcept>

namespace dredmor {
namespace {
GeometryAttempt attempt(const std::vector<RoomStamp>& pool,const RoomNameWords& words,GameRandom& random,const GeometryOptions& options,
                        const std::vector<DungeonCell>* previous=nullptr) {
    if (options.width<1 || options.height<1 || options.width>512 || options.height>512 || options.depth<0 ||
        pool.empty() || pool.size()>0x7ffffffeU || !options.max_candidate_draws || !options.max_rooms)
        throw std::invalid_argument("invalid geometry dimensions/pool/budget");
    for (const auto& room:pool) if (room.width<1 || room.height<1 || room.width>512 || room.height>512 ||
        room.cells.size()!=std::size_t(room.width)*room.height) throw std::invalid_argument("invalid geometry template");
    for (auto index:options.starters) if (index>=pool.size()) throw std::invalid_argument("invalid branch starter index");
    std::vector<DungeonCell> cells(std::size_t(options.width)*options.height);
    // The original retry clears flag bits 0..6 individually, retaining the
    // special-wall bit even where the next attempt leaves an empty cell.
    if (previous) for (std::size_t i=0;i<cells.size();++i) cells[i].flags=(*previous)[i].flags&~0x7fU;
    GeometryAttempt result{DungeonLayout(options.width,options.height,std::move(cells))};
    std::deque<Position> doors;
    std::vector<bool> spawned(pool.size(),false);bool zoo=false,god=false;
    auto write=[&](std::size_t index,Position origin) {
        if (result.rooms.size()>=options.max_rooms) throw std::runtime_error("geometry room budget exceeded");
        const auto& room=pool[index];const auto previous=result.layout.new_doors().size();
        result.layout.write_template(room,origin.x,origin.y,static_cast<std::uint8_t>(result.rooms.size()),options.depth,random.mt);
        RoomName name;
        if (room.fixed_name.empty()) name=generate_room_name(words,random);else name.display=room.fixed_name;
        result.rooms.push_back({index,origin,std::move(name),{}});
        if (room.shop) ++result.shops;
        const auto& appended=result.layout.new_doors();
        for (std::size_t i=previous;i<appended.size();++i) doors.push_back(appended[i]);
    };
    auto choice=[&] {
        if (++result.candidate_draws>options.max_candidate_draws) throw std::runtime_error("geometry candidate budget exceeded");
        return std::size_t(random.mt.bounded(static_cast<std::uint32_t>(pool.size())));
    };
    std::size_t initial=0;
    if (options.depth!=0) {
        if (!options.starters.empty()) initial=options.starters[random.libc.bounded(static_cast<std::uint32_t>(options.starters.size()))];
        else for (;;) {
            initial=choice();const auto& rules=pool[initial].rules;
            std::size_t door_count=0;
            for (const auto& cell:pool[initial].cells) door_count+=cell.kind==6;
            if (rules.minimum_level<=options.depth && options.depth<=rules.maximum_level && door_count>=3) break;
        }
    }
    const auto& starter=pool[initial];
    const auto range_x=std::uint32_t(options.width-1-starter.width),range_y=std::uint32_t(options.height-1-starter.height);
    const Position origin{static_cast<int>(random.mt.bounded(range_x))+1,static_cast<int>(random.mt.bounded(range_y))+1};
    write(initial,origin);
    if (options.depth==0) result.initial_player=Position{origin.x+4,origin.y+4};
    while (!doors.empty()) {
        const auto door=doors.front();bool placed=false;unsigned eligible_attempts=0;
        while (eligible_attempts<5 && !placed) {
            const auto index=choice();const auto& room=pool[index];const auto& rules=room.rules;
            if (rules.quest || (room.zoo && zoo) || (rules.diggle_god && god)) continue;
            if (room.shop && static_cast<int>(result.shops)>=(options.short_game?1:options.max_shops)) continue;
            if ((rules.unique && spawned[index]) || rules.minimum_level>options.depth || rules.maximum_level<options.depth) continue;
            for (int y=0;y<room.height && !placed;++y) for (int x=0;x<room.width && !placed;++x) {
                if (room.cells[std::size_t(y)*room.width+x].kind!=6) continue;
                const Position next{door.x-x,door.y-y};
                if (!result.layout.accepts(room,next.x,next.y)) continue;
                const auto parent=std::size_t(result.layout.at(door.x,door.y).room_id),child=result.rooms.size();
                if (parent>=child) throw std::runtime_error("invalid generated room adjacency");
                write(index,next);result.rooms[parent].neighbors.push_back(child);result.rooms[child].neighbors.push_back(parent);
                spawned[index]=true;if (room.zoo) zoo=true;if (rules.diggle_god) god=true;placed=true;
            }
            ++eligible_attempts;
        }
        if (!placed) result.layout.at(door.x,door.y).kind=2;
        doors.pop_front();
    }
    result.accepted=result.rooms.size()-result.shops>(options.short_game?10U:15U) && result.layout.floor_count()>(options.short_game?799U:2999U);
    return result;
}
}
GeometryAttempt generate_geometry_attempt(const std::vector<RoomStamp>& pool,const RoomNameWords& words,GameRandom& random,const GeometryOptions& options) {
    auto candidate=random;auto result=attempt(pool,words,candidate,options);random=std::move(candidate);return result;
}
GeometryAttempt generate_level_geometry(const std::vector<RoomStamp>& pool,const RoomNameWords& words,GameRandom& random,const GeometryOptions& options) {
    if (!options.max_attempts) throw std::invalid_argument("geometry attempt budget must be positive");
    auto candidate=random;
    std::vector<DungeonCell> previous;
    for (std::size_t count=1;count<=options.max_attempts;++count) {
        auto result=attempt(pool,words,candidate,options,previous.empty()?nullptr:&previous);
        if (result.accepted) {result.attempts=count;random=std::move(candidate);return result;}
        previous=result.layout.cells();
    }
    throw std::runtime_error("geometry reroll budget exceeded");
}
DoorBuildPlan build_door_plan(DungeonLayout& layout) {
    auto candidate=layout;DoorBuildPlan result;
    const auto width=layout.width(),height=layout.height();
    auto neighbor=[&](int x,int y) {
        // Original arithmetic uses a linear cell offset, including crossing a
        // row boundary. Preserve that distinction from coordinate validation.
        const auto index=std::int64_t(y)*width+x;
        if (index<0 || index>=std::int64_t(width)*height) throw std::invalid_argument("door neighbor outside allocated grid");
        return candidate.cells()[std::size_t(index)].kind;
    };
    for (int x=0;x<width;++x) for (int y=height-2;y>0;--y) {
        auto& cell=candidate.at(x,y);
        if (cell.kind!=6) continue;
        const bool horizontal=(cell.door_flags&1)!=0;
        if (horizontal && neighbor(x-2,y)==1 && neighbor(x+2,y)==1) {
            cell.kind=1;result.repaired.push_back({x,y});continue;
        }
        DoorAssembly assembly{{x,y},horizontal,(cell.door_flags&2)!=0,(cell.door_flags&4)!=0,{}};
        for (int i=0;i<3;++i) {
            const Position part{horizontal?x+i-1:x,horizontal?y:y+i-1};
            if (part.x<0 || part.y<0 || part.x>=width || part.y>=height) throw std::invalid_argument("door part outside allocated grid");
            assembly.parts[std::size_t(i)]=part;
        }
        result.doors.push_back(assembly);
    }
    layout=std::move(candidate);return result;
}
}
