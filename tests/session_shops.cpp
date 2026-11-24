#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("session shop assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
World room() {std::istringstream in("#########\n#@......#\n#.......#\n#.......#\n#.......#\n#########\n");return World::read(in);}
std::string saved(const Session& s) {std::ostringstream out;s.save(out);return out.str();}
std::shared_ptr<const std::vector<ItemDefinition>> definitions() {
    return std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{read_item(parse_xml("<item name='Sword' type='0'><weapon/><price amount='10'/></item>")),read_item(parse_xml("<item name='Cap'><armour type='head'/><price amount='10'/></item>"))});
}
ItemFactoryTables tables() {ItemFactoryTables result;auto defs=definitions();for (std::size_t i=0;i<defs->size();++i) register_factory_item(result,i,defs->at(i),15);return result;}
Actor keeper(GameRandom& random) {random.libc.next();random.libc.next();Actor result;result.name="Keeper";result.sprite="keeper.spr";result.health=result.secondary[0]=10;return result;}
std::size_t create(Session& s) {
    ShopRoomMarkers points{{5,1},{6,2},{{6,3},{7,3}}};std::vector<Position> tiles;
    for (int y=1;y<=4;++y) for (int x=4;x<=7;++x) tiles.push_back({x,y});return s.create_shop(points,tiles,tables(),{},keeper);
}
Session fixture() {Session s(room(),make_player({3,2,2}),101);s.equipment_profile({3,2,2},definitions());create(s);return s;}
Session loaded(const Session& s) {std::istringstream in(saved(s));return Session::load(in,definitions());}
int main() {
    try {
        auto s=fixture();check(s.shops().size()==1 && s.shops()[0].keeper.id==2 && s.shops()[0].pedestals[0].item->instance==1 && s.shops()[0].pedestals[1].item->instance==2);
        check(!s.passable({6,2}) && !s.passable({6,3}));GameRandom expected;expected.seed(101);const auto variant=expected.libc.next()>>29;keeper(expected);ItemFactoryState state;
        for (int i=0;i<2;++i) {const auto item=make_shop_stock(*definitions(),tables(),{},i+1,expected,state);check(item.definition==s.shops()[0].pedestals[i].item->definition);}
        check(s.shops()[0].variant==variant && parse_xml(saved(s)).attribute("version")=="20");
        check(parse_xml(saved(s)).first("random")->attribute("libc")==std::to_string(expected.libc.state()));check(saved(loaded(s))==saved(s));
        const auto before=saved(s);bool rejected=false;try {ShopRoomMarkers bad{{5,1},{6,2},{{6,3}}};s.create_shop(bad,{},tables(),{},keeper);} catch(const std::exception&) {rejected=true;}check(rejected && saved(s)==before);
        const auto declined=s.use_shop_pedestal(0,0,1,1.f);check(declined.handled && s.shops()[0].pedestals[0].item && saved(s)==before);
        const auto poor=s.use_shop_pedestal(0,0,0,1.f);check(poor.insufficient_funds && s.shops()[0].pedestals[0].item && s.player().zorkmids==0);
        s.give_currency(100);const auto bought=s.use_shop_pedestal(0,0,0,1.f);check(bought.transfer_item && !bought.handled && s.player().zorkmids==90 && !s.shops()[0].pedestals[0].item && !s.has_unpaid_goods());
        check(s.inventory().find(1) && !s.inventory().find(1)->for_sale && !s.passable({6,3}));check(saved(loaded(s))==saved(s));
        for (int i=0;i<3;++i) s.step(Action::east);check(s.shop_presence().in_shop);
        const auto theft=s.use_shop_pedestal(0,1,2,1.f);check(theft.transfer_item && theft.handled && s.has_unpaid_goods() && s.player().zorkmids==90 && s.shops()[0].guarding && s.shops()[0].keeper.position.x==5 && !s.passable({5,1}));
        s=loaded(s);check(s.shops()[0].guarding && s.has_unpaid_goods());s.step(Action::west);
        check(!s.shop_presence().in_shop && s.shop_presence().level_angry && s.shop_presence().music_override && s.shops()[0].angry && !s.has_unpaid_goods());
        s.refresh_shop_guards();check(!s.shops()[0].guarding && s.shops()[0].keeper.position.x==6 && saved(loaded(s))==saved(s));
        auto cart=fixture();cart.give_currency(100);for (int i=0;i<3;++i) cart.step(Action::east);cart.use_shop_pedestal(0,0,2,1.f);
        check(cart.has_unpaid_goods() && cart.shop_debt(1.f)==10 && cart.shops()[0].guarding);
        const auto declined_debt=cart.settle_shop_debt(0,0,1.f);check(declined_debt.debt_dialog && !declined_debt.payment && cart.player().zorkmids==100 && cart.has_unpaid_goods());
        const auto paid_debt=cart.settle_shop_debt(0,1,1.f);check(paid_debt.payment && cart.player().zorkmids==90 && !cart.has_unpaid_goods() && !cart.shops()[0].guarding && cart.shops()[0].keeper.position.x==6);
        auto chat=cart.settle_shop_debt(0,0,1.f);check(chat.chat_line==0 && cart.shop_chat_counter()==1);cart=loaded(cart);check(cart.shop_chat_counter()==1);
        for (int i=0;i<10;++i) cart.settle_shop_debt(0,0,1.f);check(cart.shop_chat_counter()==0 && saved(loaded(cart))==saved(cart));
        auto no_money=fixture();no_money.use_shop_pedestal(0,0,2,1.f);check(no_money.settle_shop_debt(0,1,1.f).insufficient_funds && no_money.has_unpaid_goods());
        auto selling=fixture();InventoryItem stack;stack.instance=100;stack.kind=ItemKind::weapon;stack.definition=0;stack.weapon_type=0;stack.quantity=3;stack.stackable=true;selling.inventory().insert(stack,false);
        const auto sale_before=saved(selling);check(selling.sell_inventory_item(0,100,0,.5f).quote==15 && saved(selling)==sale_before);
        check(selling.sell_inventory_item(0,100,1,0.f).refused_price && selling.inventory().find(100));
        const auto sold=selling.sell_inventory_item(0,100,1,.5f);check(sold.destroy_item && selling.player().zorkmids==15 && !selling.inventory().find(100) && saved(loaded(selling))==saved(selling));
        stack.instance=101;stack.for_sale=true;selling.inventory().insert(stack,false);check(selling.sell_inventory_item(0,101,1,.5f).refused_unpaid && selling.inventory().find(101));
        auto full=fixture();for (std::uint64_t i=10;i<58;++i) {InventoryItem item;item.instance=i;item.kind=ItemKind::armour;item.definition=1;check(full.inventory().insert(item,false).outcome==InsertOutcome::placed);}
        full.give_currency(100);full.use_shop_pedestal(0,0,0,1.f);check(full.player().zorkmids==90 && !full.shops()[0].pedestals[0].item && full.ground_items().size()==1 && !full.ground_items()[0].item.for_sale);
        auto corrupt=saved(s);const auto id=corrupt.find("<keeper id=\"2\"");check(id!=std::string::npos);corrupt.replace(id,14,"<keeper id=\"1\"");rejected=false;try {std::istringstream in(corrupt);Session::load(in,definitions());} catch(const std::exception&) {rejected=true;}check(rejected);
        auto travel=fixture();travel.floor_stairs({{{2,1},false,0}});Session lower(room(),make_player({3,2,2}),3);lower.equipment_profile({3,2,2},definitions());lower.depth_context({1,false,-1});lower.floor_stairs({{{2,1},true,0}});create(lower);
        auto floor=lower.floor_state();floor.shops[0].keeper.id=0;for (auto& pedestal:floor.shops[0].pedestals) pedestal.item->instance=0;FloorRegistry registry;registry.add_floor(travel,std::move(floor));
        check(registry.cached()[0].shops[0].keeper.id!=travel.shops()[0].keeper.id && registry.cached()[0].shops[0].pedestals[0].item->instance>travel.shops()[0].pedestals[1].item->instance);
        std::ostringstream run;registry.save(travel,run);std::istringstream run_input(run.str());auto restored=FloorRegistry::load(run_input,definitions());
        check(restored.second.step(restored.first,Action::east)==FloorTravel::travelled && restored.first.depth_context().depth==1 && restored.first.shops().size()==1);
        check(restored.second.travel(restored.first,true)==FloorTravel::travelled && restored.first.depth_context().depth==0 && restored.first.shops()[0].pedestals[1].item->for_sale);
        std::cout<<"owned keeper/stock constructor order, atomic placement, buy/decline/theft, guard/anger, full inventory drop, saves and cached shop identities passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
