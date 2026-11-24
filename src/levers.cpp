#include "dredmor/levers.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
bool can_click_secret_lever(const SecretLever& lever,Position player,bool held_item) {
    const auto dx=std::int64_t(lever.position.x)-player.x,dy=std::int64_t(lever.position.y)-player.y;
    return !held_item && ((dx==0 && (dy==1 || dy==-1)) || (dy==0 && (dx==1 || dx==-1)));
}
SecretLeverActivation flip_secret_lever(SecretLever& lever,bool tutorial) {
    SecretLeverActivation result;result.messages.push_back("You pull the lever.");
    if (!lever.activated && tutorial) {result.tutorial_event=true;lever.activated=true;}
    if (lever.activated) {result.sound="lever";result.messages.push_back("The lever is stuck and refuses to move.");return result;}
    lever.activated=true;result.revealed=true;result.sound="lever3";result.messages.push_back("You hear the sounds of machinery.");
    auto offset=[](int value,int delta) {const auto bits=std::uint32_t(value)+std::uint32_t(delta);std::int32_t result;std::memcpy(&result,&bits,4);return result;};
    result.graphics_bounds={offset(lever.target.x,-3),offset(lever.target.y,-3),offset(lever.target.x,3),offset(lever.target.y,3)};
    return result;
}
SecretLeverActivation flip_script_lever(ActivationState& activation,bool tutorial) {
    SecretLeverActivation result;result.messages.push_back("You pull the lever.");
    if (!activation.activated() && tutorial) {result.tutorial_event=true;activation.activate(true);}
    result.sound="lever";activation.activate(!activation.activated());return result;
}
std::optional<ScriptLever> create_script_lever(const RoomElementDefinition& definition,Position origin,DarwinRandom& random) {
    if (definition.type!="lever") throw std::invalid_argument("script lever requires lever element");
    auto candidate=random;const auto plan=plan_room_element(definition,origin,candidate);
    if (!plan.accepted) {random=candidate;return std::nullopt;}
    ScriptLever result;result.object.kind=CustomRoomKind::blocker;result.object.position=plan.position;
    result.object.name="Lever";result.object.sprite.image="dungeon/lever1.spr";result.object.room_local_id=definition.placement.id;
    result.object.can_push=definition.placement.can_push;result.object.blocks_movement=true;
    result.object.blocks_vision=false;result.object.playing=false;result.object.looping=false;
    random=candidate;return result;
}

}
