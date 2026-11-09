#pragma once
#include "dredmor/inventory.hpp"

namespace dredmor {
struct EquippedStats {
    PrimaryStats primary;
    SecondaryStats secondary;
    Damage damage_bonus{},resistance{};
};
class Equipment {
public:
    using Slots=std::array<std::optional<InventoryItem>,12>;
    const Slots& slots() const { return slots_; }
    [[nodiscard]] std::optional<InventoryItem> equip(const InventoryItem& item,EquipmentSlot slot,
                                                   const std::vector<ItemDefinition>& definitions);
    std::optional<InventoryItem> remove(EquipmentSlot slot);
    EquippedStats rebuild(const ClassRanks& ranks,const std::vector<ItemDefinition>& definitions,
                          const PrimaryStats& primary_flat={},const SecondaryStats& secondary_flat={}) const;
private:
    Slots slots_{};
};
// Transfer/replacement is atomic: failure leaves both containers unchanged.
bool equip_from_inventory(Inventory& inventory,Equipment& equipment,std::uint64_t instance,
                          EquipmentSlot slot,const std::vector<ItemDefinition>& definitions);
bool unequip_to_inventory(Inventory& inventory,Equipment& equipment,EquipmentSlot slot);
}
