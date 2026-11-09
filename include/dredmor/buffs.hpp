#pragma once
#include "dredmor/items.hpp"

namespace dredmor {
struct BuffDefinition {
    std::string tag,description,gameplay_text,icon,small_icon,insufficient_funds;
    ItemModifiers modifiers;
    std::int32_t time=0,stack_size=0,apply_to_player=1,destroy_on_move=0,brittle=0,attacks=0,mana_upkeep=0;
    std::int32_t zorkmid_upkeep=0,requires_weapon=-1,requires_shield=0;
    float zorkmid_scale=1;
    bool use_timer=true,allow_stacking=true,self=true,bad=false,removable=false,diggle_god=false;
    bool invisible=false,mute=false;
    XmlNode source;
};
// Typed fields from the original Buff constructor/XML callback. Unmodeled
// children remain in source; parsing does not imply execution support.
BuffDefinition read_buff(const XmlNode& node);
std::uint64_t buff_signature(const BuffDefinition& buff);
// Original name sorting, including the order of equal names. A nonnegative
// depth is exposed for controlled comparison of the heap fallback.
std::vector<std::size_t> buff_order(const std::vector<std::string>& names,int depth=-1);
struct BuffClock {std::int32_t remaining=0;bool permanent=false,use_timer=true,pending=false;};
struct BuffTick {BuffClock clock;bool deapply=false,remove=false;};
// One Hittable::UpdateBuffs counter/eligibility decision, before virtual
// deapplication/destruction. The counter expires below zero, not at zero.
BuffTick tick_buff(BuffClock clock);
}
