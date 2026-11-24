#include "dredmor/doors.hpp"
#include "dredmor/legacy_number.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dredmor {
bool has_free_door_pick(const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities) {
    auto lower=[](std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c=char(c-'A'+'a');return value;};
    for (const auto& ability:abilities) {
        std::int32_t rank=0;
        for (std::size_t i=0;i<std::min(std::size_t(7),skills.size());++i) if (skills[i].id==ability.skill_id) {rank=skills[i].rank;break;}
        if (ability.level>=rank) continue;
        bool opens=false;
        for (const auto& child:ability.source.children) if (lower(child.tag)=="flags") {
            auto assign=[&](const std::string& key) {const auto found=child.attributes.find(key);if (found!=child.attributes.end() && lower(key)=="opendoors") opens=legacy_integer(found->second)!=0;};
            for (const auto& key:child.attribute_order) assign(key);
            for (const auto& entry:child.attributes) if (std::find(child.attribute_order.begin(),child.attribute_order.end(),entry.first)==child.attribute_order.end()) assign(entry.first);
        }
        if (opens) return true;
    }
    return false;
}
namespace {
std::int32_t signed_bits(std::uint32_t bits) {std::int32_t value;std::memcpy(&value,&bits,4);return value;}
std::uint32_t absolute(std::uint32_t value) {return signed_bits(value)<0?0U-value:value;}
DoorClickResult apply(DungeonLayout& layout,DoorState& state,std::size_t selected,
                     DoorClickContext& context,GameRandom& random,const std::function<bool(Position)>& obstructed) {
    auto& door=state.assembly;const auto position=door.parts[selected];DoorClickResult result;
    const auto dx=std::uint32_t(context.player.x)-std::uint32_t(position.x);
    const auto dy=std::uint32_t(context.player.y)-std::uint32_t(position.y);
    if (signed_bits(absolute(dx)+absolute(dy))>=2) {
        if (door.locked) result.sounds.push_back("locked");
        return result;
    }
    auto toggle=[&] {
        const auto transition=toggle_door(layout,door,selected,obstructed);result.handled=transition.handled;
        if (transition.outcome==DoorOutcome::opened) result.sounds.push_back("door_open");
        if (transition.outcome==DoorOutcome::closed) result.sounds.push_back("door_close");
        if (transition.outcome==DoorOutcome::obstructed) result.messages.push_back("The doorway is blocked!");
    };
    if (door.open || !door.locked) {toggle();return result;}
    bool unlock=context.free_pick;
    if (!unlock) {
        for (std::size_t i=0;i<57;++i) {
            auto& quantity=i<9?context.belt_picks[i]:context.bag_picks[i-9];
            if (quantity==0) continue;
            result.consumed_pick=i;
            if (quantity>=2) --quantity;
            else {quantity=0;result.destroyed_pick=i;}
            unlock=(i<9 && context.burglary) || random.libc.next()<0x73333362;
            if (!unlock) {
                result.handled=true;result.messages.push_back("The lockpick breaks when you attempt to use it!");return result;
            }
            break;
        }
    }
    if (unlock) {
        door.locked=false;result.messages.push_back("The door unlocks!");result.eye_candy=true;
        result.experience=signed_bits((std::uint32_t(context.depth)+1U)*std::uint32_t(context.lockpick_xp));
        // The original clears the flag on immediate neighbors only, leaving
        // the far linked tile's lock flag stale when an end is selected.
        for (std::size_t i=0;i<3;++i) if ((i>selected?i-selected:selected-i)<=1) {
            const auto p=door.parts[i];layout.at(p.x,p.y).door_flags&=std::uint8_t(~2U);
        }
        toggle();return result;
    }
    result.handled=true;
    if (dx==1 && dy==0) result.animation_facing=1;
    else if (dx==0xffffffffU && dy==0) result.animation_facing=3;
    else if (dx==0 && dy==0xffffffffU) result.animation_facing=0;
    else if (dx==0 && dy==1) result.animation_facing=2;
    if (result.animation_facing) result.sounds.push_back(context.attack_sound);
    if (random.mt.bounded(2)==0 || state.kicks[selected]>2) {
        result.messages.push_back("You shatter the door with your magnificent foot.");
        result.destroyed_parts.push_back(selected);
        for (std::size_t i=selected;i>0;) result.destroyed_parts.push_back(--i);
        for (std::size_t i=selected+1;i<3;++i) result.destroyed_parts.push_back(i);
        for (auto p:door.parts) layout.at(p.x,p.y).kind=1;
        state.destroyed=true;result.sounds.push_back("breakable");return result;
    }
    result.messages.push_back("You kick the door but it resists your efforts!");
    state.kicks[selected]=signed_bits(std::uint32_t(state.kicks[selected])+1U);
    if (random.mt.bounded(3)==0) {
        result.messages.push_back("That hurt your foot!");DoorDamageRequest damage;
        if (random.mt.bounded(2)==0) damage.death_reason="in an accident of podiatry";
        else damage.death_reason=context.female?"by sticking her foot through Death's door":"by sticking his foot through Death's door";
        result.damage=std::move(damage);
    }
    result.sounds.push_back("locked");
    const auto threshold=std::max(signed_bits(100U-std::uint32_t(context.stealth)),std::int32_t(0));
    for (auto& listener:context.listeners) {
        const auto x=std::uint32_t(listener.position.x)-std::uint32_t(position.x);
        const auto y=std::uint32_t(listener.position.y)-std::uint32_t(position.y);
        if (float(signed_bits(x*x+y*y))>=25.0f) continue;
        const auto roll=(std::uint64_t(random.libc.next())*0x0c7ffffbU)>>52;
        if (std::int64_t(roll)>=threshold) continue;
        if ((listener.flags&2) && listener.tile_kind!=14)
            result.messages.push_back("A nearby monster has woken up as a result of your door kicking!");
        listener.flags&=~2U;
    }
    return result;
}
}
DoorClickResult click_door(DungeonLayout& layout,DoorState& door,std::size_t selected,
    DoorClickContext& context,GameRandom& random,const std::function<bool(Position)>& obstructed) {
    if (selected>=3 || door.destroyed) throw std::invalid_argument("invalid door click target");
    for (auto p:door.assembly.parts) (void)layout.at(p.x,p.y);
    for (auto quantity:context.belt_picks) if (quantity<0) throw std::invalid_argument("negative lockpick quantity");
    for (auto quantity:context.bag_picks) if (quantity<0) throw std::invalid_argument("negative lockpick quantity");
    auto cells=layout;auto state=door;auto input=context;auto rng=random;
    auto result=apply(cells,state,selected,input,rng,obstructed);
    layout=std::move(cells);door=state;context=std::move(input);random=rng;return result;
}
}
