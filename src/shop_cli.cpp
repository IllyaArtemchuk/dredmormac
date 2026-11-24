#include "dredmor/shops.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--markers") {
            int width,height;std::size_t count;
            while (std::cin>>width>>height>>count) {
                if (count>512) throw std::invalid_argument("too many shop rows");std::vector<std::string> rows(count);for (auto& row:rows) std::cin>>std::quoted(row);
                const auto markers=read_shop_markers(rows,width,height);std::cout<<markers.home.x<<' '<<markers.home.y<<' '<<markers.guard.x<<' '<<markers.guard.y<<' '<<markers.pedestals.size();
                for (auto point:markers.pedestals) std::cout<<' '<<point.x<<' '<<point.y;std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--guards") {
            bool in_shop,stolen;std::size_t count;
            while (std::cin>>in_shop>>stolen>>count) {
                if (count>4096) throw std::invalid_argument("too many shop keepers");std::vector<ShopKeeperPosition> keepers(count);
                for (auto& keeper:keepers) std::cin>>keeper.position.x>>keeper.position.y>>keeper.guard.x>>keeper.guard.y>>keeper.home.x>>keeper.home.y>>keeper.guarding;
                const auto moved=move_shop_keepers(keepers,in_shop,stolen);std::cout<<moved.size();for (auto i:moved) std::cout<<' '<<i;
                for (auto keeper:keepers) std::cout<<' '<<keeper.position.x<<' '<<keeper.position.y<<' '<<keeper.guarding;std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--click") {
            bool present,stack;std::int32_t cost,quantity,choice,balance;std::uint32_t scaler_bits;
            while (std::cin>>present>>cost>>stack>>quantity>>scaler_bits>>choice>>balance) {
                float scaler;std::memcpy(&scaler,&scaler_bits,4);const auto action=resolve_shop_pedestal(present,cost,stack,quantity,scaler,choice,balance);
                std::cout<<action.handled<<' '<<action.transfer_item<<' '<<action.item_for_sale<<' '<<action.force_keeper_move<<' '<<action.insufficient_funds<<' '<<action.scaled_cost<<' '<<action.total_cost<<' '<<action.balance<<' '<<bool(action.payment)<<' '<<(action.payment?action.payment->delta:0)<<'\n';
            }
        } else {
            unsigned kind;bool defined,stack,artifact,spell;std::int32_t price,quantity;
            while (std::cin>>kind>>defined>>price>>stack>>quantity>>artifact>>spell) {
                ItemDefinition definition;definition.price=price;InventoryItem item;item.kind=ItemKind(kind);item.stackable=stack;item.quantity=quantity;
                Artifact data;
                for (auto& value:data.modifiers.primary) std::cin>>value;for (auto& value:data.modifiers.secondary) std::cin>>value;
                for (auto& value:data.modifiers.damage) std::cin>>value;for (auto& value:data.modifiers.resistance) std::cin>>value;
                if (!std::cin) throw std::invalid_argument("incomplete shop price fixture");if (artifact) item.artifact=data;
                std::cout<<item_shop_cost(defined?&definition:nullptr,item,spell)<<'\n';
            }
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
