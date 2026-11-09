#include "dredmor/equipment.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t add(std::int32_t a,std::int32_t b) {
    std::uint32_t bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;
}
}
std::optional<InventoryItem> Equipment::equip(const InventoryItem& item,EquipmentSlot slot,
                                           const std::vector<ItemDefinition>& definitions) {
    const auto index=static_cast<std::size_t>(slot);
    if (index>=slots_.size() || item.definition>=definitions.size()) throw std::invalid_argument("invalid equipment slot/definition");
    const auto& definition=definitions[item.definition];
    if (!definition.can_equip(slot) || definition.kind!=item.kind || item.quantity!=1 || !item.instance)
        throw std::invalid_argument("item cannot be equipped in this slot");
    for (const auto& current:slots_) if (current && current->instance==item.instance)
        throw std::invalid_argument("equipment instance already worn");
    auto previous=slots_[index];slots_[index]=item;return previous;
}
std::optional<InventoryItem> Equipment::remove(EquipmentSlot slot) {
    const auto index=static_cast<std::size_t>(slot);
    if (index>=slots_.size()) throw std::invalid_argument("invalid equipment slot");
    auto previous=slots_[index];slots_[index].reset();return previous;
}
EquippedStats Equipment::rebuild(const ClassRanks& ranks,const std::vector<ItemDefinition>& definitions,
                               const PrimaryStats& primary_flat,const SecondaryStats& secondary_flat,
                               const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities) const {
    auto pflat=primary_flat;auto sflat=secondary_flat;
    EquippedStats result;int armour=0;
    for (std::size_t index=0;index<slots_.size();++index) if (const auto& slot=slots_[index]) {
        const auto& definition=definitions.at(slot->definition);
        for (std::size_t i=0;i<6;++i) pflat[i]=add(pflat[i],definition.modifiers.primary[i]);
        for (std::size_t i=0;i<24;++i) sflat[i]=add(sflat[i],definition.modifiers.secondary[i]);
        if (definition.kind==ItemKind::armour) armour=add(armour,definition.armour_ac);
        for (std::size_t i=0;i<16;++i) {
            if (definition.kind!=ItemKind::weapon || index!=8)
                result.damage_bonus[i]=add(result.damage_bonus[i],definition.modifiers.damage[i]);
            result.resistance[i]=add(result.resistance[i],definition.modifiers.resistance[i]);
        }
        if (slot->artifact) {
            const auto& artifact=*slot->artifact;
            for (std::size_t i=0;i<6;++i) pflat[i]=add(pflat[i],artifact.modifiers.primary[i]);
            for (std::size_t i=0;i<24;++i) sflat[i]=add(sflat[i],artifact.modifiers.secondary[i]);
            for (std::size_t i=0;i<16;++i) {
                // Weapon-owned buff damage is excluded by original Rebuild;
                // its extra damage lives on the weapon used for an attack.
                if (definition.kind==ItemKind::armour) result.damage_bonus[i]=add(result.damage_bonus[i],artifact.modifiers.damage[i]);
                result.resistance[i]=add(result.resistance[i],artifact.modifiers.resistance[i]);
            }
        }
        if (definition.kind==ItemKind::weapon) {
            const auto training=weapon_ability_modifiers(definition.weapon_type,skills,abilities);
            // The supported session path has at most one melee weapon. For
            // static two-hand rebuilding use canonical left-then-right order;
            // complete original dual-wield transition behavior remains pending.
            int divisor=1;
            if (index==2 && slots_[1] && slots_[1]->kind==ItemKind::weapon &&
                weapon_skill_id(definitions.at(slots_[1]->definition).weapon_type)==weapon_skill_id(definition.weapon_type)) divisor=2;
            for (std::size_t i=0;i<6;++i) pflat[i]=add(pflat[i],training.primary[i]/divisor);
            for (std::size_t i=0;i<24;++i) sflat[i]=add(sflat[i],training.secondary[i]/divisor);
            for (std::size_t i=0;i<16;++i) {
                if (index!=8) result.damage_bonus[i]=add(result.damage_bonus[i],training.damage[i]/divisor);
                result.resistance[i]=add(result.resistance[i],training.resistance[i]/divisor);
            }
        }
    }
    AbilityContext context;
    if (slots_[1]) {context.weapon_left=slots_[1]->kind==ItemKind::weapon;context.armour_left=slots_[1]->kind==ItemKind::armour;}
    if (slots_[2]) {context.weapon_right=slots_[2]->kind==ItemKind::weapon;context.armour_right=slots_[2]->kind==ItemKind::armour;}
    PassiveStatInput base;base.ranks=ranks;base.primary_flat=pflat;base.secondary_flat=sflat;
    const auto passive=derive_passives(base,skills,abilities,context);
    result.primary=passive.primary;result.secondary=passive.secondary;
    for (std::size_t i=0;i<16;++i) {
        result.damage_bonus[i]=add(result.damage_bonus[i],passive.damage[i]);
        result.resistance[i]=add(result.resistance[i],passive.resistance[i]);
    }
    result.secondary[10]=add(result.secondary[10],armour);
    return result;
}
bool equip_from_inventory(Inventory& inventory,Equipment& equipment,std::uint64_t instance,
                          EquipmentSlot slot,const std::vector<ItemDefinition>& definitions) {
    auto bag=inventory;auto worn=equipment;
    auto item=bag.remove(instance);
    if (!item) return false;
    try {
        auto displaced=worn.equip(*item,slot,definitions);
        if (displaced && bag.insert(*displaced,false).outcome==InsertOutcome::full) return false;
    } catch (const std::invalid_argument&) { return false; }
    inventory=std::move(bag);equipment=std::move(worn);return true;
}
bool unequip_to_inventory(Inventory& inventory,Equipment& equipment,EquipmentSlot slot) {
    auto bag=inventory;auto worn=equipment;
    auto item=worn.remove(slot);
    if (!item || bag.insert(*item,false).outcome==InsertOutcome::full) return false;
    inventory=std::move(bag);equipment=std::move(worn);return true;
}
}
