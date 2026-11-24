#pragma once
#include "dredmor/currency.hpp"
#include "dredmor/shop_layout.hpp"
#include "dredmor/equipment.hpp"

namespace dredmor {
// Original virtual Item/Weapon/Armour::GetShopCost. A missing definition uses
// the base100 fallback. Artifact spell presence is explicit until that part of
// artifact headers is represented by the item model.
std::int32_t item_shop_cost(const ItemDefinition* definition,const InventoryItem& item,bool artifact_spell=false);
// Actual PlayerHasStolenGoods examines all bag/belt/equipment entries and the
// mouse-held item; an unpaid item alone is enough, regardless of quantity/kind.
bool carried_unpaid_goods(const Inventory::Bag& bag,const Inventory::Belt& belt,
    const Equipment::Slots& equipment,const std::optional<InventoryItem>& held=std::nullopt);
// ClearStolenGoods omits the equipped ranged weapon; preserve that omission.
void clear_unpaid_goods(Inventory::Bag& bag,Inventory::Belt& belt,
    Equipment::Slots& equipment,std::optional<InventoryItem>& held);
enum class ShopPresenceEvent {shop_music,free_music,anger_keepers,theft_dialog,thief_music,rotate_music};
struct ShopPresenceState {bool in_shop=false,music_override=false,level_angry=false;};
struct ShopPresenceTransition {ShopPresenceState state;std::vector<ShopPresenceEvent> events;};
// Shop portion of DoSteppedOn, before random colour text and tile callbacks.
ShopPresenceTransition step_shop_presence(std::uint8_t tile_kind,ShopPresenceState state,bool stolen_goods);
struct ShopPedestalAction {
    bool handled=false,transfer_item=false,item_for_sale=true,force_keeper_move=false,insufficient_funds=false;
    std::int32_t scaled_cost=0,total_cost=0,balance=0;
    std::optional<CurrencyTransfer> payment;
};
// Raw ShopPedestal::Clicked decision: UI choice0 buys,1 declines,2 takes without
// payment. Empty pedestals do nothing. The original applies quantity again after
// the virtual shop cost (whose base already includes stack quantity).
ShopPedestalAction resolve_shop_pedestal(bool has_item,std::int32_t shop_cost,bool stackable,
    std::int32_t quantity,float buy_scaler,int dialog_choice,std::int32_t balance);
}
