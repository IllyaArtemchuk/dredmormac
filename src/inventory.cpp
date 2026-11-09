#include "dredmor/inventory.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
InventoryItem make_inventory_item(const ItemDefinition& definition,std::uint32_t index,
                                 std::uint64_t instance,DarwinRandom& random) {
    if (!instance) throw std::invalid_argument("item instance must be nonzero");
    InventoryItem item;item.instance=instance;item.definition=index;item.kind=definition.kind;item.weapon_type=definition.weapon_type;
    switch (definition.kind) {
    case ItemKind::weapon:item.stackable=definition.weapon_type==5 || definition.weapon_type==6;break;
    case ItemKind::armour:break;
    case ItemKind::currency:item.currency_value=1;break;
    case ItemKind::potion:case ItemKind::food:case ItemKind::gem:case ItemKind::mushroom:
    case ItemKind::reagent:case ItemKind::toolkit:case ItemKind::trap:item.stackable=true;break;
    case ItemKind::misc:
        if (definition.builtin_subtype<0 || definition.builtin_subtype>1) throw std::invalid_argument("unsupported builtin item");
        item.stackable=true;break;
    case ItemKind::wand: {
        const auto minimum=definition.wand_min_charge,maximum=definition.wand_max_charge;
        if (minimum<0 || maximum<minimum) throw std::invalid_argument("invalid wand charge range");
        item.charges=minimum+static_cast<std::int32_t>(random.bounded(static_cast<std::uint32_t>(maximum-minimum)));
        break;
    }
    default:throw std::invalid_argument("unsupported item kind");
    }
    return item;
}
namespace {
bool belt_eligible(const InventoryItem& item) {
    return item.kind==ItemKind::food || item.kind==ItemKind::potion || item.kind==ItemKind::wand ||
           item.kind==ItemKind::mushroom || item.kind==ItemKind::trap ||
           (item.kind==ItemKind::weapon && (item.weapon_type==5 || item.weapon_type==6));
}
}
const InventoryItem* Inventory::find(std::uint64_t instance) const {
    for (const auto& slot:bag_) if (slot && slot->instance==instance) return &*slot;
    for (const auto& slot:belt_) if (slot && slot->instance==instance) return &*slot;
    return nullptr;
}
void Inventory::validate_new(const InventoryItem& item) const {
    if (!item.instance || item.quantity<1) throw std::invalid_argument("invalid inventory instance/quantity");
    if (item.charges && (item.kind!=ItemKind::wand || *item.charges<0)) throw std::invalid_argument("invalid item charges");
    if (item.artifact && item.kind!=ItemKind::weapon && item.kind!=ItemKind::armour) throw std::invalid_argument("invalid artifact item kind");
    if (find(item.instance)) throw std::invalid_argument("inventory instance already owned");
}
Inventory Inventory::restore(const Bag& bag,const Belt& belt) {
    Inventory inventory;
    for (std::size_t i=0;i<bag.size();++i) if (bag[i]) {
        inventory.validate_new(*bag[i]);inventory.bag_[i]=bag[i];
    }
    for (std::size_t i=0;i<belt.size();++i) if (belt[i]) {
        inventory.validate_new(*belt[i]);inventory.belt_[i]=belt[i];
    }
    return inventory;
}
InsertResult Inventory::insert(const InventoryItem& item,bool prefer_belt) {
    validate_new(item);
    auto merge=[&](auto& slots,InventoryLocation location)->std::optional<InsertResult> {
        for (std::size_t i=0;i<slots.size();++i) {
            auto& slot=slots[i];
            if (slot && slot->kind==item.kind && slot->definition==item.definition && slot->stackable) {
                const std::uint32_t sum=std::uint32_t(slot->quantity)+std::uint32_t(item.quantity);
                std::memcpy(&slot->quantity,&sum,4);
                return InsertResult{InsertOutcome::stacked,InventoryPosition{location,i},slot->instance};
            }
        }
        return std::nullopt;
    };
    if (prefer_belt) if (auto result=merge(belt_,InventoryLocation::belt)) return *result;
    if (auto result=merge(bag_,InventoryLocation::bag)) return *result;
    auto place_first=[&](auto& slots,InventoryLocation location)->std::optional<InsertResult> {
        for (std::size_t i=0;i<slots.size();++i) if (!slots[i]) {
            slots[i]=item;return InsertResult{InsertOutcome::placed,InventoryPosition{location,i},item.instance};
        }
        return std::nullopt;
    };
    if (prefer_belt && belt_eligible(item)) if (auto result=place_first(belt_,InventoryLocation::belt)) return *result;
    if (auto result=place_first(bag_,InventoryLocation::bag)) return *result;
    return {InsertOutcome::full,std::nullopt,0};
}
bool Inventory::place(const InventoryItem& item,int x,int y) {
    validate_new(item);
    if (x<0 || y<0 || x>=int(width) || y>=int(height)) return false;
    auto& slot=bag_[static_cast<std::size_t>(y)*width+static_cast<std::size_t>(x)];
    if (slot) return false;
    slot=item;return true;
}
std::optional<InventoryItem> Inventory::remove(std::uint64_t instance) {
    for (auto* slots:{&bag_[0],&belt_[0]}) {
        const auto count=slots==&bag_[0]?capacity:belt_capacity;
        for (std::size_t i=0;i<count;++i) if (slots[i] && slots[i]->instance==instance) {
            auto item=slots[i];slots[i].reset();return item;
        }
    }
    return std::nullopt;
}
std::optional<InventoryItem> Inventory::split_one(std::uint64_t instance,std::uint64_t new_instance) {
    if (!new_instance || find(new_instance)) throw std::invalid_argument("split requires a fresh instance ID");
    for (auto* slots:{&bag_[0],&belt_[0]}) {
        const auto count=slots==&bag_[0]?capacity:belt_capacity;
        for (std::size_t i=0;i<count;++i) if (slots[i] && slots[i]->instance==instance) {
            if (!slots[i]->stackable || slots[i]->quantity<2) return remove(instance);
            auto split=*slots[i];split.instance=new_instance;split.quantity=1;--slots[i]->quantity;return split;
        }
    }
    return std::nullopt;
}
}
