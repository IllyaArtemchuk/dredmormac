#include "dredmor/content.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("content assertion at line "+std::to_string(__LINE__)); } while(false)
int main(int argc,char** argv) {
    try {
        check(argc==2);auto content=dredmor::GameContent::load(argv[1]);
        check(content.items().size()==7 && content.monsters().size()==2 && content.skills().size()==7);
        check(content.monster_tables().named[0]==std::vector<std::uint32_t>({1,0}) && content.monster_tables().named[3].empty());
        check(content.monster_tables().levels[0][0]==std::vector<std::uint32_t>({1,0}));
        std::vector<dredmor::MonsterDefinition> duplicate_monsters{
            dredmor::read_monster(dredmor::parse_xml("<monster name='Internal parent' displayName='Shared' special='1'/>")),
            dredmor::read_monster(dredmor::parse_xml("<monster name='Internal child' displayName='Shared' level='2'/>"))};
        dredmor::MonsterTables monster_tables;dredmor::register_monster(monster_tables,1,duplicate_monsters[1],7);
        dredmor::register_monster(monster_tables,0,duplicate_monsters[0],15);
        check(dredmor::find_registered_monster(monster_tables,duplicate_monsters,"Shared")==1);
        check(!dredmor::find_registered_monster(monster_tables,duplicate_monsters,"Internal child"));
        check(monster_tables.levels[2][0]==std::vector<std::uint32_t>({1}) && monster_tables.levels[2][3].empty());
        check(content.items()[4].builtin_subtype==0 && content.items()[5].builtin_subtype==1);
        check(content.items()[6].builtin_subtype==2 && content.items()[6].kind==dredmor::ItemKind::currency);
        check(content.abilities().size()==1 && content.abilities()[0].skill_name=="Fixture 0");
        check(content.spells().size()==3 && content.find_spell("Fixture spell")==&content.spells()[0]);
        check(!content.find_spell("fixture spell") && content.find_spell("Fixture spell")->effects[0].damage[1]==8);
        const auto scaled_effect=dredmor::read_spell_effect(dredmor::parse_xml("<effect TYPE='damage' crushing='4' CRUSHING='99' crushingF='0.5tail' CrushingF='9' amount=' -2suffix' secondaryScale='4' affectsCaster='-3'/>"));
        check(scaled_effect.damage[0]==4 && scaled_effect.scaling[0]==.5f && scaled_effect.amount==-2 && scaled_effect.secondary_scale==4 && scaled_effect.affects_caster==-3);
        check(scaled_effect.percent==100 && scaled_effect.primary_scale==-1 && scaled_effect.resistable && scaled_effect.affects_corpses && scaled_effect.floor_scale==1);
        auto world=content.room("Starting Room").instantiate();
        check(world.width()==7 && world.height()==5 && !world.passable({3,2}));
        check(world.player().x==3 && world.player().y==1);
        check(content.starting_ranks({0,1,2,3,4,5,6})==dredmor::ClassRanks({3,2,2}));
        const auto& tables=content.item_factory_tables();
        check(tables.named[0].size()==4 && tables.named[0]==tables.named[1] && tables.named[0]==tables.named[2] && tables.named[3].empty());
        check(tables.food[0]==std::vector<std::uint32_t>({1,2}) && tables.armour[0]==std::vector<std::uint32_t>({0,3}));
        const auto loot_context=content.loot_context({3,false,-1},2);
        check(loot_context.gold_per_level==10 && loot_context.trap_deviation==1.4f && loot_context.level_set==2 && loot_context.depth.depth==3);
        const auto loadout=content.starting_loadout({6,5,4,3,2,1,0});
        const auto static_room=content.room("Starting Room").instantiate(),dynamic_room=content.room("Starting Room").instantiate(false);
        check(!static_room.passable({3,2}) && dynamic_room.passable({3,2}));
        check(static_room.player().x==dynamic_room.player().x && static_room.player().y==dynamic_room.player().y);
        check(loadout.size()==2 && loadout[0].skill_id==0 && loadout[0].item.amount==1);
        check(loadout[1].skill_id==1 && loadout[1].item.always && loadout[1].item.subtype=="Fixture");
        auto enemy=content.make_monster("Fixture variant",{2,1},dredmor::Difficulty::medium);
        check(enemy.health==11 && enemy.damage[0]==1 && enemy.experience_reward==10);
        check(content.tweaks().integer(dredmor::Difficulty::medium,"monster additional HP per level")==0);
        dredmor::DarwinRandom creation_random(17);const auto created_enemy=content.create_monster("Fixture variant",{2,1},dredmor::Difficulty::medium,0,creation_random);
        check(created_enemy.primary==enemy.primary && creation_random.state()==17);
        // Rejected records bypass construction; failures roll back its RNG and IDs.
        std::istringstream monster_map("#####\n#@..#\n#...#\n#####\n");
        dredmor::Session room_monsters(dredmor::World::read(monster_map),dredmor::make_player({3,2,2}),1);
        auto save=[](const dredmor::Session& value){std::ostringstream out;value.save(out);return out.str();};
        dredmor::RoomMonsterRecord monster_record;monster_record.name="Fixture variant";monster_record.placement.relative={3,2};
        monster_record.placement.percent=-1;int calls=0;
        auto constructor=[&](const auto& plan,auto& random,auto&) {++calls;check(plan.request==dredmor::PopulationRequest::named_monster);
            return content.create_monster(plan.name,plan.position,dredmor::Difficulty::medium,0,random.libc);};
        check(!room_monsters.create_room_monster(monster_record,constructor) && calls==0);
        monster_record.placement.percent=100;monster_record.placement.id="room-monster";monster_record.placement.can_push=255;
        const auto room_enemy=room_monsters.create_room_monster(monster_record,constructor);
        check(room_enemy && calls==1 && room_monsters.enemies()[0].room_spawned && room_monsters.enemies()[0].room_local_id=="room-monster");
        check(room_monsters.enemies()[0].can_push==255);
        const auto occupied_save=save(room_monsters);bool failed=false;
        try {room_monsters.create_room_monster(monster_record,[&](const auto& plan,auto& random,auto& state) {random.libc.next();return constructor(plan,random,state);});}
        catch(const std::invalid_argument&) {failed=true;}
        check(failed && save(room_monsters)==occupied_save);
        std::istringstream monster_input(occupied_save);auto loaded_monsters=dredmor::Session::load(monster_input);check(save(loaded_monsters)==occupied_save);
        room_monsters.step(dredmor::Action::wait);loaded_monsters.step(dredmor::Action::wait);check(save(loaded_monsters)==save(room_monsters));
        auto bad_monster=occupied_save;const auto push=bad_monster.find("push=\"255\"");check(push!=std::string::npos);
        bad_monster.replace(push,10,"push=\"256\"");failed=false;
        try {std::istringstream invalid(bad_monster);dredmor::Session::load(invalid);} catch(const std::runtime_error&) {failed=true;}
        check(failed);
        dredmor::DarwinRandom rank_random(1);check(dredmor::monster_creation_ranks({-2,0,4},0,rank_random)==dredmor::ClassRanks({-2,0,4}));
        check(dredmor::monster_creation_ranks({-2,0,4},-1,rank_random)==dredmor::ClassRanks({0,0,4}));
        std::istringstream random_map("#########\n#@......#\n#.......#\n#.......#\n#########\n");
        dredmor::Session random_monsters(dredmor::World::read(random_map),dredmor::make_player({3,2,2}),777);
        dredmor::RoomMonsterRecord random_record;random_record.placement.relative={2,1};random_record.placement.id="random-record";
        auto random_constructor=[&](const auto& plan,auto& random,auto& state) {
            dredmor::MonsterSpawnContext context;context.depth.depth=-1;
            return content.create_room_monster(plan,context,dredmor::Difficulty::medium,random,state);
        };
        check(random_monsters.create_room_monster(random_record,random_constructor));
        check(random_monsters.monster_spawn_state().counts.size()==1 && !(random_monsters.enemies()[0].monster_flags&~10U));
        const auto random_before=save(random_monsters);failed=false;
        try {random_monsters.create_room_monster(random_record,random_constructor);} catch(const std::invalid_argument&) {failed=true;}
        check(failed && save(random_monsters)==random_before);
        std::istringstream random_input(random_before);auto random_resumed=dredmor::Session::load(random_input);
        for (int i=0;i<8;++i) {
            random_record.placement.relative={3+i%4,1+i/4};random_record.placement.percent=50;
            random_monsters.create_room_monster(random_record,random_constructor);random_resumed.create_room_monster(random_record,random_constructor);
            check(save(random_monsters)==save(random_resumed));
        }
        auto bad_counts=random_before;const auto counts_at=bad_counts.find("</monster-counts>");check(counts_at!=std::string::npos);
        bad_counts.insert(counts_at,"<entry name='Fixture monster' count='0'/>");failed=false;
        try {std::istringstream invalid(bad_counts);dredmor::Session::load(invalid);} catch(const std::runtime_error&) {failed=true;}
        check(failed);
        bool rejected=false;try {content.starting_ranks({0,0,1,2,3,4,5});} catch (const std::invalid_argument&) {rejected=true;}
        check(rejected);
        dredmor::RoomDefinition geometry;
        geometry.name="Geometry fixture";geometry.source=dredmor::XmlNode{};
        geometry.source=dredmor::parse_xml("<room width='6' height='4'><flags shop='1' zoo='1' minLevel='2' maxLevel='8' roomset='1' special='1'/><flags NoMonsters='1'/></room>");
        geometry.rows={"!P@^Dd","WL GIX","Ssi123",".     ignored outside declared width"};
        auto numeric=geometry.numeric_template();
        check(numeric.width==6 && numeric.height==4 && numeric.shop && numeric.zoo);
        check(numeric.rules.minimum_level==2 && numeric.rules.maximum_level==8 && numeric.rules.room_set==1 && numeric.rules.unique && numeric.rules.no_monsters);
        check(numeric.cells[0].kind==2 && numeric.cells[0].wall_marker==4);
        check(numeric.cells[1].wall_marker==1 && numeric.cells[2].wall_marker==2 && numeric.cells[3].wall_marker==3);
        check(numeric.cells[4].kind==6 && numeric.cells[4].door_orientation==0 && numeric.cells[5].door_orientation==1);
        check(numeric.cells[6].kind==7 && numeric.cells[7].kind==8 && numeric.cells[8].kind==0);
        check(numeric.cells[9].kind==9 && numeric.cells[10].kind==10 && numeric.cells[11].kind==15);
        check(numeric.cells[12].kind==1 && numeric.cells[13].kind==1 && numeric.cells[14].kind==1 && numeric.cells[15].kind==0);
        geometry.source.children.clear();geometry.source.attributes["shop"]="1";
        numeric=geometry.numeric_template();check(!numeric.shop && numeric.rules.maximum_level==25);
        std::cout<<"content-to-session definitions, selected classes, variant stats and room collision passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
