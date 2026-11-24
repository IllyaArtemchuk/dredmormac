#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("unpaid goods assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
World room() {std::istringstream input("#####\n#@..#\n#...#\n#####\n");return World::read(input);}
int main() {
    try {
        InventoryItem item;item.instance=1;item.kind=ItemKind::armour;item.definition=0;item.for_sale=true;
        Inventory::Bag bag;Inventory::Belt belt;Equipment::Slots equipment;check(!carried_unpaid_goods(bag,belt,equipment));
        for (auto& slot:bag) {slot=item;check(carried_unpaid_goods(bag,belt,equipment));slot.reset();}
        for (auto& slot:belt) {slot=item;check(carried_unpaid_goods(bag,belt,equipment));slot.reset();}
        for (auto& slot:equipment) {slot=item;check(carried_unpaid_goods(bag,belt,equipment));slot.reset();}
        check(carried_unpaid_goods(bag,belt,equipment,item));auto paid=item;paid.for_sale=false;check(!carried_unpaid_goods(bag,belt,equipment,paid));
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{read_item(parse_xml("<item name='Cap'><armour type='head'/></item>"))});
        Session session(room(),make_player({3,2,2}),1);session.equipment_profile({3,2,2},definitions);session.drop(item,{1,1});
        check(!session.has_unpaid_goods() && parse_xml(saved(session)).attribute("version")=="19");session.pickup();check(session.has_unpaid_goods());
        check(session.equip(1,EquipmentSlot::head) && session.has_unpaid_goods());const auto bytes=saved(session);std::istringstream input(bytes);auto restored=Session::load(input,definitions);
        check(saved(restored)==bytes && restored.has_unpaid_goods() && restored.equipment().slots()[0]->for_sale);
        check(restored.unequip(EquipmentSlot::head) && restored.has_unpaid_goods());const auto found=restored.inventory().find(1);check(found && found->for_sale);
        for (auto invalid:{std::string("for-sale=\"2\""),std::string("for-sale=\"-1\"")}) {
            auto wrong=bytes;const auto pos=wrong.find("for-sale=\"1\"");check(pos!=std::string::npos);wrong.replace(pos,12,invalid);bool rejected=false;
            try {std::istringstream bad(wrong);Session::load(bad,definitions);} catch(const std::exception&) {rejected=true;}check(rejected);
        }
        auto old=bytes;old.replace(old.find("version=\"19\""),12,"version=\"18\"");bool rejected=false;try {std::istringstream bad(old);Session::load(bad,definitions);} catch(const std::exception&) {rejected=true;}check(rejected);
        Session traveller(room(),make_player({3,2,2}),9);traveller.equipment_profile({3,2,2},definitions);traveller.floor_stairs({{{2,1},false,0}});
        SessionFloor lower(room());lower.depth.depth=1;lower.stairs={{{2,1},true,0}};item.instance=0;lower.ground.push_back({item,{3,1}});FloorRegistry floors;floors.add_floor(traveller,std::move(lower));
        std::ostringstream run;floors.save(traveller,run);std::istringstream run_input(run.str());auto loaded=FloorRegistry::load(run_input,definitions);check(loaded.second.cached()[0].ground[0].item.for_sale);
        check(loaded.second.step(loaded.first,Action::east)==FloorTravel::travelled && loaded.first.depth_context().depth==1);
        loaded.second.step(loaded.first,Action::east);loaded.first.pickup();check(loaded.first.has_unpaid_goods());
        check(loaded.second.step(loaded.first,Action::west)==FloorTravel::travelled && loaded.first.depth_context().depth==0 && loaded.first.has_unpaid_goods());
        std::cout<<"all unpaid query slots, equipped/bag/ground persistence, format gates, cached floor ownership and travel passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
