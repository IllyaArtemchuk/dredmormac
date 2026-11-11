#include "dredmor/floors.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace dredmor {
namespace {
void validate_global_ids(const Session& session,const std::vector<SessionFloor>& cached) {
    std::set<std::uint64_t> actors{session.player().id},items;
    auto item=[&](std::uint64_t id) {if (!id || !items.insert(id).second) throw std::runtime_error("item identity appears on multiple floors");};
    for (const auto& slot:session.inventory().bag()) if (slot) item(slot->instance);
    for (const auto& slot:session.inventory().belt()) if (slot) item(slot->instance);
    for (const auto& slot:session.equipment().slots()) if (slot) item(slot->instance);
    auto floor=[&](const SessionFloor& value) {
        for (const auto& actor:value.enemies) if (!actor.id || !actors.insert(actor.id).second) throw std::runtime_error("actor identity appears on multiple floors");
        for (const auto& ground:value.ground) item(ground.item.instance);
        for (const auto& trap:value.traps) item(trap.instance);
    };
    floor(session.floor_state());for (const auto& value:cached) floor(value);
}
}
SessionFloor Session::floor_state() const {
    SessionFloor floor(world_);floor.depth=depth_context_;floor.branch=floor_branch_;floor.stairs=stairs_;
    floor.enemies=enemies_;floor.ground=ground_;floor.objects=custom_objects_;floor.breakables=breakables_;floor.traps=traps_;return floor;
}
void Session::floor_stairs(std::vector<StairPlacement> stairs,std::int32_t branch) {
    if (stairs.size()>4096) throw std::invalid_argument("too many floor stairs");
    std::set<std::pair<int,int>> positions;
    for (const auto& stair:stairs) if (!world_.passable(stair.position) || !positions.emplace(stair.position.x,stair.position.y).second)
        throw std::invalid_argument("invalid or duplicate stair position");
    stairs_=std::move(stairs);floor_branch_=branch;
}
SessionFloor Session::swap_floor(SessionFloor floor,Position arrival) {
    for (const auto& trigger:player_.triggers) if (trigger.caster && trigger.caster!=player_.id)
        throw std::invalid_argument("cross-floor enemy spell casters are not implemented");
    auto next=*this;auto previous=floor_state();const auto turn=world_.turn();
    next.item_floor_=next_item_instance();
    floor.world.place_player(arrival,turn);next.world_=std::move(floor.world);next.player_.position=arrival;
    next.depth_context_=floor.depth;next.floor_stairs(std::move(floor.stairs),floor.branch);
    next.enemies_=std::move(floor.enemies);next.ground_=std::move(floor.ground);next.custom_objects_=std::move(floor.objects);
    next.breakables_=std::move(floor.breakables);next.traps_=std::move(floor.traps);next.events_.clear();
    // Reuse the save validator for actor/item/trigger identities, positions,
    // buff profiles and all other floor payloads before replacing live state.
    std::ostringstream encoded;next.save(encoded);std::istringstream input(encoded.str());
    next=Session::load(input,next.definitions_,next.abilities_,next.spells_);
    *this=std::move(next);return previous;
}
void FloorRegistry::add_floor(Session& session,SessionFloor floor) {
    if (floor.world.turn()>session.world().turn()) throw std::invalid_argument("cached floor has a future visit clock");
    const auto visit_turn=floor.world.turn();
    if (cached_.size()>=63 || floor.depth.depth==session.depth_context_.depth ||
        std::any_of(cached_.begin(),cached_.end(),[&](const auto& other){return other.depth.depth==floor.depth.depth;}))
        throw std::invalid_argument("duplicate floor depth or floor cache budget exceeded");
    auto next=session;std::set<std::uint64_t> actors{session.player_.id},items;
    auto collect=[&](const SessionFloor& source) {
        for (const auto& actor:source.enemies) if (!actors.insert(actor.id).second) throw std::invalid_argument("actor identity appears on multiple floors");
        for (const auto& ground:source.ground) if (!items.insert(ground.item.instance).second) throw std::invalid_argument("item identity appears on multiple floors");
        for (const auto& trap:source.traps) if (!items.insert(trap.instance).second) throw std::invalid_argument("trap identity appears on multiple floors");
    };
    collect(session.floor_state());for (const auto& cached:cached_) collect(cached);
    for (const auto& slot:session.inventory_.bag()) if (slot) items.insert(slot->instance);
    for (const auto& slot:session.inventory_.belt()) if (slot) items.insert(slot->instance);
    for (const auto& slot:session.equipment_.slots()) if (slot) items.insert(slot->instance);
    for (auto& actor:floor.enemies) {
        if (!actor.id) actor.id=next.next_actor_++;
        if (actor.id==std::numeric_limits<std::uint64_t>::max() || !actors.insert(actor.id).second) throw std::invalid_argument("invalid cached actor identity");
        next.next_actor_=std::max(next.next_actor_,actor.id+1);
    }
    auto fresh=next.next_item_instance();
    auto reserve=[&](std::uint64_t& id) {
        if (!id) id=fresh;
        if (id==std::numeric_limits<std::uint64_t>::max() || !items.insert(id).second) throw std::invalid_argument("invalid cached item identity");
        fresh=std::max(fresh,id+1);
    };
    for (auto& ground:floor.ground) reserve(ground.item.instance);
    for (auto& trap:floor.traps) reserve(trap.instance);
    next.item_floor_=fresh;next.player_.triggers.clear();const auto placeholder=floor.world.player();
    next.swap_floor(std::move(floor),placeholder);next.world_.place_player(placeholder,visit_turn);auto validated=next.floor_state();
    auto cached=cached_;cached.push_back(std::move(validated));
    const auto item_base=std::max(fresh,next.next_item_instance());
    session.next_actor_=next.next_actor_;session.item_floor_=item_base;cached_=std::move(cached);
}
FloorTravel FloorRegistry::travel(Session& session,bool ascending) {
    if (!session.player_.alive()) return FloorTravel::dead;
    const auto p=session.player_.position;
    const auto stair=std::find_if(session.stairs_.begin(),session.stairs_.end(),[&](const auto& s){return s.up==ascending && s.position.x==p.x && s.position.y==p.y;});
    if (stair==session.stairs_.end()) return FloorTravel::no_stair;
    const auto destination=stair_destination(ascending,stair->link,session.floor_branch_,session.depth_context_.depth);
    const auto found=std::find_if(cached_.begin(),cached_.end(),[&](const auto& floor){return floor.depth.depth==destination.depth;});
    if (found==cached_.end()) return FloorTravel::missing_floor;
    validate_global_ids(session,cached_);
    const auto arrival=stair_arrival(found->stairs,ascending,destination.link);
    if (!arrival) return FloorTravel::missing_arrival;
    auto next=session;auto cached=cached_;const auto index=std::size_t(found-cached_.begin());
    auto previous=next.swap_floor(cached[index],*arrival);cached[index]=std::move(previous);
    session=std::move(next);cached_=std::move(cached);return FloorTravel::travelled;
}
FloorTravel FloorRegistry::step(Session& session,Action action) {
    auto next=session;auto registry=*this;const auto before=next.player().position;next.step(action);const auto after=next.player().position;
    auto outcome=FloorTravel::no_stair;
    if (before.x!=after.x || before.y!=after.y) for (const auto& stair:next.floor_stairs()) if (stair.position.x==after.x && stair.position.y==after.y) {outcome=registry.travel(next,stair.up);break;}
    session=std::move(next);*this=std::move(registry);return outcome;
}
void FloorRegistry::save(const Session& session,std::ostream& output) const {
    validate_global_ids(session,cached_);
    output<<"<dredmor-run version=\"1\">\n";session.save(output);output<<"<cached-floors>\n";
    for (const auto& floor:cached_) {
        auto copy=session;copy.player_.triggers.clear();copy.swap_floor(floor,floor.world.player());copy.world_.place_player(floor.world.player(),floor.world.turn());copy.save(output);
    }
    output<<"</cached-floors>\n</dredmor-run>\n";
    if (!output) throw std::runtime_error("cannot write floor registry save");
}
void FloorRegistry::for_each_floor(const Session& session,const std::function<void(const Session&)>& visitor) const {
    visitor(session);
    for (const auto& floor:cached_) {auto copy=session;copy.player_.triggers.clear();copy.swap_floor(floor,floor.world.player());visitor(copy);}
}
std::pair<Session,FloorRegistry> FloorRegistry::load(std::istream& input,
        std::shared_ptr<const std::vector<ItemDefinition>> definitions,
        std::shared_ptr<const std::vector<AbilityDefinition>> abilities,
        std::shared_ptr<const std::vector<SpellDefinition>> spells) {
    std::string text;std::array<char,4096> buffer;
    while (input) {input.read(buffer.data(),buffer.size());text.append(buffer.data(),std::size_t(input.gcount()));if (text.size()>64*1024*1024) throw std::runtime_error("run save exceeds size limit");}
    if (input.bad()) throw std::runtime_error("cannot read run save");
    const auto root=parse_xml(text,"run save");
    if (root.tag!="dredmor-run" || root.attribute("version")!="1" || root.children.size()!=2 || root.children[0].tag!="dredmor-session" || root.children[1].tag!="cached-floors") throw std::runtime_error("invalid run save structure");
    auto session=Session::load_node(root.children[0],definitions,abilities,spells);FloorRegistry registry;
    const auto item_base=session.item_floor_,actor_base=session.next_actor_;
    for (const auto& saved:root.children[1].children) {
        if (saved.tag!="dredmor-session") throw std::runtime_error("invalid cached floor record");
        auto cached=Session::load_node(saved,definitions,abilities,spells);auto floor=cached.floor_state();
        for (const auto& actor:floor.enemies) if (actor.id>=actor_base) throw std::runtime_error("cached actor exceeds the global identity sequence");
        for (const auto& ground:floor.ground) if (ground.item.instance>=item_base) throw std::runtime_error("cached item exceeds the global identity reservation");
        for (const auto& trap:floor.traps) if (trap.instance>=item_base) throw std::runtime_error("cached trap exceeds the global identity reservation");
        registry.add_floor(session,std::move(floor));
    }
    session.item_floor_=item_base;session.next_actor_=actor_base;
    return {std::move(session),std::move(registry)};
}
}
