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
                               const PrimaryStats& primary_flat,const SecondaryStats& secondary_flat) const {
    auto pflat=primary_flat;auto sflat=secondary_flat;
    EquippedStats result;int armour=0;
    for (const auto& slot:slots_) if (slot) {
        const auto& definition=definitions.at(slot->definition);
        for (std::size_t i=0;i<6;++i) pflat[i]=add(pflat[i],definition.modifiers.primary[i]);
        for (std::size_t i=0;i<24;++i) sflat[i]=add(sflat[i],definition.modifiers.secondary[i]);
        if (definition.kind==ItemKind::armour) armour=add(armour,definition.armour_ac);
        for (std::size_t i=0;i<16;++i) {
            result.damage_bonus[i]=add(result.damage_bonus[i],definition.modifiers.damage[i]);
            result.resistance[i]=add(result.resistance[i],definition.modifiers.resistance[i]);
        }
    }
    result.primary=derive_primary(ranks,pflat);
    result.secondary=derive_secondary(result.primary,sflat);
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
