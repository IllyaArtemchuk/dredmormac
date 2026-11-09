#include "dredmor/artifact.hpp"
#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("artifact assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string state(GameRandom random) {
    std::ostringstream out;out<<random.libc.state()<<' '<<random.normal.state().available<<' '<<random.normal.state().spare;
    for (int i=0;i<16;++i) out<<' '<<random.mt.u32();return out.str();
}
int main() {
    try {
        const auto words=read_artifact_words(parse_xml("<text><StartPhoneme Text='A'/><phoneme text='b'/><title text='Keeper'/><noun text='Thing' plural='Things'/><adjective text='Bright'/><quality text='polished'/><material text='tin'/><decor text='dots'/></text>"));
        GameRandom random;random.seed(123);ArtifactRequest request;request.quality=10;request.base_description="Base";
        // Expected values from the original full routine over these synthetic pools.
        const auto artifact=create_artifact(words,request,random);
        check(artifact.name=="Ab, the Bright Thing" && artifact.quality==10);
        check(artifact.description=="It is studded with polished tin and is decorated with polished dots and polished dots.\n\n");
        check(artifact.buff_description=="Base "+artifact.description);
        SecondaryStats secondary{};secondary[13]=3;Damage damage{},resist{};damage[12]=damage[14]=2;resist[1]=2;resist[4]=1;
        check(artifact.modifiers.primary==PrimaryStats{} && artifact.modifiers.secondary==secondary);
        check(artifact.modifiers.damage==damage && artifact.modifiers.resistance==resist && artifact.weapon_damage==damage);
        check(random.libc.state()==1184067167 && random.mt.u32()==2694306016U);
        GameRandom generated,fixed;generated.seed(123);fixed.seed(123);
        create_artifact(words,request,generated);request.fixed_name="Fixed fixture name";const auto named=create_artifact(words,request,fixed);
        check(named.name==*request.fixed_name && state(generated)==state(fixed)); // Generation still occurs before fixed-name replacement.
        request.kind=ItemKind::armour;fixed.seed(123);const auto armour=create_artifact(words,request,fixed);
        check(armour.weapon_damage==Damage{} && armour.modifiers.damage==damage && state(generated)==state(fixed));
        const auto before=state(fixed);bool rejected=false;
        try {create_artifact(words,request,fixed,0);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && state(fixed)==before);
        auto incomplete=words;incomplete.materials.clear();rejected=false;
        try {create_artifact(incomplete,request,fixed);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && state(fixed)==before);
        request.quality=-1;const auto empty=create_artifact(words,request,fixed);
        check(empty.quality==-1 && empty.modifiers.primary==PrimaryStats{} && empty.modifiers.secondary==SecondaryStats{} && empty.modifiers.damage==Damage{});
        auto weapon=read_item(parse_xml("<item name='Fixture weapon' type='0'><weapon/></item>"));weapon.weapon_damage[0]=3;
        auto cap=read_item(parse_xml("<item name='Fixture artifact cap' artifact='1'><armour type='head'/><artifact quality='5'/></item>"));
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{weapon,cap});
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({3,2,2}),123);
        session.equipment_profile({3,2,2},definitions);const auto base=session.player();
        const auto id=session.create_ground_artifact(0,10,words,{2,1});
        auto saved=[](const Session& value){std::ostringstream out;value.save(out);return out.str();};
        const auto ground_save=saved(session);std::istringstream ground_input(ground_save);auto restored=Session::load(ground_input,definitions);
        check(saved(restored)==ground_save && restored.ground_items()[0].item.artifact->name==artifact.name);
        session.step(Action::east);session.pickup();check(session.inventory().find(id)->artifact.has_value());
        check(session.equip(id,EquipmentSlot::hand_left));
        check(session.player().damage[0]==3 && session.player().damage[12]==2 && session.player().damage[14]==2);
        check(session.player().resistance==resist && session.player().secondary[13]==base.secondary[13]+3);
        const auto equipped_save=saved(session);std::istringstream equipped_input(equipped_save);auto equipped=Session::load(equipped_input,definitions);
        check(saved(equipped)==equipped_save && equipped.equipment().slots()[1]->artifact->weapon_damage==damage);
        for (auto* current:{&session,&equipped}) {
            current->create_ground_artifact(1,99,words,current->player().position);current->pickup();
        }
        check(saved(session)==saved(equipped));
        const auto* created_cap=session.inventory().bag()[0]?&*session.inventory().bag()[0]:nullptr;
        check(created_cap && created_cap->artifact && created_cap->artifact->quality==5 && created_cap->artifact->name==cap.name);
        check(session.unequip(EquipmentSlot::hand_left));
        check(session.player().primary==base.primary && session.player().secondary==base.secondary && session.player().damage==base.damage && session.player().resistance==base.resistance);
        auto malformed=ground_save;auto pos=malformed.find("quality=\"10\"");check(pos!=std::string::npos);
        malformed.replace(pos,std::string("quality=\"10\"").size(),"quality=\"2147483648\"");rejected=false;
        try {std::istringstream bad(malformed);Session::load(bad,definitions);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected);
        const auto unchanged=saved(session);rejected=false;
        try {session.create_ground_artifact(0,10,words,{0,0});} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(session)==unchanged);
        std::istringstream resource_map("#####\n#@..#\n#...#\n#####\n");Session resources(World::read(resource_map),make_player({3,2,2}),1);
        resources.equipment_profile({3,2,2},definitions);const auto initial_resources=resources.player();
        InventoryItem resource_cap{900,ItemKind::armour,1,0,1,false};Artifact bonus;bonus.modifiers.secondary[0]=5;bonus.modifiers.secondary[1]=7;resource_cap.artifact=bonus;
        resources.inventory().insert(resource_cap);check(resources.equip(900,EquipmentSlot::head));
        check(resources.player().health==initial_resources.health+5 && resources.player().mana==initial_resources.mana+7);
        auto injured=saved(resources);const auto health_text="hp=\""+std::to_string(resources.player().health)+"\"";
        pos=injured.find(health_text);check(pos!=std::string::npos);injured.replace(pos,health_text.size(),"hp=\"1\"");
        std::istringstream injured_input(injured);auto injured_loaded=Session::load(injured_input,definitions);
        check(injured_loaded.unequip(EquipmentSlot::head) && injured_loaded.player().health==1);
        check(resources.unequip(EquipmentSlot::head) && resources.player().health==initial_resources.health && resources.player().mana==initial_resources.mana);
        std::cout<<"original artifact fixture, fixed-name RNG, armour damage distinction and atomic resource guards passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
