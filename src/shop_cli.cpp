#include "dredmor/shops.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--sell") {
            bool angry,unpaid;std::int32_t cost,choice,balance;std::uint32_t bits;
            while (std::cin>>angry>>unpaid>>cost>>bits>>choice>>balance) {
                float scaler;std::memcpy(&scaler,&bits,4);const auto action=resolve_keeper_sale(angry,unpaid,cost,scaler,choice,balance);
                std::cout<<action.handled<<' '<<action.monster_fallback<<' '<<action.refused_unpaid<<' '<<action.refused_price<<' '<<action.quote_dialog<<' '<<action.destroy_item<<' '<<action.quote<<' '<<action.balance<<' '<<bool(action.payment)<<' '<<(action.payment?action.payment->delta:0)<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--debt") {
            bool present,angry;int choice;std::int32_t balance,counter;std::uint32_t bits;
            while (std::cin>>present>>angry>>choice>>balance>>counter>>bits) {
                ShopDebtEntries entries;bool unpaid=false;
                for (auto& entry:entries) {unsigned state;std::cin>>state>>entry.cost;if (!std::cin || state>2) throw std::invalid_argument("invalid shop debt fixture");entry.present=state!=0;entry.unpaid=state==2;unpaid=unpaid || entry.unpaid;}
                float scaler;std::memcpy(&scaler,&bits,4);const auto debt=shop_debt(entries,scaler);const auto action=resolve_shop_debt(present,angry,unpaid,debt,choice,balance,counter);
                std::cout<<(action.debt_dialog?debt:0)<<' '<<action.debt_dialog<<' '<<action.insufficient_funds<<' '<<action.clear_unpaid<<' '<<action.force_keeper_move<<' '<<action.balance<<' '<<action.chat_counter<<' '<<action.chat_line<<' '<<bool(action.payment)<<' '<<(action.payment?action.payment->delta:0)<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--presence") {
            unsigned kind;ShopPresenceState state;bool stolen;
            while (std::cin>>kind>>state.in_shop>>state.music_override>>state.level_angry>>stolen) {
                if (kind>255) throw std::invalid_argument("invalid shop tile");const auto transition=step_shop_presence(std::uint8_t(kind),state,stolen);
                std::cout<<transition.state.in_shop<<' '<<transition.state.music_override<<' '<<transition.state.level_angry<<' '<<transition.events.size();for (auto event:transition.events) std::cout<<' '<<int(event);std::cout<<'\n';
            }
        } else if (argc==2 && (std::string(argv[1])=="--unpaid" || std::string(argv[1])=="--unpaid-clear")) {
            unsigned state;
            while (std::cin>>state) {
                Inventory::Bag bag;Inventory::Belt belt;Equipment::Slots equipment;std::optional<InventoryItem> held;std::size_t n=0;
                auto take=[&](auto& slot) {if (n++) std::cin>>state;if (!std::cin || state>2) throw std::invalid_argument("invalid unpaid fixture");if (state) {InventoryItem item;item.for_sale=state==2;slot=item;}};
                for (auto& item:bag) take(item);for (auto& item:belt) take(item);for (auto& item:equipment) take(item);take(held);
                std::cout<<carried_unpaid_goods(bag,belt,equipment,held);
                if (std::string(argv[1])=="--unpaid-clear") {
                    clear_unpaid_goods(bag,belt,equipment,held);std::cout<<' '<<carried_unpaid_goods(bag,belt,equipment,held);
                    for (const auto& item:bag) std::cout<<' '<<bool(item && item->for_sale);for (const auto& item:belt) std::cout<<' '<<bool(item && item->for_sale);for (const auto& item:equipment) std::cout<<' '<<bool(item && item->for_sale);std::cout<<' '<<bool(held && held->for_sale);
                }
                std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--markers") {
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
