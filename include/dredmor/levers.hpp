#pragma once
#include "dredmor/world.hpp"
#include "dredmor/clocks.hpp"
#include "dredmor/room_records.hpp"
#include <array>
#include <string>
#include <vector>

namespace dredmor {
struct ScriptLever {CustomRoomObject object;ActivationState activation;};
struct SecretLever {Position position{},target{};bool activated=false;};
struct SecretLeverActivation {
    bool revealed=false,tutorial_event=false;
    std::vector<std::string> messages;
    std::string sound;
    // Original GenLevelGFXRep rectangle, populated only when the wall opens.
    std::array<std::int32_t,4> graphics_bounds{};
};
// Lever::FlipLever type1. The tutorial path activates the lever before the
// type dispatch, so its first tutorial flip reports stuck without opening.
// Caller applies revealed target kind1 and queues graphics/audio/UI effects.
SecretLeverActivation flip_secret_lever(SecretLever& lever,bool tutorial=false);
// Original Clicked gate: exactly one cardinal tile away, no held mouse item.
bool can_click_secret_lever(const SecretLever& lever,Position player,bool held_item=false);
// Ordinary type0 lever. Tutorial activation occurs before the toggle.
SecretLeverActivation flip_script_lever(ActivationState& activation,bool tutorial=false);
std::optional<ScriptLever> create_script_lever(const RoomElementDefinition& definition,Position origin,DarwinRandom& random);

}
