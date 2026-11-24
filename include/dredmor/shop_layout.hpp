#pragma once
#include "dredmor/world.hpp"
#include <string>
#include <vector>

namespace dredmor {
struct ShopRoomMarkers {
    Position guard{},home{};
    std::vector<Position> pedestals;
};
// Source row order is retained for stock; lowercase s is the guard point,
// uppercase S the keeper home. Last occurrences replace earlier point markers.
ShopRoomMarkers read_shop_markers(const std::vector<std::string>& rows,int width,int height);
struct ShopKeeperPosition {Position position{},guard{},home{};bool guarding=false;};
// ForceShopkeeperMove ignores its int argument. It uses global inShop and
// PlayerHasStolenGoods; only state transitions teleport a keeper.
std::vector<std::size_t> move_shop_keepers(std::vector<ShopKeeperPosition>& keepers,bool in_shop,bool stolen_goods);
}
