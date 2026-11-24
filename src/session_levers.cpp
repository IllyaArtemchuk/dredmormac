#include "dredmor/session.hpp"
#include <limits>
#include <stdexcept>

namespace dredmor {
std::optional<std::size_t> Session::create_script_lever(const RoomElementDefinition& definition,Position origin) {
    auto candidate=random_.libc;auto lever=dredmor::create_script_lever(definition,origin,candidate);
    if (!lever) {random_.libc=candidate;return std::nullopt;}
    if (script_levers_.size()>=10000) throw std::runtime_error("session lever limit exceeded");
    const auto point=lever->object.position;
    if (point.x<0 || point.y<0 || point.x>=world_.width() || point.y>=world_.height()) throw std::invalid_argument("lever outside session world");
    script_levers_.push_back(std::move(*lever));random_.libc=candidate;return script_levers_.size()-1;
}
void Session::use_script_lever_impl(std::size_t index) {
    auto& lever=script_levers_.at(index);flip_script_lever(lever.activation);lever.object.frame=lever.activation.activated()?1:0;
    events_.push_back({EventKind::lever_used,player_.id,index+1,lever.activation.activated()?1:0});
}
bool Session::use_script_lever(std::size_t index,bool held_item) {
    const auto& lever=script_levers_.at(index);
    if (!player_.alive() || !can_click_secret_lever({lever.object.position,{0,0},false},player_.position,held_item)) return false;
    if (world_.turn()==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("turn counter exhausted");
    auto next=*this;next.events_.clear();next.use_script_lever_impl(index);next.world_.step(Action::wait);next.enemy_turns();*this=std::move(next);return true;
}
}
