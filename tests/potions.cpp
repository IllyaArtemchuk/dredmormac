#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("potion assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto items=std::make_shared<std::vector<ItemDefinition>>();
        for (const auto& xml:{"<item name='Tonic'><potion spell='Effect'/></item>","<item name='Absent'><potion spell='Missing'/></item>","<item name='Empty'><potion/></item>","<item name='Broken'><potion spell='Unsupported'/></item>","<item name='Food'><food hp='2'/></item>"}) items->push_back(read_item(parse_xml(xml)));
        check(items->at(0).cast_spell=="Effect");
        check(read_item(parse_xml("<item name='Override'><potion spell='First'/><CaStS SPELL='Second'/></item>")).cast_spell=="Second");
        check(read_item(parse_xml("<item name='Retain'><potion spell='First'/><casts/></item>")).cast_spell=="First");
        auto spells=std::make_shared<std::vector<SpellDefinition>>();
        spells->push_back(read_spell(parse_xml("<spell name='Effect' type='self' downtime='99'><requirements mp='9999' zorkmids='9999'/><effect type='spellpoints' amount='-2'/></spell>")));
        spells->push_back(read_spell(parse_xml("<spell name='Unsupported' type='self'><effect type='summon'/></spell>")));
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");auto player=make_player({20,20,20});
        Session session(World::read(map),player,1);session.equipment_profile({20,20,20},items);session.spell_profile(spells);
        const auto mana=session.player().mana;
        auto add=[&](std::uint32_t definition,int quantity) {const auto id=session.create_ground_item(definition,session.player().position,quantity);session.pickup();return id;};
        const auto potion=add(0,3);check(session.inventory().belt()[0] && session.inventory().belt()[0]->instance==potion);
        check(session.drink_potion(potion) && session.inventory().belt()[0]->quantity==2 && session.player().mana==mana-2 && session.world().turn()==1 && session.spell_cooldown(0)==0);
        const auto pending=saved(session);std::istringstream input(pending);auto resumed=Session::load(input,items,{},spells);check(saved(resumed)==pending);
        check(session.drink_potion(potion) && resumed.drink_potion(potion) && saved(resumed)==saved(session));
        check(session.drink_potion(potion) && !session.inventory().find(potion) && session.player().mana==mana-6 && session.world().turn()==3);
        const auto before=saved(session);check(!session.drink_potion(potion) && saved(session)==before);
        const auto absent=add(1,1),empty=add(2,1);check(session.drink_potion(absent) && session.drink_potion(empty) && session.player().mana==mana-6 && session.world().turn()==5);
        const auto food=add(4,1);const auto prior=saved(session);check(!session.drink_potion(food) && saved(session)==prior);
        const auto broken=add(3,2);const auto snapshot=saved(session);bool rejected=false;try {session.drink_potion(broken);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==snapshot);
        auto changed=std::make_shared<std::vector<ItemDefinition>>(*items);changed->at(0).cast_spell="Missing";std::istringstream bad(pending);rejected=false;try {Session::load(bad,changed,{},spells);} catch(const std::exception&) {rejected=true;}check(rejected);
        Inventory bag;DarwinRandom random(1);auto item=make_inventory_item(items->at(0),0,123,random);item.quantity=2;bag.insert(item,false);
        check(bag.consume_one(123) && bag.bag()[0]->quantity==1 && bag.consume_one(123) && !bag.bag()[0] && !bag.consume_one(123));
        std::cout<<"potion spell refs, internal cost bypass, retained slots, stack consumption, save/reload and atomic failure passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
