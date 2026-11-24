#include "dredmor/content.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace dredmor {
std::size_t Session::queued_horde_members() const {
    std::size_t count=0;for (const auto& group:pending_hordes_) count+=group.actors.size();return count;
}
std::optional<HordeResult> Session::queue_room_horde(const RoomHordeRecord& record,const DungeonLayout& layout,
    const std::vector<HordeRoomBounds>& rooms,
    const std::function<HordePopulation(const RoomPopulationPlan&,const HordeWorld&,const MonsterSpawnState&,GameRandom&,std::uint64_t)>& constructor,
    Position origin) {
    auto next=*this;const auto plan=plan_room_population(RoomPopulationRecord{record},origin,next.random_.libc);
    if (plan.request==PopulationRequest::none) {random_.libc=next.random_.libc;return std::nullopt;}
    if (!constructor || layout.width()!=world_.width() || layout.height()!=world_.height()) throw std::invalid_argument("horde construction needs matching floor geometry");
    std::vector<bool> occupied(std::size_t(layout.width())*layout.height(),false);
    auto mark=[&](Position point) {
        if (point.x>=0 && point.y>=0 && point.x<layout.width() && point.y<layout.height()) occupied[std::size_t(point.y)*layout.width()+point.x]=true;
    };
    mark(player_.position);
    for (const auto& actor:enemies_) mark(actor.position);
    for (const auto& item:ground_) mark(item.position);
    for (const auto& object:custom_objects_) mark(object.position);
    for (const auto& object:breakables_) mark(object.object.position);
    for (const auto& trap:traps_) mark(trap.position);
    for (const auto& lever:script_levers_) mark(lever.object.position);
    for (const auto& lever:secret_levers_) mark(lever.position);
    for (const auto& stair:stairs_) mark(stair.position);
    if (doors_) for (const auto& door:doors_->doors) for (auto point:door.assembly.parts) mark(point);
    for (const auto& shop:shops_) {mark(shop.keeper.position);for (const auto& pedestal:shop.pedestals) mark(pedestal.position);}
    const HordeWorld world{layout,layout,occupied,occupied,rooms};
    auto population=constructor(plan,world,next.monster_state_,next.random_,next.next_actor_);
    auto result=std::move(population.result);
    if (result.members.size()!=population.actors.size() || result.teleports.size()!=population.actors.size() ||
        result.accepted!=!population.actors.empty() || enemies_.size()+queued_horde_members()+population.actors.size()>512)
        throw std::invalid_argument("invalid horde population count");
    if (result.accepted) {
        const auto& leader=result.members.front();
        if (leader.leader || leader.followers.size()+1!=result.members.size() ||
            bool(leader.quest)!=result.quest_count.has_value() ||
            (result.quest_count && *result.quest_count!=std::int32_t(result.members.size())))
            throw std::invalid_argument("invalid new horde leader or quest count");
        for (std::size_t i=1;i<result.members.size();++i) if (leader.followers[i-1]!=result.members[i].identity ||
            result.members[i].leader!=leader.identity || result.members[i].quest!=leader.quest || !result.members[i].followers.empty())
            throw std::invalid_argument("invalid new horde follower links");
    }
    for (std::size_t i=0;i<population.actors.size();++i) {
        auto& actor=population.actors[i];const auto& member=result.members[i];const auto& teleport=result.teleports[i];
        if (next.next_actor_==std::numeric_limits<std::uint64_t>::max() || actor.id!=next.next_actor_ || member.identity!=actor.id ||
            teleport.object!=actor.id || teleport.level!=1 || !member.horde || !actor.stat_profile ||
            actor.stat_profile->primary_percent!=member.primary_percent || !actor.buffs.empty() || !actor.alive() ||
            member.moniker.size()>4096 || member.platoon.size()>4096 ||
            actor.health>actor.secondary[0] || actor.mana<0 || actor.mana>actor.secondary[1] ||
            teleport.position.x<0 || teleport.position.y<0 || teleport.position.x>=world_.width() || teleport.position.y>=world_.height())
            throw std::invalid_argument("invalid queued horde actor or placement");
        ++next.next_actor_;actor.horde=HordeMembership{member.moniker,member.platoon,member.leader,member.quest,member.followers};
    }
    next.depth_context_.depth=result.depth;
    if (!population.actors.empty()) next.pending_hordes_.push_back({std::move(population.actors),result.teleports});
    *this=std::move(next);return result;
}
std::vector<std::uint64_t> Session::place_queued_hordes() {
    auto next=*this;std::vector<std::uint64_t> placed;
    for (auto& group:next.pending_hordes_) for (std::size_t i=0;i<group.actors.size();++i) {
        auto& actor=group.actors[i];actor.position=group.teleports[i].position;placed.push_back(actor.id);
        // SafeTeleport does not require a vacant/passable destination. Two
        // queued members can occupy the same square, including the leader's.
        next.enemies_.push_back(std::move(actor));
    }
    next.pending_hordes_.clear();next.validate_hordes();*this=std::move(next);return placed;
}
void Session::validate_hordes() const {
    if (!pending_hordes_.empty()) throw std::runtime_error("finish queued horde placement before saving");
    if (player_.horde) throw std::runtime_error("player cannot own monster horde metadata");
    for (const auto& shop:shops_) if (shop.keeper.horde) throw std::runtime_error("shop keeper cannot own horde metadata");
    std::map<std::uint64_t,const Actor*> actors;
    for (const auto& actor:enemies_) if (!actors.emplace(actor.id,&actor).second) throw std::runtime_error("duplicate horde actor identity");
    for (const auto& actor:enemies_) if (actor.horde) {
        const auto& group=*actor.horde;
        if (!actor.stat_profile || group.moniker.size()>4096 || group.platoon.size()>4096 || group.followers.size()>511)
            throw std::runtime_error("invalid horde membership");
        if (group.leader) {
            const auto found=actors.find(group.leader);
            if (group.leader==actor.id || found==actors.end() || !found->second->horde || found->second->horde->leader ||
                found->second->horde->quest!=group.quest || !group.followers.empty() ||
                std::count(found->second->horde->followers.begin(),found->second->horde->followers.end(),actor.id)!=1)
                throw std::runtime_error("invalid horde leader reference");
        } else {
            std::set<std::uint64_t> followers;
            for (auto id:group.followers) {
                const auto found=actors.find(id);
                if (!id || !followers.insert(id).second || found==actors.end() || !found->second->horde || found->second->horde->leader!=actor.id)
                    throw std::runtime_error("invalid horde follower reference");
            }
        }
    }
}
}
