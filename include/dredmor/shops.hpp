#pragma once
#include "dredmor/currency.hpp"
#include "dredmor/shop_layout.hpp"
#include "dredmor/equipment.hpp"
#include <functional>

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
struct ShopDebtEntry {bool present=false,unpaid=false;std::int32_t cost=0;};
using ShopDebtEntries=std::array<ShopDebtEntry,70>; // bag48, belt9, gear12, held1.
// GoShopping omits equipped ranged gear from debt, although the query sees it.
std::int32_t shop_debt(const ShopDebtEntries& entries,float buy_scaler);
struct ShopDebtAction {
    bool debt_dialog=false,insufficient_funds=false,clear_unpaid=false,force_keeper_move=false;
    std::int32_t balance=0,chat_counter=0;
    int chat_line=-1;
    std::optional<CurrencyTransfer> payment;
};
ShopDebtAction resolve_shop_debt(bool keeper_present,bool keeper_angry,bool stolen_goods,
    std::int32_t debt,int choice,std::int32_t balance,std::int32_t chat_counter);
struct ShopSellAction {
    bool handled=false,monster_fallback=false,refused_unpaid=false,refused_price=false,refused_protected=false,quote_dialog=false,destroy_item=false;
    std::int32_t quote=0,balance=0;
    std::optional<CurrencyTransfer> payment;
};
// Shopkeeper::ItemUsed delegates angry keepers to Monster::ItemUsed. A calm
// keeper refuses unpaid or nonpositive offers; nonzero dialog choice sells.
ShopSellAction resolve_keeper_sale(bool angry,bool unpaid,std::int32_t shop_cost,float sell_scaler,int choice,std::int32_t balance);
// FastSellItem has no offer/positive-price gate. Context/hover selection and
// recipe/drag bookkeeping are outside this transfer decision.
ShopSellAction resolve_quick_sale(bool present,bool unpaid,ItemKind kind,int builtin_subtype,
    std::int32_t shop_cost,float sell_scaler,std::int32_t balance);
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
struct ShopkeeperUpdate {
    bool angry=false,monster_updated=false,location_queried=false,collector_spawned=false;
    std::optional<Position> collector_position;
    std::string flavour_text="You hear the sounds of a cash register.";
};
// Shopkeeper::Update wrapper: base monster update, quarter MT roll, current
// player site query, then Dread Collector construction/placement. Callbacks must
// act on provisional caller-owned state; this function commits RNG on success.
ShopkeeperUpdate update_shopkeeper(bool angry,bool level_angry,GameRandom& random,
    const std::function<void(GameRandom&)>& monster_update,
    const std::function<std::optional<Position>(GameRandom&)>& find_site,
    const std::function<void(Position,GameRandom&)>& spawn_collector);

}
