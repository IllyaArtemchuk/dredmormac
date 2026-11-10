#pragma once
#include "dredmor/combat.hpp"
#include "dredmor/stats.hpp"
#include "dredmor/xml.hpp"

namespace dredmor {
enum class ItemKind : std::uint32_t {
    unknown=0, weapon=1, armour=2, potion=3, wand=4, food=5, misc=6, gem=7,
    mushroom=8, reagent=9, currency=10, toolkit=11, trap=12
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
enum class TrapTrigger {once,charges,repeat};
enum class TrapMount {floor,wall,ceiling};
enum class TrapFacing {south,west,north,east};
struct TrapDefinition {
    TrapTrigger trigger=TrapTrigger::repeat;
    TrapMount mount=TrapMount::ceiling;
    TrapFacing facing=TrapFacing::south;
    int charges=0;
    bool target_is_caster=false;
    std::string spell,origin;
};
struct ItemDefinition {
    std::string name,icon,description;
    ItemKind kind=ItemKind::unknown;
    int price=0,level=0,weapon_type=-1,armour_type=-1,armour_ac=0,food_hp=0,food_mp=0;
    bool special=false,debug=false,alchemical=false;
    int wand_min_charge=1,wand_max_charge=10,builtin_subtype=-1;
    Damage weapon_damage{};
    ItemModifiers modifiers;
    std::optional<TrapDefinition> trap;
    std::string cast_spell;
    bool food_meat=false;
    XmlNode source; // Preserve unsupported effects; never silently discard them.
    bool can_equip(EquipmentSlot slot) const;
};
// The original item callback compares ASCII tag/attribute names without case,
// and repeated stat IDs assign the last value rather than summing.
ItemDefinition read_item(const XmlNode& source);
}
