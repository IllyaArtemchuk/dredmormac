#include "dredmor/hordes.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t add(std::int32_t value,std::int32_t amount) {
    const auto bits=std::uint32_t(value)+std::uint32_t(amount);std::int32_t result;
    std::memcpy(&result,&bits,4);return result;
}
bool inside(const DungeonLayout& level,Position p) {return p.x>=0 && p.y>=0 && p.x<level.width() && p.y<level.height();}
}
HordeResult spawn_horde(const MonsterTables& tables,const std::vector<MonsterDefinition>& definitions,HordeOptions options,
    const HordeWorld& world,GameRandom& random,const HordeHooks& hooks) {
    HordeResult result;result.depth=options.depth;result.leader_position=options.position;
    if (!inside(world.argument_level,options.position) && (options.position.x!=-1 || options.position.y!=-1)) return result;
    if (options.level_set>=4 || !options.level || !options.budget || (options.quest && !options.quest->identity) || world.argument_occupied.size()!=world.argument_level.cells().size() || world.current_occupied.size()!=world.current_level.cells().size())
        throw std::invalid_argument("invalid horde world/context");
    if (!hooks.tweak || !hooks.can_spawn || !hooks.construct || !hooks.monster_name || !hooks.tint || !hooks.rebuild || (!options.quest && !hooks.platoon_name) || (options.named && !hooks.find_named))
        throw std::invalid_argument("horde requires constructor/name/attribute hooks");
    result.additional=options.additional==-1?hooks.tweak("additional horde size"):options.additional;
    result.minimum=options.minimum==-1?hooks.tweak("minimum horde size"):options.minimum;
    const auto chance=hooks.tweak("horde is overpowered change");
    if (random.mt.bounded(std::uint32_t(chance))!=0) result.depth=add(result.depth,-1);
    result.table_level=std::max(0,std::min(options.depth,14));if (!options.expansion && result.table_level>=10) result.table_level=9;
    const auto& pool=tables.levels[result.table_level][options.level_set];if (pool.empty() || pool.size()>0x7ffffffeU) throw std::invalid_argument("horde monster pool is empty/invalid");
    bool selected=false;
    for (std::size_t attempt=0;attempt<options.budget;++attempt) {
        const auto candidate=pool[random.libc.bounded(std::uint32_t(pool.size()))];const auto& definition=definitions.at(candidate);
        if (definition.special==1) continue;
        if (!hooks.can_spawn(candidate,result.depth,true)) continue;
        result.selected=candidate;selected=true;break;
    }
    if (!selected) throw std::runtime_error("horde monster selection budget exhausted");
    const auto additional=std::int32_t(random.libc.bounded(std::uint32_t(result.additional)));
    if (options.position.x==-1 && options.position.y==-1) {
        bool located=false;
        for (std::size_t attempt=0;attempt<options.budget;++attempt) {
            const Position point{int(random.libc.bounded(std::uint32_t(world.argument_level.width()))),int(random.libc.bounded(std::uint32_t(world.argument_level.height())))};
            if (world.argument_level.at(point.x,point.y).kind==1) {result.leader_position=point;located=true;break;}
        }
        if (!located) throw std::runtime_error("horde origin search budget exhausted");
    }
    auto construct=[&]() {
        const auto index=options.named?hooks.find_named(*options.named):result.selected;
        if (index>=definitions.size()) throw std::invalid_argument("named horde definition is missing");
        auto member=hooks.construct(index,result.depth,random);member.definition=index;
        if (!member.identity || std::any_of(result.members.begin(),result.members.end(),[&](const auto& other) {return other.identity==member.identity;}))
            throw std::runtime_error("horde constructor returned invalid identity");
        return member;
    };
    result.members.push_back(construct());auto& leader=result.members[0];
    leader.moniker=hooks.monster_name(random);
    for (std::size_t i=0;i<leader.primary_percent.size();++i) leader.primary_percent[i]=add(leader.primary_percent[i],i?10:20);
    leader.platoon=options.quest?options.quest->platoon:hooks.platoon_name(random);leader.leader=0;
    hooks.tint(leader,-1,random);hooks.rebuild(leader,random);
    auto queue=[&](HordeMember& member,Position point) {
        RoomTeleport request{member.identity,options.level,point};result.teleports.push_back(request);if (hooks.queue) hooks.queue(request,member);
    };
    queue(leader,result.leader_position);leader.horde=true;leader.quest=options.quest?options.quest->identity:0;
    const auto attempts=add(add(result.minimum,additional),1);
    if (attempts>0) {
        if (std::uint32_t(attempts)>options.budget) throw std::runtime_error("horde member search budget exceeded");
        if (!inside(world.current_level,result.leader_position)) throw std::invalid_argument("horde origin outside current level");
        const auto room=world.current_level.at(result.leader_position.x,result.leader_position.y).room_id;
        if (room>=world.current_rooms.size()) throw std::invalid_argument("horde origin has no current room");
        const auto bounds=world.current_rooms[room];
        for (std::int32_t attempt=0;attempt<attempts;++attempt) {
            HordeSearch search;
            // Original consumes Y then X seed draws even though BoundedBy does
            // not use the seed when it chooses its own random trial points.
            search.seed.y=add(bounds.minimum.y,std::int32_t(random.libc.bounded(std::uint32_t(bounds.maximum.y)-std::uint32_t(bounds.minimum.y))));
            search.seed.x=add(bounds.minimum.x,std::int32_t(random.libc.bounded(std::uint32_t(bounds.maximum.x)-std::uint32_t(bounds.minimum.x))));
            const auto location=find_good_location_bounded(world.current_level,bounds.minimum,bounds.maximum,world.current_occupied,random.libc);
            search.position=location.position;search.found=location.found;
            const auto point=search.position;
            if (inside(world.current_level,point) && inside(world.argument_level,point) && world.argument_level.at(point.x,point.y).kind==1 && !world.argument_occupied[std::size_t(point.y)*world.argument_level.width()+point.x]) {
                // SpawnHorde ignores BoundedBy's return flag and checks only
                // the final coordinates against its argument level's floor/list.
                auto member=construct();queue(member,point);member.horde=true;member.leader=result.members[0].identity;member.quest=options.quest?options.quest->identity:0;
                result.members[0].followers.push_back(member.identity);result.members.push_back(std::move(member));search.accepted=true;
            }
            result.searches.push_back(search);
        }
    }
    result.accepted=true;if (options.quest) result.quest_count=std::int32_t(result.members.size());return result;
}
}
