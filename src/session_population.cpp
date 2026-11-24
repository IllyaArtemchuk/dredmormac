#include "dredmor/session.hpp"
#include <stdexcept>

namespace dredmor {
DungeonLayout Session::room_spawn_layout(const DungeonLayout& initial) const {
    if (initial.width()!=world_.width() || initial.height()!=world_.height()) throw std::invalid_argument("spawn geometry does not match Session");
    auto current=doors_?doors_->layout:initial;
    for (const auto& shop:shops_) for (auto point:shop.tiles) current.at(point.x,point.y).kind=13;
    for (const auto& lever:secret_levers_) if (lever.activated) current.at(lever.target.x,lever.target.y).kind=1;
    return current;
}
std::vector<bool> Session::occupied_tiles(const DungeonLayout& layout) const {
    if (layout.width()!=world_.width() || layout.height()!=world_.height()) throw std::invalid_argument("occupancy geometry does not match Session");
    std::vector<bool> occupied(layout.cells().size(),false);
    auto mark=[&](Position point) {
        if (point.x>=0 && point.y>=0 && point.x<layout.width() && point.y<layout.height()) occupied[std::size_t(point.y)*layout.width()+point.x]=true;
    };
    mark(player_.position);
    for (const auto& actor:enemies_) mark(actor.position);
    for (const auto& item:ground_) mark(item.position);
    for (const auto& object:custom_objects_) mark(object.position);
    for (const auto& object:breakables_) mark(object.object.position);
    for (const auto& trap:traps_) {mark(trap.position);if (trap.has_origin_object) mark(trap.origin);}
    for (const auto& lever:script_levers_) mark(lever.object.position);
    for (const auto& lever:secret_levers_) mark(lever.position);
    for (const auto& stair:stairs_) mark(stair.position);
    if (doors_) for (const auto& door:doors_->doors) for (auto point:door.assembly.parts) mark(point);
    for (const auto& shop:shops_) {mark(shop.keeper.position);for (const auto& pedestal:shop.pedestals) mark(pedestal.position);}
    return occupied;
}
std::optional<std::uint64_t> Session::create_respawn_monster(const DungeonLayout& initial,const std::vector<Position>& special_sites,
    const std::function<std::optional<Actor>(const MonsterSpawnWorld&,GameRandom&,MonsterSpawnState&)>& constructor) {
    if (!constructor) throw std::invalid_argument("respawn requires monster constructor");
    auto next=*this;const auto layout=next.room_spawn_layout(initial);const auto occupied=next.occupied_tiles(layout);
    const MonsterSpawnWorld world{layout,occupied,special_sites};auto actor=constructor(world,next.random_,next.monster_state_);
    std::optional<std::uint64_t> result;
    if (actor) result=next.spawn_enemy(std::move(*actor));
    *this=std::move(next);return result;
}
}
