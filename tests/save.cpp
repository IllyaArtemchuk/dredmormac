#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>
#define check(value) do { if (!(value)) throw std::runtime_error("save assertion at line "+std::to_string(__LINE__)); } while(false)
using namespace dredmor;
Session fixture() {
    std::istringstream room("#######\n#@....#\n#.....#\n#######\n");
    auto hero=make_player({3,2,2},"A&B \"fixture\"\nplayer");
    hero.secondary[2]=1;hero.secondary[6]=12;hero.secondary[7]=20;
    Session session(World::read(room),hero,1234);
    Actor enemy;enemy.position={2,1};enemy.name="Enemy <fixture>";enemy.health=enemy.secondary[0]=1000;
    enemy.secondary[2]=1;enemy.secondary[4]=10;enemy.secondary[6]=15;
    session.spawn_enemy(enemy);
    session.inventory().insert({100,ItemKind::food,0,-1,3,true});
    session.drop({101,ItemKind::armour,0,-1,1,false},{3,1});
    return session;
}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
std::shared_ptr<const std::vector<ItemDefinition>> legacy_profile_items(bool with_currency) {
    std::vector<ItemDefinition> items{read_item(parse_xml("<item name='Legacy fixture cap'><armour type='head' ac='1'/></item>"))};
    for (int i=0;i<2;++i) {ItemDefinition item;item.name=i?"Spores":"Lockpick";item.kind=ItemKind::misc;item.builtin_subtype=i;items.push_back(item);}
    if (with_currency) {ItemDefinition item;item.name="Zorkmids";item.kind=ItemKind::currency;item.builtin_subtype=2;items.push_back(item);}
    return std::make_shared<const std::vector<ItemDefinition>>(std::move(items));
}
std::shared_ptr<const std::vector<AbilityDefinition>> legacy_profile_abilities() {
    auto ability=read_ability(parse_xml("<ability name='Legacy fixture bonus' skill='0'><primarybuff id='0' amount='1'/><zorkmidbuff percent='0.25'/></ability>"));
    return std::make_shared<const std::vector<AbilityDefinition>>(std::vector<AbilityDefinition>{ability});
}
Session legacy_profile(bool with_currency) {
    std::istringstream room("#####\n#@..#\n#...#\n#####\n");Session session(World::read(room),make_player({3,2,2}),777);
    std::vector<SkillRank> skills;for (int id=0;id<7;++id) skills.push_back({id,1});
    session.character_profile({3,2,2},legacy_profile_items(with_currency),skills,legacy_profile_abilities());
    session.inventory().insert({500,ItemKind::armour,0,0,1,false});
    RoomStamp stamp{5,4,std::vector<TemplateCell>(20)};
    auto object=read_custom_room_object(parse_xml("<customengraving x='2' y='2' png='fixture.spr' name='Legacy fixture decor' id='fixture-id'/>"),{},stamp);
    session.create_custom_object(object);
    std::vector<TrapCandidate> pool;for (int level=1;level<=20;++level) pool.push_back({level,false,true});
    session.select_trap_candidate(pool,{2,false,-1},1.4f);return session;
}
void reject(std::string text) {
    bool rejected=false;
    try {std::istringstream input(text);Session::load(input);} catch (const std::exception&) {rejected=true;}
    check(rejected);
}
int main(int argc,char** argv) {
    try {
        auto original=fixture();for (int i=0;i<7;++i) original.step(Action::east);
        const auto text=saved(original);std::istringstream input(text);auto loaded=Session::load(input);
        check(original.snapshot()==loaded.snapshot() && saved(loaded)==text);
        for (int i=0;i<30;++i) {
            const auto action=i%3==0?Action::wait:Action::east;
            original.step(action);loaded.step(action);
            check(original.snapshot()==loaded.snapshot() && saved(original)==saved(loaded));
        }
        auto invalid=text;auto p=invalid.find("version=\"6\"");invalid.replace(p,11,"version=\"7\"");reject(invalid);
        auto legacy=text;legacy.replace(legacy.find("version=\"6\""),11,"version=\"1\"");
        p=legacy.find("<skills ");const auto end=legacy.find("</skills>\n",p);check(p!=std::string::npos && end!=std::string::npos);
        legacy.erase(p,end+10-p);std::istringstream legacy_input(legacy);auto legacy_loaded=Session::load(legacy_input);
        check(saved(legacy_loaded)==text);
        auto version_three=text;version_three.replace(version_three.find("version=\"6\""),11,"version=\"3\"");
        p=version_three.find("<custom-objects>");const auto objects_end=version_three.find("</custom-objects>\n",p);
        check(p!=std::string::npos && objects_end!=std::string::npos);version_three.erase(p,objects_end+18-p);
        std::istringstream version_three_input(version_three);check(saved(Session::load(version_three_input))==text);
        auto version_four=text;version_four.replace(version_four.find("version=\"6\""),11,"version=\"4\"");
        p=version_four.find(" normal-available=");const auto normal_end=version_four.find('>',p);
        check(p!=std::string::npos && normal_end!=std::string::npos);version_four.erase(p,normal_end-p);
        std::istringstream version_four_input(version_four);check(saved(Session::load(version_four_input))==text);
        auto version_five=text;version_five.replace(version_five.find("version=\"6\""),11,"version=\"5\"");
        std::istringstream version_five_input(version_five);check(saved(Session::load(version_five_input))==text);
        const std::string empty_normal="normal-available=\"0\" normal-spare=\"0\"";
        auto cached=text;p=cached.find(empty_normal);
        check(p!=std::string::npos);cached.replace(p,empty_normal.size(),"normal-available=\"1\" normal-spare=\"1067450368\"");
        std::istringstream cache_input(cached);check(saved(Session::load(cache_input))==cached);
        auto traps=fixture();std::vector<TrapCandidate> trap_pool;for (int level=1;level<=20;++level) trap_pool.push_back({level,false,true});
        traps.select_trap_candidate(trap_pool,{0,false,-1},1.4f);
        check(parse_xml(saved(traps)).first("random")->attribute("normal-available")=="1");
        std::istringstream trap_input(saved(traps));auto trap_loaded=Session::load(trap_input);
        for (int i=0;i<20;++i) {
            const SpawnDepthContext depth{i%10,i%3==0,i%7-1};
            const auto a=traps.select_trap_candidate(trap_pool,depth,1.4f),b=trap_loaded.select_trap_candidate(trap_pool,depth,1.4f);
            check(a.index==b.index && a.target_level==b.target_level && a.attempts==b.attempts && saved(traps)==saved(trap_loaded));
        }
        invalid=text;p=invalid.find("normal-available=\"0\"");invalid.replace(p,20,"normal-available=\"2\"");reject(invalid);
        invalid=text;p=invalid.find("normal-spare=\"0\"");invalid.replace(p,16,"normal-spare=\"4294967295\"");reject(invalid);
        invalid=text;p=invalid.find("index=\"624\"");check(p!=std::string::npos);invalid.replace(p,11,"index=\"626\"");reject(invalid);
        invalid=text;p=invalid.find("<enemy id=\"2\"");check(p!=std::string::npos);invalid.replace(p,13,"<enemy id=\"1\"");reject(invalid);
        reject("<!DOCTYPE d [<!ENTITY x 'bad'>]>"+text);reject("<dredmor-session/>");
        invalid=text;p=invalid.find("</dredmor-session>");invalid.insert(p,"<random/>");reject(invalid);
        auto dead=fixture();while (dead.player().alive()) dead.step(Action::wait);
        std::istringstream dead_input(saved(dead));auto dead_loaded=Session::load(dead_input);
        check(!dead_loaded.player().alive() && saved(dead_loaded)==saved(dead));
        auto definition=read_item(parse_xml("<item name='Fixture helmet'><armour type='head' ac='1'/><secondarybuff id='10' amount='2'/></item>"));
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{definition});
        std::istringstream room("#####\n#@..#\n#####\n");Session gear(World::read(room),make_player({3,2,2}),3);
        gear.equipment_profile({3,2,2},definitions);
        gear.inventory().insert({500,ItemKind::armour,0,-1,1,false},false);
        check(gear.equip(500,EquipmentSlot::head) && gear.player().secondary[10]==3);
        std::istringstream gear_input(saved(gear));auto gear_loaded=Session::load(gear_input,definitions);
        check(saved(gear_loaded)==saved(gear) && gear_loaded.equipment().slots()[0]->instance==500);
        check(gear.unequip(EquipmentSlot::head) && gear_loaded.unequip(EquipmentSlot::head));
        check(saved(gear)==saved(gear_loaded));
        auto changed=definition;changed.name="Different content";
        auto wrong=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{changed});
        bool mismatch=false;try {std::istringstream bad(saved(gear));Session::load(bad,wrong);} catch (const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        auto sword=read_item(parse_xml("<item name='Fixture sword' type='0'><weapon crushing='3'/></item>"));
        auto profile_items=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{definition,sword});
        auto passive=read_ability(parse_xml("<ability name='Fixture unarmed' skill='6'><primarybuff id='0' amount='3'/><damagebuff crushing='2'/></ability>"));
        passive.skill_name="Unarmed Combat";
        auto abilities=std::make_shared<const std::vector<AbilityDefinition>>(std::vector<AbilityDefinition>{passive});
        std::vector<SkillRank> selected;for (int id=0;id<7;++id) selected.push_back({id,1});
        std::istringstream profile_room("#####\n#@..#\n#####\n");Session profile(World::read(profile_room),make_player({3,2,2}),4);
        profile.character_profile({3,2,2},profile_items,selected,abilities);
        check(profile.player().primary[0]==13 && profile.player().health==25 && profile.player().damage[0]==2);
        profile.inventory().insert({700,ItemKind::weapon,1,0,1,false},false);
        const auto profile_text=saved(profile);std::istringstream profile_input(profile_text);
        auto profile_loaded=Session::load(profile_input,profile_items,abilities);
        check(saved(profile_loaded)==profile_text && profile_loaded.skills().size()==7);
        check(profile.equip(700,EquipmentSlot::hand_left) && profile_loaded.equip(700,EquipmentSlot::hand_left));
        check(profile.player().primary[0]==10 && profile.player().damage[0]==3 && saved(profile)==saved(profile_loaded));
        check(profile.unequip(EquipmentSlot::hand_left) && profile_loaded.unequip(EquipmentSlot::hand_left));
        check(profile.player().primary[0]==13 && profile.player().damage[0]==2 && saved(profile)==saved(profile_loaded));
        auto wrong_ability=passive;wrong_ability.modifiers.primary[0]=4;
        auto wrong_abilities=std::make_shared<const std::vector<AbilityDefinition>>(std::vector<AbilityDefinition>{wrong_ability});
        mismatch=false;try {std::istringstream bad(profile_text);Session::load(bad,profile_items,wrong_abilities);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        mismatch=false;try {std::istringstream bad(profile_text);Session::load(bad,profile_items);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        auto duplicate=profile_text;auto location=duplicate.find("<skill id=\"1\"");check(location!=std::string::npos);
        duplicate.replace(location,13,"<skill id=\"0\"");
        mismatch=false;try {std::istringstream bad(duplicate);Session::load(bad,profile_items,abilities);} catch(const std::runtime_error&) {mismatch=true;}
        check(mismatch);
        check(argc==3);
        // Generated by the v5 serializer from df8cac9, using only synthetic data.
        std::ifstream old_profile_input(argv[2]);check(old_profile_input.good());
        auto old_profile=Session::load(old_profile_input,legacy_profile_items(true),legacy_profile_abilities());
        auto reference_profile=legacy_profile(true);check(saved(old_profile)==saved(reference_profile));
        check(old_profile.player().zorkmids==0 && old_profile.custom_objects().size()==1);
        old_profile.create_ground_currency(100,old_profile.player().position);old_profile.pickup();
        reference_profile.create_ground_currency(100,reference_profile.player().position);reference_profile.pickup();
        check(old_profile.player().zorkmids==125 && saved(old_profile)==saved(reference_profile));
        old_profile.create_ground_currency(19,{3,2});const auto cash_text=saved(old_profile);
        std::istringstream cash_input(cash_text);auto cash_loaded=Session::load(cash_input,legacy_profile_items(true),legacy_profile_abilities());
        check(saved(cash_loaded)==cash_text && cash_loaded.ground_items()[0].item.currency_value==23);
        auto changed_currency_ability=std::make_shared<std::vector<AbilityDefinition>>(*legacy_profile_abilities());
        changed_currency_ability->at(0).currency_bonus=.5f;bool currency_mismatch=false;
        try {std::istringstream changed(cash_text);Session::load(changed,legacy_profile_items(true),changed_currency_ability);} catch(const std::runtime_error&) {currency_mismatch=true;}
        check(currency_mismatch);
        invalid=cash_text;p=invalid.find(" currency=\"23\"");check(p!=std::string::npos);invalid.erase(p,std::string(" currency=\"23\"").size());
        bool currency_missing=false;try {std::istringstream missing(invalid);Session::load(missing,legacy_profile_items(true),legacy_profile_abilities());} catch(const std::runtime_error&) {currency_missing=true;}
        check(currency_missing);
        invalid=text;p=invalid.find("zorkmids=\"0\"");check(p!=std::string::npos);invalid.replace(p,std::string("zorkmids=\"0\"").size(),"zorkmids=\"2147483648\"");reject(invalid);
        auto trap=read_item(parse_xml("<item name='Fixture trap'><trap/></item>"));
        auto toolkit=read_item(parse_xml("<item name='Fixture toolkit'><toolkit/></item>"));
        auto migrated_definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{trap,toolkit});
        std::ifstream legacy_types(argv[1]);check(legacy_types.good());
        auto migrated=Session::load(legacy_types,migrated_definitions);
        check(migrated.inventory().bag()[0]->instance==800 && migrated.inventory().bag()[0]->kind==ItemKind::trap);
        check(migrated.inventory().belt()[0]->instance==801 && migrated.inventory().belt()[0]->kind==ItemKind::toolkit);
        std::istringstream migrated_input(saved(migrated));auto migrated_again=Session::load(migrated_input,migrated_definitions);
        check(saved(migrated)==saved(migrated_again));
        auto builtin=ItemDefinition{};builtin.name="Lockpick";builtin.kind=ItemKind::misc;builtin.builtin_subtype=0;
        auto extended=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{trap,toolkit,builtin});
        std::ifstream old_with_builtins(argv[1]);auto appended=Session::load(old_with_builtins,extended);
        check(saved(appended)==saved(migrated));
        DarwinRandom item_random(1);appended.inventory().insert(make_inventory_item(builtin,2,802,item_random));
        std::istringstream appended_input(saved(appended));auto appended_loaded=Session::load(appended_input,extended);
        check(saved(appended)==saved(appended_loaded) && appended_loaded.inventory().find(802));
        auto wand=read_item(parse_xml("<item name='Fixture wand'><wand mincharge='2' maxcharge='3'/></item>"));
        auto wand_definitions=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{wand});
        std::istringstream wand_room("#####\n#@..#\n#####\n");Session wands(World::read(wand_room),make_player({3,2,2}),5);
        wands.equipment_profile({3,2,2},wand_definitions);
        auto wand_instance=make_inventory_item(wand,0,900,item_random);wand_instance.charges=0;
        wands.inventory().insert(wand_instance);
        std::istringstream wand_input(saved(wands));auto wand_loaded=Session::load(wand_input,wand_definitions);
        check(wand_loaded.inventory().find(900)->charges==0 && saved(wand_loaded)==saved(wands));
        auto decorated=fixture();CustomRoomDefinition object;object.placement.relative={3,2};object.placement.can_push=2;
        object.placement.id="fixture<&>\"";object.name="Fixture<&>\"\n";object.description="Details & more";
        object.sprite.series="dungeon/fixture";object.sprite.first=2;object.sprite.count=3;object.sprite.rate=150;
        object.passable=true;object.vis_block=true;decorated.create_custom_object(object);
        const auto decorated_text=saved(decorated);std::istringstream decorated_input(decorated_text);auto decorated_loaded=Session::load(decorated_input);
        check(saved(decorated_loaded)==decorated_text && decorated_loaded.snapshot()==decorated.snapshot());
        check(decorated_loaded.custom_objects().size()==1 && decorated_loaded.custom_objects()[0].can_push==2);
        check(decorated_loaded.custom_objects()[0].name==object.name && !decorated_loaded.custom_objects()[0].blocks_movement && decorated_loaded.custom_objects()[0].blocks_vision);
        for (int i=0;i<20;++i) {
            object.placement.percent=60;object.placement.relative={4,2};
            check(decorated.create_custom_object(object)==decorated_loaded.create_custom_object(object));
            check(saved(decorated)==saved(decorated_loaded));
        }
        for (const auto& replacement:std::vector<std::pair<std::string,std::string>>{{"kind=\"0\"","kind=\"2\""},{"push=\"2\"","push=\"256\""},
             {"movement=\"0\"","movement=\"2\""},{"x=\"3\" y=\"2\"","x=\"99\" y=\"2\""},{"count=\"3\"","count=\"bad\""}}) {
            auto malformed=decorated_text;const auto field=malformed.find(replacement.first,malformed.find("<object "));
            check(field!=std::string::npos);malformed.replace(field,replacement.first.size(),replacement.second);reject(malformed);
        }
        std::cout<<"save/load: exact state, future random turns, equipment, escaping and malformed-input rejection passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
