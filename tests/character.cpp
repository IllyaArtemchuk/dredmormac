#include "dredmor/character.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("character assertion at line "+std::to_string(__LINE__)); } while(false)
using namespace dredmor;
const InventoryItem* named(const Session& session,const GameContent& content,std::string_view name) {
    for (const auto& slot:session.inventory().bag()) if (slot && content.items()[slot->definition].name==name) return &*slot;
    for (const auto& slot:session.inventory().belt()) if (slot && content.items()[slot->definition].name==name) return &*slot;
    throw std::runtime_error("missing starting item: "+std::string(name));
}
std::string saved(const Session& session) {std::ostringstream output;session.save(output);return output.str();}
int main(int argc,char** argv) {
    try {
        check(argc==2);const auto content=GameContent::load(argv[1]);
        auto session=create_character(content,content.room("Starting Room").instantiate(),{16,15,14,13,12,11,10},123);
        check(session.skills().size()==7 && session.skills()[0].id==10 && session.skills()[6].id==16);
        check(session.equipment().slots()[0] && session.equipment().slots()[1] && session.equipment().slots()[5]);
        check(content.items()[session.equipment().slots()[5]->definition].name=="Leather Cuirass");
        check(named(session,content,"Fixture bolt")->quantity==4); // Original amount stack plus one extra.
        check(named(session,content,"Lockpick")->quantity==2 && named(session,content,"Spores")->quantity==1);
        check(named(session,content,"Fixture potion")->quantity==3 && named(session,content,"Fixture mushroom")->quantity==2);
        check(named(session,content,"Fixture reagent")->quantity==3 && named(session,content,"Fixture toolkit")->quantity==1);
        check(named(session,content,"Fixture food")->quantity==2 && named(session,content,"Fixture drink")->quantity==2);
        auto charges=named(session,content,"Fixture wand")->charges;check(charges && *charges>=2 && *charges<5);
        check(session.player().health==session.player().secondary[0] && session.player().mana==session.player().secondary[1]);
        auto items=std::make_shared<const std::vector<ItemDefinition>>(content.items());
        auto abilities=std::make_shared<const std::vector<AbilityDefinition>>(content.abilities());
        std::istringstream input(saved(session));auto loaded=Session::load(input,items,abilities);
        check(saved(loaded)==saved(session));session.step(Action::wait);loaded.step(Action::wait);check(saved(loaded)==saved(session));
        auto wizard=create_character(content,content.room("Starting Room").instantiate(),{17,18,19,20,21,22,23},1);
        check(wizard.equipment().slots()[5] && content.items()[wizard.equipment().slots()[5]->definition].name=="Robe");
        auto hatted=create_character(content,content.room("Starting Room").instantiate(),{14,17,18,19,20,21,22},1);
        check(hatted.equipment().slots()[0] && !hatted.equipment().slots()[5]);
        auto rogue=create_character(content,content.room("Starting Room").instantiate(),{24,25,26,27,28,29,30},1);
        check(rogue.equipment().slots()[5] && content.items()[rogue.equipment().slots()[5]->definition].name=="Fine Linen Shirt");
        const auto before=saved(session);bool rejected=false;
        try {session.drop(*session.equipment().slots()[0],session.player().position);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(session)==before);
        rejected=false;try {session.create_ground_item(3,{-1,-1});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(session)==before); // Invalid placement cannot consume wand RNG.
        const auto identifier=session.create_ground_item(3,session.player().position);
        check(session.ground_items().back().item.instance==identifier && session.ground_items().back().item.charges);
        std::cout<<"starting grants, quantities, equipment, clothes, charges, skills and atomic item creation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
