#include "dredmor/session.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace dredmor {
namespace {bool same(Position a,Position b) {return a.x==b.x && a.y==b.y;}}
void Session::create_room_doors(const RoomStamp& room,bool burglary,bool female,std::int32_t lockpick_xp) {
    if (room.width!=world_.width() || room.height!=world_.height()) throw std::invalid_argument("door room dimensions differ");
    auto next=*this;DungeonLayout layout(room.width,room.height,std::vector<DungeonCell>(std::size_t(room.width)*room.height));
    layout.write_template(room,0,0,0,depth_context_.depth,next.random_.mt);const auto plan=build_door_plan(layout);
    SessionDoors doors(std::move(layout));doors.burglary=burglary;doors.female=female;doors.lockpick_xp=lockpick_xp;
    for (const auto& assembly:plan.doors) {DoorState state;state.assembly=assembly;doors.doors.push_back(state);}
    for (auto point:plan.repaired) next.world_.carve_floor(point);
    next.floor_doors(std::move(doors));*this=std::move(next);
}
void Session::floor_doors(SessionDoors doors) {
    if (doors.layout.width()!=world_.width() || doors.layout.height()!=world_.height() || doors.doors.size()>4096 || doors.attack_sound.size()>4096)
        throw std::invalid_argument("invalid session door grid");
    if (doors.frames.empty()) for (const auto& door:doors.doors) doors.frames.push_back(door.assembly.open?1:0);
    if (doors.frames.size()!=doors.doors.size()) throw std::invalid_argument("invalid session door frames");
    std::set<std::pair<int,int>> positions;auto map=world_;
    for (std::size_t index=0;index<doors.doors.size();++index) {
        const auto& state=doors.doors[index];const auto& door=state.assembly;
        if (doors.frames[index]>(door.horizontal?1:2)) throw std::invalid_argument("invalid door sprite frame");
        if (!state.destroyed && ((door.open && !doors.frames[index]) || (!door.open && doors.frames[index]))) throw std::invalid_argument("door frame disagrees with open state");
        for (int i=0;i<3;++i) {
            const auto p=door.parts[i];const auto x=std::int64_t(door.center.x)+(door.horizontal?i-1:0),y=std::int64_t(door.center.y)+(door.horizontal?0:i-1);
            if (p.x!=x || p.y!=y || !positions.emplace(p.x,p.y).second) throw std::invalid_argument("invalid or overlapping door parts");
            const auto kind=doors.layout.at(p.x,p.y).kind;
            if (state.destroyed && kind!=1) throw std::invalid_argument("destroyed door must have floor tiles");
            if (!state.destroyed && !door.open && same(p,player_.position)) throw std::invalid_argument("closed door overlaps player");
            for (const auto& lever:secret_levers_) if (same(lever.position,p)) throw std::invalid_argument("door overlaps secret lever");
            for (const auto& stair:stairs_) if (same(stair.position,p)) throw std::invalid_argument("door overlaps stairs");
            if (i==1 || state.destroyed || kind==1) map.carve_floor(p);
        }
    }
    world_=std::move(map);doors_=std::move(doors);last_door_click_.reset();
}
DoorClickResult Session::use_door_impl(std::size_t index,std::size_t selected) {
    if (!doors_ || index>=doors_->doors.size() || selected>=3) throw std::out_of_range("invalid session door target");
    auto& state=doors_->doors[index];if (state.destroyed || !player_.alive()) return {};
    DoorClickContext input;input.player=player_.position;input.burglary=doors_->burglary;input.female=doors_->female;
    input.lockpick_xp=doors_->lockpick_xp;input.depth=depth_context_.depth;input.stealth=player_.secondary[12];input.attack_sound=doors_->attack_sound;
    input.free_pick=abilities_ && has_free_door_pick(skills_,*abilities_);
    auto pick=[&](const std::optional<InventoryItem>& item) {
        return item && item->kind==ItemKind::misc && definitions_ && item->definition<definitions_->size() && (*definitions_)[item->definition].builtin_subtype==0?item->quantity:0;
    };
    for (std::size_t i=0;i<9;++i) input.belt_picks[i]=pick(inventory_.belt()[i]);
    for (std::size_t i=0;i<48;++i) input.bag_picks[i]=pick(inventory_.bag()[i]);
    auto listen=[&](const Actor& actor) {input.listeners.push_back({actor.position,actor.monster_flags,doors_->layout.at(actor.position.x,actor.position.y).kind});};
    for (const auto& actor:enemies_) listen(actor);
    for (const auto& shop:shops_) listen(shop.keeper);
    const auto opened=state.assembly.open;
    auto result=click_door(doors_->layout,state,selected,input,random_,[&](Position p) {
        if (same(p,player_.position) || enemy_at(p)) return true;
        for (const auto& shop:shops_) if (shop.keeper.alive() && same(shop.keeper.position,p)) return true;
        // Original query receives object/player occupancy. Existing non-door
        // blocking objects also count; the raw World wall is not an occupant.
        for (const auto& object:custom_objects_) if (object.blocks_movement && same(object.position,p)) return true;
        for (const auto& object:breakables_) if (object.object.blocks_movement && same(object.object.position,p)) return true;
        return false;
    });
    if (result.consumed_pick) {
        const auto slot=*result.consumed_pick;
        const auto& item=slot<9?inventory_.belt()[slot]:inventory_.bag()[slot-9];
        if (!item || !inventory_.consume_one(item->instance)) throw std::logic_error("door lockpick disappeared");
    }
    std::size_t listener=0;for (auto& actor:enemies_) actor.monster_flags=input.listeners[listener++].flags;
    for (auto& shop:shops_) shop.keeper.monster_flags=input.listeners[listener++].flags;
    if (result.animation_facing) player_facing_=*result.animation_facing;
    if (opened!=state.assembly.open) doors_->frames[index]=state.assembly.open?(state.assembly.horizontal?1:2):0;
    if (state.destroyed) for (auto p:state.assembly.parts) world_.carve_floor(p);
    if (result.experience && progression_.gain(*result.experience,experience_n_)) {
        player_.health=player_.secondary[0];player_.mana=player_.secondary[1];events_.push_back({EventKind::level_up,player_.id,0,progression_.level});
    }
    if (result.damage) {
        player_.health=std::max(0,player_.health-result.damage->amount);
        if (!player_.health) {player_.corpse=true;events_.push_back({EventKind::death,player_.id});}
    }
    if (result.handled) events_.push_back({EventKind::door_used,player_.id,index+1,state.destroyed?3:state.assembly.open?1:0});
    last_door_click_=result;return result;
}
DoorClickResult Session::use_door(std::size_t index,std::size_t selected) {
    if (world_.turn()==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("turn counter exhausted");
    auto next=*this;next.events_.clear();next.last_door_click_.reset();auto result=next.use_door_impl(index,selected);
    if (result.handled) {next.world_.step(Action::wait);next.enemy_turns();}
    *this=std::move(next);return result;
}
}
