#include "dredmor/shops.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("shop action assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        ItemDefinition definition;definition.price=10;InventoryItem item;item.kind=ItemKind::potion;item.stackable=true;item.quantity=3;
        check(item_shop_cost(&definition,item)==30 && item_shop_cost(nullptr,item)==300);
        const auto buy=resolve_shop_pedestal(true,item_shop_cost(&definition,item),true,3,1.5f,0,200);
        check(!buy.handled && buy.transfer_item && !buy.item_for_sale && buy.scaled_cost==45 && buy.total_cost==135 && buy.balance==65 && buy.payment && buy.payment->delta==-135);
        const auto poor=resolve_shop_pedestal(true,30,true,3,1.5f,0,134);
        check(poor.insufficient_funds && !poor.transfer_item && !poor.payment && poor.balance==134 && !poor.handled);
        const auto decline=resolve_shop_pedestal(true,30,true,3,1.5f,1,200);
        check(decline.handled && !decline.transfer_item && !decline.payment && decline.balance==200);
        const auto take=resolve_shop_pedestal(true,30,true,3,1.5f,2,0);
        check(take.handled && take.transfer_item && take.item_for_sale && take.force_keeper_move && !take.payment && take.balance==0);
        const auto empty=resolve_shop_pedestal(false,30,true,3,1.5f,2,0);check(!empty.handled && !empty.transfer_item && empty.scaled_cost==0);
        const auto other=resolve_shop_pedestal(true,30,false,1,1.5f,-1,0);check(!other.handled && !other.transfer_item && other.scaled_cost==45);
        // A negative shop quote credits money through TakeZorkmids' give path.
        const auto credit=resolve_shop_pedestal(true,-7,false,1,1.f,0,0);check(credit.transfer_item && credit.balance==7 && credit.payment && credit.payment->money_sound);
        definition.price=100;item.kind=ItemKind::weapon;item.stackable=false;item.artifact=Artifact{};item.artifact->modifiers.primary[0]=-1;
        check(item_shop_cost(&definition,item)==325);item.kind=ItemKind::armour;check(item_shop_cost(&definition,item)==-35);
        item.artifact->modifiers={};check(item_shop_cost(&definition,item,true)==700);item.artifact.reset();check(item_shop_cost(&definition,item,true)==100);
        definition.price=std::numeric_limits<std::int32_t>::max();item.kind=ItemKind::potion;check(item_shop_cost(&definition,item)==definition.price);
        item.kind=ItemKind::weapon;check(item_shop_cost(&definition,item)==std::numeric_limits<std::int32_t>::min());
        bool rejected=false;try {resolve_shop_pedestal(true,std::numeric_limits<std::int32_t>::min(),false,1,1.f,0,0);} catch(const std::overflow_error&) {rejected=true;}check(rejected);
        std::cout<<"artifact shop costs, signed armour/absolute weapon bonuses, stack repricing, buy/decline/theft decisions and wallet limits passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
