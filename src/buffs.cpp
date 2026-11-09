#include "dredmor/buffs.hpp"
#include "dredmor/legacy_number.hpp"
#include <cstring>
#include <cstdlib>
#include <stdexcept>

namespace dredmor {
namespace {
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+=32;return value;}
}
BuffDefinition read_buff(const XmlNode& node) {
    if (lower(node.tag)!="buff") throw std::invalid_argument("expected buff definition");
    BuffDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);const auto& value=attribute.second;const auto n=legacy_integer(value);
        if (key=="tag") result.tag=value;
        else if (key=="description") result.description=value;
        else if (key=="gameplaytext") result.gameplay_text=value;
        else if (key=="icon") result.icon=value;
        else if (key=="smallicon") result.small_icon=value;
        else if (key=="insufficientfunds") result.insufficient_funds=value;
        else if (key=="time") result.time=n;
        else if (key=="stacksize") result.stack_size=n;
        else if (key=="applytoplayer") result.apply_to_player=n;
        else if (key=="destroyonmove") result.destroy_on_move=n;
        else if (key=="brittle") result.brittle=n;
        else if (key=="attacks") result.attacks=n;
        else if (key=="manaupkeep") result.mana_upkeep=n;
        else if (key=="zorkmidupkeep") result.zorkmid_upkeep=n;
        else if (key=="requiresweapon") result.requires_weapon=n;
        else if (key=="requiresshield") result.requires_shield=n;
        else if (key=="zorkmidscalef") result.zorkmid_scale=static_cast<float>(std::strtod(value.c_str(),nullptr));
        else if (key=="usetimer") result.use_timer=n!=0;
        else if (key=="allowstacking") result.allow_stacking=n!=0;
        else if (key=="self") result.self=n!=0;
        else if (key=="bad") result.bad=n!=0;
        else if (key=="removable") result.removable=n!=0;
        else if (key=="digglegod") result.diggle_god=n!=0;
    }
    for (const auto& child:node.children) {
        const auto tag=lower(child.tag);
        auto number=[&](const std::string& key) {int value=0;for (const auto& a:child.attributes) if (lower(a.first)==key) value=legacy_integer(a.second);return value;};
        if (tag=="primarybuff" || tag=="highbuff" || tag=="secondarybuff") {
            const auto id=number("id"),amount=number("amount");const bool primary=tag!="secondarybuff";
            if (id<0 || id>=(primary?6:24)) throw std::invalid_argument("buff stat ID outside range");
            if (primary) result.modifiers.primary[id]=amount;else result.modifiers.secondary[id]=amount;
        } else if (tag=="sightbuff") result.modifiers.secondary[18]=number("amount");
        else if (tag=="invisible") result.invisible=true;
        else if (tag=="mute") result.mute=true;
        else if (tag=="damagebuff" || tag=="resistbuff") {
            auto& values=tag=="damagebuff"?result.modifiers.damage:result.modifiers.resistance;
            for (const auto& a:child.attributes) for (std::size_t i=0;i<damage_names.size();++i)
                if (a.first==damage_names[i]) values[i]=legacy_integer(a.second);
        }
    }
    return result;
}
BuffTick tick_buff(BuffClock clock) {
    bool deapply=false;
    if (!clock.permanent) {
        const auto bits=std::uint32_t(clock.remaining)-1;std::memcpy(&clock.remaining,&bits,4);
        deapply=clock.pending || (clock.remaining<0 && clock.use_timer);
    }
    const bool remove=clock.pending || (clock.remaining<0 && !clock.permanent && clock.use_timer);
    return {clock,deapply,remove};
}
}
