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
        rejected=false;try {session.create_ground_loot(tables,context,"anything","Fixture artifact",1,false,{2,1});} catch(const std::invalid_argument&) {rejected=true;}
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
        const auto words=read_artifact_words(parse_xml("<text><startPhoneme text='A'/><phoneme text='b'/><title text='Keeper'/><noun text='Thing' plural='Things'/><adjective text='Bright'/><quality text='polished'/><material text='tin'/><decor text='dots'/></text>"));
        std::vector<ItemDefinition> gear{
            read_item(parse_xml("<item name='Ordinary weapon' level='0' type='0'><weapon/></item>")),
            read_item(parse_xml("<item name='Unique weapon' level='1' type='0' artifact='1'><weapon/><artifact quality='3'/></item>")),
            read_item(parse_xml("<item name='Ordinary cap' level='0'><armour type='head'/></item>")),
            read_item(parse_xml("<item name='Named cap' level='1' artifact='1'><armour type='head'/><artifact quality='4'/></item>"))};
        ItemFactoryTables gear_tables;for (std::size_t i=0;i<gear.size();++i) register_factory_item(gear_tables,i,gear[i],15);
        auto gear_definitions=std::make_shared<const std::vector<ItemDefinition>>(gear);
        std::istringstream gear_map("#####\n#@..#\n#...#\n#####\n");Session unique(World::read(gear_map),make_player({3,2,2}),123);
        unique.equipment_profile({3,2,2},gear_definitions);ItemFactoryContext gear_context;gear_context.artifact_words=&words;gear_context.level_depth=8;
        const auto untouched=saved(unique);rejected=false;
        try {unique.create_ground_loot(gear_tables,gear_context,"weapon","random",1,true,{0,0});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(unique)==untouched);
        unique.create_ground_loot(gear_tables,gear_context,"weapon","random",1,true,{2,1});
        check(unique.ground_items().back().item.definition==1 && unique.ground_items().back().item.artifact->name=="Unique weapon");
        const auto unique_save=saved(unique);check(parse_xml(unique_save).first("spawned-artifacts")->text=="1 ");
        std::istringstream unique_input(unique_save);auto resumed=Session::load(unique_input,gear_definitions);
        for (int i=0;i<8;++i) {
            for (auto* current:{&unique,&resumed}) {
                current->create_ground_loot(gear_tables,gear_context,"weapon","random",1,true,{2,1});
                check(current->ground_items().back().item.definition==0 && current->ground_items().back().item.artifact.has_value());
                current->create_ground_loot(gear_tables,gear_context,"artifact","ignored",2,false,{2,1});
                current->create_ground_loot(gear_tables,gear_context,"ignored","Unique weapon",1,false,{2,1});
            }
            check(saved(unique)==saved(resumed));
        }
        auto bad=unique_save;auto start=bad.find("<spawned-artifacts>1 ");check(start!=std::string::npos);
        bad.replace(start,std::string("<spawned-artifacts>1 ").size(),"<spawned-artifacts>1 1 ");rejected=false;
        try {std::istringstream duplicate(bad);Session::load(duplicate,gear_definitions);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
        bad=unique_save;bad.replace(start,std::string("<spawned-artifacts>1 ").size(),"<spawned-artifacts>3 ");rejected=false;
        try {std::istringstream invalid(bad);Session::load(invalid,gear_definitions);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
        GameRandom plain;plain.seed(1);auto plain_armour=make_loot_item(gear,gear_tables,gear_context,"armour","random",1,false,1,plain);
        check(plain_armour && plain_armour->definition==3 && !plain_armour->artifact); // Root artifact flag alone does not generate in this route.
        std::cout<<"loot lookup ordering, full-session atomic failures, currency pickup and resumed mixed factory RNG passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
