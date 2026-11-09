#pragma once
#include "dredmor/items.hpp"
#include <optional>

namespace dredmor {
struct InventoryItem {
    std::uint64_t instance=0;
    ItemKind kind=ItemKind::unknown;
    std::uint32_t definition=0;
    int weapon_type=-1;
    std::int32_t quantity=1;
    bool stackable=false;
};
enum class InventoryLocation { bag,belt };
struct InventoryPosition { InventoryLocation location;std::size_t slot; };
enum class InsertOutcome { placed,stacked,full };
struct InsertResult {
    InsertOutcome outcome;
    std::optional<InventoryPosition> position;
    std::uint64_t destination=0;
};
class Inventory {
public:
    static constexpr std::size_t width=8,height=6,capacity=48,belt_capacity=9;
    using Bag=std::array<std::optional<InventoryItem>,capacity>;
    using Belt=std::array<std::optional<InventoryItem>,belt_capacity>;
    const Bag& bag() const { return bag_; }
    const Belt& belt() const { return belt_; }
    static Inventory restore(const Bag& bag,const Belt& belt);
    InsertResult insert(const InventoryItem& item,bool prefer_belt=true);
    bool place(const InventoryItem& item,int x,int y);
    std::optional<InventoryItem> remove(std::uint64_t instance);
    std::optional<InventoryItem> split_one(std::uint64_t instance,std::uint64_t new_instance);
    const InventoryItem* find(std::uint64_t instance) const;
private:
    Bag bag_{};Belt belt_{};
    void validate_new(const InventoryItem& item) const;
};
}
