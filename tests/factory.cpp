#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("loot assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        std::vector<ItemDefinition> items{
            read_item(parse_xml("<item name='Same'><potion/></item>")),
            read_item(parse_xml("<item name='Same'><wand mincharge='1' maxcharge='10'/></item>")),
            read_item(parse_xml("<item name='Fixture trap' level='1'><trap level='1'/></item>")),
            read_item(parse_xml("<item name='Fixture artifact' artifact='1'><weapon/></item>"))};
        for (int subtype=0;subtype<3;++subtype) {
            ItemDefinition item;item.builtin_subtype=subtype;item.kind=subtype==2?ItemKind::currency:ItemKind::misc;items.push_back(item);
        }
        ItemFactoryTables tables;tables.named[0]={0,3};tables.named[3]={1};tables.wands[0]={1};tables.traps[0]={2};
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(items);
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({3,2,2}),777);
        session.equipment_profile({3,2,2},definitions);ItemFactoryContext context;context.gold_per_level=10;context.trap_deviation=1.4f;
        auto result=session.create_ground_loot(tables,context,"wand","Same",0,true,{2,1});check(result.has_value());
        check(session.ground_items()[0].item.definition==0 && session.ground_items()[0].item.quantity==1);
        session.create_ground_loot(tables,context,"misc","lockpicks",4,false,{2,1});
        check(session.ground_items()[1].item.definition==4 && session.ground_items()[1].item.quantity==4);
        const auto before=saved(session);
        bool rejected=false;try {session.create_ground_loot(tables,context,"wand","random",1,false,{0,0});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(session)==before); // Rolls/ID allocation must also roll back.
        rejected=false;try {session.create_ground_loot(tables,context,"anything","Fixture artifact",1,false,{2,1});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && saved(session)==before);
        rejected=false;try {session.create_ground_loot(tables,context,"weapon","random",1,false,{2,1});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && saved(session)==before);
        check(!session.create_ground_loot(tables,context,"unknown","missing",1,false,{2,1}) && saved(session)==before);
        session.create_ground_loot(tables,context,"trap","random",3,false,{2,1});
        check(session.ground_items().back().item.definition==2 && session.ground_items().back().item.quantity==3);
        std::istringstream input(saved(session));auto restored=Session::load(input,definitions);
        for (int i=0;i<8;++i) {
            for (auto* current:{&session,&restored}) {
                current->create_ground_loot(tables,context,"wand","random",2,false,{2,1});
                current->create_ground_loot(tables,context,"trap","random",1,false,{2,1});
                current->create_ground_loot(tables,context,"zorkmids","ignored",5,false,{2,1});
                current->create_ground_loot(tables,context,"unknown","zorkmids",1,false,{2,1});
            }
            check(saved(session)==saved(restored));
        }
        session.step(Action::east);restored.step(Action::east);
        check(session.player().zorkmids>0 && saved(session)==saved(restored));
        // A first matching unsupported entry prevents a later valid duplicate.
        auto unknown=items;unknown[0].kind=ItemKind::unknown;GameRandom random;random.seed(123);const auto state=random.libc.state();
        check(!make_loot_item(unknown,tables,context,"unknown","Same",1,false,99,random) && random.libc.state()==state);
        std::cout<<"loot lookup ordering, full-session atomic failures, currency pickup and resumed mixed factory RNG passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
