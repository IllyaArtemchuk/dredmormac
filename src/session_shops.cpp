#include "dredmor/session.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace dredmor {
namespace {
bool same(Position a,Position b) {return a.x==b.x && a.y==b.y;}
}
void Session::validate_shop(const SessionShop& shop) const {
    if (shop.variant<0 || shop.variant>3 || !shop.keeper.id || !world_.passable(shop.keeper.position) ||
        !world_.passable(shop.guard) || !world_.passable(shop.home) || shop.tiles.size()>262144 || shop.pedestals.size()>10000 ||
        !shop.keeper.triggers.empty() || !shop.keeper.buffs.empty()) throw std::invalid_argument("invalid shop keeper/layout");
    std::set<std::pair<int,int>> tiles,pedestals;
    for (auto point:shop.tiles) if (!world_.passable(point) || !tiles.emplace(point.x,point.y).second) throw std::invalid_argument("invalid or duplicate shop tile");
    for (const auto& pedestal:shop.pedestals) {
        if (!world_.passable(pedestal.position) || !pedestals.emplace(pedestal.position.x,pedestal.position.y).second) throw std::invalid_argument("invalid or duplicate shop pedestal");
        if (pedestal.item) {
            const auto& item=*pedestal.item;
            if (!definitions_ || item.definition>=definitions_->size() || definitions_->at(item.definition).kind!=item.kind ||
                !item.instance || item.quantity<1 || !item.for_sale || (item.charges && (item.kind!=ItemKind::wand || *item.charges<0)) ||
                (item.artifact && item.kind!=ItemKind::weapon && item.kind!=ItemKind::armour)) throw std::invalid_argument("invalid owned shop item");
        }
    }
}
std::size_t Session::create_shop(const ShopRoomMarkers& points,std::vector<Position> tiles,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,const std::function<Actor(GameRandom&)>& constructor) {
    if (!definitions_ || !constructor || shops_.size()>=64 || next_actor_==std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("invalid shop construction context");
    auto next=*this;SessionShop shop;shop.variant=next.random_.libc.next()>>29;
    shop.keeper=constructor(next.random_);if (shop.keeper.id || !shop.keeper.alive() || shop.keeper.health>shop.keeper.secondary[0]) throw std::invalid_argument("invalid new shop keeper");
    shop.keeper.id=next.next_actor_++;shop.keeper.position=points.home;shop.guard=points.guard;shop.home=points.home;shop.tiles=std::move(tiles);shop.angry=shop_presence_.level_angry;
    auto identity=next.next_item_instance();
    for (auto position:points.pedestals) {
        if (identity==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("shop item sequence exhausted");
        auto item=make_shop_stock(*definitions_,tables,context,identity++,next.random_,next.factory_state_);shop.pedestals.push_back({position,std::move(item)});
    }
    next.validate_shop(shop);
    if (!next.passable(shop.keeper.position) || same(shop.keeper.position,player_.position)) throw std::invalid_argument("shop keeper placement obstructed");
    for (const auto& pedestal:shop.pedestals) if (!next.passable(pedestal.position) || same(pedestal.position,player_.position) || same(pedestal.position,shop.keeper.position)) throw std::invalid_argument("shop pedestal placement obstructed");
    next.shops_.push_back(std::move(shop));*this=std::move(next);return shops_.size()-1;
}
ShopPedestalAction Session::use_shop_pedestal(std::size_t shop_index,std::size_t pedestal_index,int choice,float scaler) {
    const auto& pedestal=shops_.at(shop_index).pedestals.at(pedestal_index);
    if (!player_.alive()) return {};
    const int cost=pedestal.item?item_shop_cost(&definitions_->at(pedestal.item->definition),*pedestal.item):0;
    auto action=resolve_shop_pedestal(bool(pedestal.item),cost,pedestal.item && pedestal.item->stackable,pedestal.item?pedestal.item->quantity:1,scaler,choice,player_.zorkmids);
    auto next=*this;next.events_.clear();
    if (action.insufficient_funds) next.events_.push_back({EventKind::shop_insufficient_funds,player_.id,pedestal.item->instance,action.total_cost});
    if (action.transfer_item) {
        auto item=*pedestal.item;item.for_sale=action.item_for_sale;next.shops_[shop_index].pedestals[pedestal_index].item.reset();
        if (action.payment) next.currency_transfer_impl(action.total_cost,CurrencyOperation::take);
        const auto inserted=next.inventory_.insert(item);
        if (inserted.outcome==InsertOutcome::full) {next.drop(item,next.player_.position);next.events_.push_back({EventKind::inventory_full,player_.id,item.instance});}
        next.events_.push_back({action.item_for_sale?EventKind::shop_theft:EventKind::shop_purchase,player_.id,item.instance,action.total_cost});
        if (action.force_keeper_move) next.refresh_shop_guards_impl();
    }
    *this=std::move(next);return action;
}
ShopSellAction Session::sell_inventory_item(std::size_t index,std::uint64_t instance,int choice,float scaler) {
    const auto& shop=shops_.at(index);const auto* item=inventory_.find(instance);
    if (!item || !player_.alive() || !shop.keeper.alive()) return {};
    const auto cost=item_shop_cost(definitions_?&definitions_->at(item->definition):nullptr,*item);
    auto action=resolve_keeper_sale(shop.angry,item->for_sale,cost,scaler,choice,player_.zorkmids);
    auto next=*this;next.events_.clear();
    if (action.destroy_item) {next.inventory_.remove(instance);next.currency_transfer_impl(action.quote,CurrencyOperation::give);}
    *this=std::move(next);return action;
}
std::int32_t Session::shop_debt(float scaler) const {
    ShopDebtEntries entries;std::size_t cursor=0;
    auto take=[&](const std::optional<InventoryItem>& item) {
        auto& entry=entries.at(cursor++);entry.present=bool(item);entry.unpaid=item && item->for_sale;
        if (entry.unpaid) entry.cost=item_shop_cost(definitions_?&definitions_->at(item->definition):nullptr,*item);
    };
    for (const auto& item:inventory_.bag()) take(item);for (const auto& item:inventory_.belt()) take(item);for (const auto& item:equipment_.slots()) take(item);
    return dredmor::shop_debt(entries,scaler);
}
ShopDebtAction Session::settle_shop_debt(std::size_t index,int choice,float scaler) {
    const auto& shop=shops_.at(index);const auto debt=shop_debt(scaler);
    auto action=resolve_shop_debt(shop.keeper.alive() && player_.alive(),shop.angry,has_unpaid_goods(),debt,choice,player_.zorkmids,shop_chat_counter_);
    auto next=*this;next.events_.clear();next.shop_chat_counter_=action.chat_counter;
    if (action.payment) next.currency_transfer_impl(debt,CurrencyOperation::take);
    if (action.clear_unpaid) {next.inventory_.clear_unpaid_flags();next.equipment_.clear_unpaid_flags();}
    if (action.force_keeper_move) next.refresh_shop_guards_impl();
    if (action.insufficient_funds) next.events_.push_back({EventKind::shop_insufficient_funds,player_.id,shop.keeper.id,debt});
    *this=std::move(next);return action;
}
void Session::refresh_shop_guards_impl() {
    std::vector<ShopKeeperPosition> positions;
    for (const auto& shop:shops_) positions.push_back({shop.keeper.position,shop.guard,shop.home,shop.guarding});
    move_shop_keepers(positions,shop_presence_.in_shop,has_unpaid_goods());
    for (std::size_t i=0;i<shops_.size();++i) {shops_[i].keeper.position=positions[i].position;shops_[i].guarding=positions[i].guarding;}
}
void Session::refresh_shop_guards() {auto next=*this;next.refresh_shop_guards_impl();*this=std::move(next);}
void Session::update_shop_presence_impl() {
    bool inside=false;
    for (const auto& shop:shops_) for (auto point:shop.tiles) if (same(point,player_.position)) inside=true;
    const auto transition=step_shop_presence(inside?13:1,shop_presence_,has_unpaid_goods());shop_presence_=transition.state;
    for (auto event:transition.events) {
        if (event==ShopPresenceEvent::anger_keepers) {
            for (auto& shop:shops_) shop.angry=true;
            inventory_.clear_unpaid_flags();equipment_.clear_unpaid_flags();
        }
        events_.push_back({EventKind::shop_presence,player_.id,0,std::int32_t(event)});
    }
}
void Session::update_shop_presence() {auto next=*this;next.events_.clear();next.update_shop_presence_impl();*this=std::move(next);}
}
