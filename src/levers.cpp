#include "dredmor/levers.hpp"
#include <cstring>

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
}
