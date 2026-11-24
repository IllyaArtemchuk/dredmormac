#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("armour constructor assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& s) {std::ostringstream out;s.save(out);return out.str();}
int main() {
    try {
        const auto neck=read_item(parse_xml("<item name='Pendant' level='1'><armour type='neck'/></item>"));
        const auto head=read_item(parse_xml("<item name='Cap'><armour type='head'/></item>"));
        const auto legs=read_item(parse_xml("<item name='Legs'><armour type='legs'/></item>"));
        check(neck.armour_type==9 && armour_constructor_rules(neck).type==6 && armour_constructor_rules(neck).randoms);
        check(head.armour_type==0 && armour_constructor_rules(head).type==5 && legs.armour_type==6 && armour_constructor_rules(legs).type==11);
        const auto overridden=read_item(parse_xml("<item name='Ring'><armour type='ring' randoms='1' RANDOMS='0'/></item>"));check(!armour_constructor_rules(overridden).randoms);
        const auto unknown=read_item(parse_xml("<item name='Alias'><armour type='head'/><armour type='hands' TYPE='unknown'/></item>"));check(armour_constructor_rules(unknown).type==5);
        const auto words=read_artifact_words(parse_xml("<text><startPhoneme text='A'/><phoneme text='b'/><title text='Keeper'/><noun text='Thing' plural='Things'/><adjective text='Bright'/><quality text='polished'/><material text='tin'/><decor text='dots'/></text>"));
        ItemFactoryContext context;context.artifact_words=&words;GameRandom random;random.seed(1);
        const auto plain=construct_factory_item(neck,0,1,context,random,true);check(!plain.artifact && random.libc.state()==1 && random.mt.state().index==624);
        check(!construct_factory_item(head,0,1,context,random).artifact && !construct_factory_item(legs,0,1,context,random).artifact && random.libc.state()==1 && random.mt.state().index==624);
        const auto enchanted=construct_factory_item(neck,0,1,context,random);check(enchanted.artifact && enchanted.artifact->quality==1 && !enchanted.artifact->name.empty());
        GameRandom rollback;rollback.seed(1);ItemFactoryContext missing;bool rejected=false;
        try {construct_factory_item(neck,0,1,missing,rollback);} catch(const std::invalid_argument&) {rejected=true;}check(rejected && rollback.libc.state()==1 && rollback.mt.state().index==624);
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{neck});ItemFactoryTables tables;register_factory_item(tables,0,neck,1);
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({3,2,2}),3);session.equipment_profile({3,2,2},definitions);
        const auto id=session.create_ground_loot(tables,context,"armour","Pendant",1,false,{2,1});check(id && session.ground_items()[0].item.artifact);
        const auto before=saved(session);std::istringstream input(before);auto restored=Session::load(input,definitions);check(saved(restored)==before);
        for (auto* s:{&session,&restored}) s->create_ground_loot(tables,context,"armour","Pendant",1,false,{2,1});check(saved(session)==saved(restored));
        std::cout<<"native armour types versus legacy slots, randoms aliases/defaults, suppression, automatic artifacts and saved continuation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
