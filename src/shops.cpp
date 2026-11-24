#include "dredmor/shops.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t signed_bits(std::uint32_t bits) {std::int32_t value;std::memcpy(&value,&bits,4);return value;}
std::int32_t truncate(float value) {
    if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}
}
std::int32_t item_shop_cost(const ItemDefinition* definition,const InventoryItem& item,bool artifact_spell) {
    auto base=std::uint32_t(definition?definition->price:100)*(item.stackable?std::uint32_t(item.quantity):1U);
    if (item.kind!=ItemKind::weapon && item.kind!=ItemKind::armour) return signed_bits(base);
    float factor=1.0f;
    if (item.artifact && (item.kind==ItemKind::weapon || item.kind==ItemKind::armour)) {
        auto add=[&](std::int32_t value,std::uint32_t cost,float multiplier) {
            // Weapon uses abs with wrapping INT_MIN; armour keeps signed values.
            if (item.kind==ItemKind::weapon && value<0) value=signed_bits(0U-std::uint32_t(value));
            base+=std::uint32_t(value)*cost;
            const float extra=float(value)*multiplier;factor=factor+extra;
        };
        const auto& modifiers=item.artifact->modifiers;
        for (auto value:modifiers.primary) add(value,150U,0.3f);
        for (auto value:modifiers.secondary) add(value,100U,0.1f);
        for (auto value:modifiers.damage) add(value,100U,0.25f);
        for (auto value:modifiers.resistance) add(value,50U,0.1f);
        if (artifact_spell) {factor=factor+1.0f;base+=250U;}
    }
    const float scaled=float(signed_bits(base))*factor;return truncate(scaled);
}
ShopPedestalAction resolve_shop_pedestal(bool has_item,std::int32_t shop_cost,bool stackable,
    std::int32_t quantity,float buy_scaler,int dialog_choice,std::int32_t balance) {
    ShopPedestalAction result;result.balance=balance;
    if (!has_item) return result;
    const float scaled=float(shop_cost)*buy_scaler;result.scaled_cost=truncate(scaled);
    if (dialog_choice==1) {result.handled=true;return result;}
    if (dialog_choice==2) {result.handled=true;result.transfer_item=true;result.force_keeper_move=true;return result;}
    if (dialog_choice!=0) return result;
    result.total_cost=signed_bits(std::uint32_t(result.scaled_cost)*(stackable?std::uint32_t(quantity):1U));
    if (balance<result.total_cost) {result.insufficient_funds=true;return result;}
    result.payment=transfer_currency(balance,result.total_cost,CurrencyOperation::take);result.balance=result.payment->balance;
    result.transfer_item=true;result.item_for_sale=false;return result;
}
ShopRoomMarkers read_shop_markers(const std::vector<std::string>& rows,int width,int height) {
    if (width<1 || height<1 || width>512 || height>512 || rows.size()>std::size_t(height)) throw std::invalid_argument("invalid shop room dimensions");
    ShopRoomMarkers result;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(width));++x) {
        const Position point{int(x),int(y)};
        switch (rows[y][x]) {
        case 's':result.guard=point;break;
        case 'S':result.home=point;break;
        case 'i':result.pedestals.push_back(point);break;
        }
    }
    return result;
}
std::vector<std::size_t> move_shop_keepers(std::vector<ShopKeeperPosition>& keepers,bool in_shop,bool stolen_goods) {
    std::vector<std::size_t> moved;const bool guard=in_shop && stolen_goods;
    for (std::size_t i=0;i<keepers.size();++i) if (keepers[i].guarding!=guard) {
        auto& keeper=keepers[i];keeper.position=guard?keeper.guard:keeper.home;keeper.guarding=guard;moved.push_back(i);
    }
    return moved;
}

}
