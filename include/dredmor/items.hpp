#pragma once
#include "dredmor/combat.hpp"
#include "dredmor/stats.hpp"
#include "dredmor/xml.hpp"

namespace dredmor {
enum class ItemKind : std::uint32_t {
    unknown=0, weapon=1, armour=2, potion=3, wand=4, food=5, misc=6, gem=7,
    mushroom=8, reagent=9, trap=11, toolkit=12
};
enum class EquipmentSlot : int {
    head=0, hand_left=1, hand_right=2, ring_left=3, ring_right=4, chest=5,
    legs=6, feet=7, ranged=8, neck=9, hands=10, waist=11
};
struct ItemModifiers {
    PrimaryStats primary{};
    SecondaryStats secondary{};
    Damage damage{},resistance{};
};
struct ItemDefinition {
    std::string name,icon,description;
    ItemKind kind=ItemKind::unknown;
    int price=0,level=0,weapon_type=-1,armour_type=-1,armour_ac=0,food_hp=0,food_mp=0;
    bool special=false,debug=false;
    Damage weapon_damage{};
    ItemModifiers modifiers;
    XmlNode source; // Preserve unsupported effects; never silently discard them.
    bool can_equip(EquipmentSlot slot) const;
};
// The original item callback compares ASCII tag/attribute names without case,
// and repeated stat IDs assign the last value rather than summing.
ItemDefinition read_item(const XmlNode& source);
}
