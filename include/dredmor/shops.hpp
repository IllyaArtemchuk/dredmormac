#pragma once
#include "dredmor/currency.hpp"

namespace dredmor {
// Original virtual Item/Weapon/Armour::GetShopCost. A missing definition uses
// the base100 fallback. Artifact spell presence is explicit until that part of
// artifact headers is represented by the item model.
std::int32_t item_shop_cost(const ItemDefinition* definition,const InventoryItem& item,bool artifact_spell=false);
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
