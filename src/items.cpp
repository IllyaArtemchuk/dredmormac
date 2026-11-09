#include "dredmor/items.hpp"
#include "dredmor/legacy_number.hpp"
#include <charconv>
#include <stdexcept>

namespace dredmor {
namespace {
std::string lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return value;
}
std::string attr(const XmlNode& node,std::string_view key,std::string fallback={}) {
    for (const auto& pair:node.attributes) if (lower(pair.first)==key) return pair.second;
    return fallback;
}
int integer(const XmlNode& node,std::string_view key,int fallback=0) {
    auto value=attr(node,key);
    if (value.empty()) return fallback;
    int result;
    auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if (parsed.ec!=std::errc() || parsed.ptr!=value.data()+value.size())
        throw std::runtime_error("invalid item integer: "+std::string(key)+" in "+node.tag);
    return result;
}
void damage_attributes(const XmlNode& node,Damage& target) {
    for (const auto& attr:node.attributes)
        for (std::size_t i=0;i<damage_names.size();++i)
            if (lower(attr.first)==damage_names[i]) target[i]=integer(node,damage_names[i]);
}
}
ItemDefinition read_item(const XmlNode& source) {
    if (lower(source.tag)!="item") throw std::runtime_error("expected an item definition");
    ItemDefinition item;
    item.kind=ItemKind::reagent;
    item.source=source;
    item.name=attr(source,"name");item.icon=attr(source,"iconfile");
    item.level=integer(source,"level");item.weapon_type=integer(source,"type",0);
    item.special=integer(source,"special")!=0;item.debug=integer(source,"debug")!=0;
    item.alchemical=integer(source,"alchemical")!=0;
    if (item.name.empty()) throw std::runtime_error("item name missing");
    for (const auto& child:source.children) {
        const auto tag=lower(child.tag);
        if (tag=="weapon") { item.kind=ItemKind::weapon;damage_attributes(child,item.weapon_damage); }
        else if (tag=="armour") {
            item.kind=ItemKind::armour;item.armour_ac=integer(child,"ac");item.level=integer(child,"level",item.level);
            item.armour_type=0;
            const auto type=attr(child,"type");
            if (type=="head") item.armour_type=0;
            else if (type=="shield") item.armour_type=1;
            else if (type=="ring") item.armour_type=3;
            else if (type=="chest") item.armour_type=5;
            else if (type=="legs") item.armour_type=6;
            else if (type=="feet") item.armour_type=7;
            else if (type=="neck") item.armour_type=9;
            else if (type=="hands") item.armour_type=10;
            else if (type=="waist") item.armour_type=11;
            else if (type=="sleeve") item.armour_type=12;
        } else if (tag=="food") { item.kind=ItemKind::food;item.food_hp=integer(child,"hp");item.food_mp=integer(child,"mp"); }
        else if (tag=="potion") item.kind=ItemKind::potion;
        else if (tag=="wand") {
            item.kind=ItemKind::wand;item.wand_min_charge=integer(child,"mincharge",item.wand_min_charge);
            item.wand_max_charge=integer(child,"maxcharge",item.wand_max_charge);
        }
        else if (tag=="gem") item.kind=ItemKind::gem;
        else if (tag=="mushroom") item.kind=ItemKind::mushroom;
        else if (tag=="trap") {
            item.kind=ItemKind::trap;auto trap=item.trap.value_or(TrapDefinition{});
            const auto trigger=attr(child,"trigger"),mount=attr(child,"originmount"),facing=attr(child,"originfacing");
            trap.trigger=trigger=="once"?TrapTrigger::once:trigger=="charges"?TrapTrigger::charges:TrapTrigger::repeat;
            trap.mount=mount=="wall"?TrapMount::wall:mount=="floor"?TrapMount::floor:TrapMount::ceiling;
            if (facing=="north") trap.facing=TrapFacing::north;
            if (facing=="south") trap.facing=TrapFacing::south;
            if (facing=="west") trap.facing=TrapFacing::west;
            if (facing=="east") trap.facing=TrapFacing::east;
            trap.spell=attr(child,"casts");trap.charges=legacy_integer(attr(child,"charges","0"));
            trap.target_is_caster=legacy_integer(attr(child,"targetiscaster","0"))!=0;
            for (const auto& value:child.attributes) {
                if (lower(value.first)=="level") item.level=legacy_integer(value.second);
                if (lower(value.first)=="origin") trap.origin=value.second;
            }
            item.trap=std::move(trap);
        }
        else if (tag=="toolkit") item.kind=ItemKind::toolkit;
        else if (tag=="price") item.price=integer(child,"amount");
        else if (tag=="description") item.description=attr(child,"text");
        else if (tag=="primarybuff" || tag=="secondarybuff") {
            const auto id=integer(child,"id"),amount=integer(child,"amount");
            const auto count=tag=="primarybuff"?6:24;
            if (id<0 || id>=count) throw std::runtime_error("item buff ID out of bounds: "+item.name);
            if (tag=="primarybuff") item.modifiers.primary[id]=amount;
            else item.modifiers.secondary[id]=amount;
        } else if (tag=="damagebuff") damage_attributes(child,item.modifiers.damage);
        else if (tag=="resistbuff") damage_attributes(child,item.modifiers.resistance);
    }
    return item;
}
bool ItemDefinition::can_equip(EquipmentSlot slot) const {
    const auto target=static_cast<int>(slot);
    if (kind==ItemKind::weapon) {
        if (weapon_type==4) return target==8;
        if (weapon_type==5 || weapon_type==6) return false;
        return target==1 || target==2;
    }
    if (kind!=ItemKind::armour) return false;
    if (target==1 || target==2) return armour_type==1 || armour_type==2;
    if (target==3 || target==4) return armour_type==3 || armour_type==4;
    if (target<0 || target>11 || target==8) return false;
    return target==armour_type;
}
}
