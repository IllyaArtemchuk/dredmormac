#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("food assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto items=std::make_shared<std::vector<ItemDefinition>>();
        for (const auto& xml:{"<item name='Meal'><food hp='3' mp='2' effect='Gift'/></item>","<item name='Meat'><food hp='2' meat='1'/></item>","<item name='Drink'><food mp='3'/></item>"}) items->push_back(read_item(parse_xml(xml)));
        const auto repeated=read_item(parse_xml("<item name='Mixed'><food HP=' 3tail' MP='2' meat='-1' effect='First'/><FOOD hp='5'/><casts spell='Second'/></item>"));check(repeated.food_hp==5 && repeated.food_mp==2 && repeated.food_meat && repeated.cast_spell=="Second");
        auto abilities=std::make_shared<std::vector<AbilityDefinition>>();
        abilities->push_back(read_ability(parse_xml("<ability name='Fixture' skill='0'/>")));
        auto spells=std::make_shared<std::vector<SpellDefinition>>();
        for (const auto& xml:{"<spell name='Drain' type='self'><effect type='heal' amount='-5'/><effect type='spellpoints' amount='-5'/></spell>","<spell name='Gift' type='self'><effect type='spellpoints' amount='-1'/></spell>","<spell name='Fallen Vegan' type='self'><effect type='spellpoints' amount='-2'/></spell>"}) spells->push_back(read_spell(parse_xml(xml)));
        auto fixture=[&](std::shared_ptr<const std::vector<AbilityDefinition>> profile,bool vampire=false) {
            std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({20,20,20}),1);std::vector<SkillRank> skills;for (int i=0;i<7;++i) skills.push_back({i,1});session.character_profile({20,20,20},items,skills,profile);session.spell_profile(spells);session.regeneration_profile(1000,1000,vampire);return session;
        };
        auto session=fixture(abilities);const auto hp=session.player().health,mp=session.player().mana;
        session.cast_spell(spells->at(0),{1,1},{1,1});const auto meal=session.create_ground_item(0,{1,1},2);session.pickup();check(session.eat_food(meal));
        check(session.regeneration()->food==2 && session.regeneration()->booze==1 && session.player().health==hp-4 && session.player().mana==mp-5 && session.inventory().find(meal)->quantity==1 && session.world().turn()==1);
        const auto pending=saved(session);std::istringstream input(pending);auto resumed=Session::load(input,items,abilities,spells);check(saved(resumed)==pending);
        session.step(Action::wait);resumed.step(Action::wait);check(saved(session)==saved(resumed) && session.regeneration()->food==1 && session.regeneration()->booze==0 && session.player().health==hp-3 && session.player().mana==mp-4);
        check(session.eat_food(meal) && !session.inventory().find(meal) && session.regeneration()->food==3 && session.regeneration()->booze==1);
        const auto before=saved(session);check(!session.eat_food(meal) && saved(session)==before);
        auto vampire=std::make_shared<std::vector<AbilityDefinition>>(*abilities);vampire->push_back(read_ability(parse_xml("<ability name='Vampirism' skill='0'/>")));session=fixture(vampire,true);
        auto meat=session.create_ground_item(1,{1,1});auto drink=session.create_ground_item(2,{1,1});session.pickup();check(!session.eat_food(meat) && session.world().turn()==0 && session.inventory().find(meat));check(session.eat_food(drink) && session.regeneration()->booze==2 && session.world().turn()==1);
        session=fixture(vampire);meat=session.create_ground_item(1,{1,1});session.pickup();check(session.eat_food(meat)); // An ability with that name alone is insufficient.
        session=fixture(abilities,true);meat=session.create_ground_item(1,{1,1});session.pickup();check(!session.eat_food(meat));
        auto vegan=std::make_shared<std::vector<AbilityDefinition>>(*abilities);vegan->push_back(read_ability(parse_xml("<ability name='Diet' skill='0'><flags veganism='1'/></ability>")));session=fixture(vegan);meat=session.create_ground_item(1,{1,1});session.pickup();const auto mana=session.player().mana;check(session.eat_food(meat) && session.player().mana==mana-2);
        auto hooks=std::make_shared<std::vector<AbilityDefinition>>(*abilities);hooks->push_back(read_ability(parse_xml("<ability name='Reactive' skill='0'><foodBuff name='Gift'/></ability>")));session=fixture(hooks);meat=session.create_ground_item(1,{1,1});session.pickup();const auto untouched=saved(session);bool rejected=false;try {session.eat_food(meat);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==untouched);
        auto changed=std::make_shared<std::vector<ItemDefinition>>(*items);changed->at(0).food_hp=4;std::istringstream mismatch(pending);rejected=false;try {Session::load(mismatch,changed,abilities,spells);} catch(const std::exception&) {rejected=true;}check(rejected);
        session=fixture(abilities);const auto meal2=session.create_ground_item(0,{1,1});session.pickup();auto unsupported=std::make_shared<std::vector<SpellDefinition>>(*spells);unsupported->at(1).effects[0].type="summon";session.spell_profile(unsupported);const auto before_effect=saved(session);rejected=false;try {session.eat_food(meal2);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==before_effect);
        std::cout<<"food counters, effect timing, stacked meals, diet gates, nutrition saves and unsupported-hook rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
