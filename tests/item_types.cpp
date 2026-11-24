#include "dredmor/item_types.hpp"
#include "dredmor/session_room_scripts.hpp"
#include "dredmor/content.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("item type assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& value) {std::ostringstream out;value.save(out);return out.str();}
Session make_session(std::shared_ptr<const std::vector<ItemDefinition>> definitions) {
    std::istringstream rows("#######\n#@....#\n#.....#\n#.....#\n#######\n");Session session(World::read(rows),make_player({3,2,2}),7);
    session.equipment_profile({3,2,2},std::move(definitions));return session;
}
int main() {
    try {
        std::vector<ItemDefinition> values={read_item(parse_xml("<item name='Cap'><armour type='head'/></item>")),
            read_item(parse_xml("<item name='Potion A'><potion/></item>")),read_item(parse_xml("<item name='Potion B'><potion/></item>")),
            read_item(parse_xml("<item name='Wand'><wand mincharge='2' maxcharge='6'/></item>")),
            read_item(parse_xml("<item name='Food'><food hp='1'/></item>")),read_item(parse_xml("<item name='Trap'><trap originmount='floor'/></item>"))};
        for (int subtype=0;subtype<2;++subtype) {ItemDefinition misc;misc.kind=ItemKind::misc;misc.name=subtype?"Spores":"Lockpick";misc.builtin_subtype=subtype;values.push_back(misc);}
        ItemDefinition money;money.kind=ItemKind::currency;money.name="Zorkmids";money.builtin_subtype=2;values.push_back(money);
        const auto definitions=std::make_shared<const std::vector<ItemDefinition>>(values);
        ItemFactoryTables tables;for (std::size_t i=0;i<values.size();++i) register_factory_item(tables,std::uint32_t(i),values[i],7);
        auto type=[&](std::uint32_t index,const ItemFactoryTables& registrations) {InventoryItem item;item.definition=index;item.kind=values[index].kind;return script_item_type(item,values[index],registrations,100+index);};
        check(type(0,tables).kind==2 && type(0,tables).subtype==100);
        check(type(1,tables).subtype==0 && type(2,tables).subtype==1 && type(3,tables).subtype==0);
        check(type(6,tables).kind==6 && type(6,tables).subtype==0 && type(7,tables).subtype==1);
        check(type(8,tables).kind==10 && type(8,tables).subtype==0);
        auto alias=tables;alias.potions={};alias.potions[0]={2};alias.potions[1]={1,2};
        check(type(1,alias).subtype==0 && type(2,alias).subtype==0); // first pool ordinal can alias across sets
        alias.wands={};check(type(3,alias).subtype==0); // inherited base value when absent
        ItemFactoryContext context;auto session=make_session(definitions);GameRandom expected;expected.seed(7);
        const auto expected_prototype=make_loot_item(values,tables,context,"wand","random",1,false,1,expected);check(expected_prototype);
        const auto prototype=session.create_room_item_prototype(tables,context,"wand","random");check(prototype && prototype->kind==4 && prototype->subtype==0);
        check(session.ground_items().empty() && parse_xml(saved(session)).first("random")->attribute("libc")==std::to_string(expected.libc.state()));
        check(session.create_ground_item(3,{3,2})==1); // prototype never became a placed inventory instance
        auto tile=session.room_tile_items(tables,{3,2});check(tile.size()==1 && tile[0].item && tile[0].type.kind==4 && tile[0].type.subtype==0);
        RoomTrapRecord trap;trap.name="Trap";trap.placement.relative={4,2};TrapMap map{7,5,std::vector<std::uint8_t>(35,1)};
        check(session.create_room_trap(trap,tables,context,map).instance);tile=session.room_tile_items(tables,{4,2});
        check(tile.size()==1 && tile[0].type.kind==12 && tile[0].type.subtype==5);
        check(session.room_tile_items(tables,{1,1}).empty());
        // Full runtime creates a prototype on an empty target, then shares the
        // cached native comparison type without more factory/RNG use.
        RoomDefinition room;room.rows={"#######","#.....#","#.....#","#.....#","#######"};
        room.source=parse_xml(R"(<room width="7" height="5"><script repeat="-1">
            <condition condition_type="type_at" type="wand" subtype="random" x="3" y="2"/>
            <action on="success" action_type="ticker" text="found"/></script></room>)");
        auto program=std::make_shared<const RoomScriptProgram>(room.script_program());SessionRoomScriptProfile profile(DungeonLayout(7,5,std::vector<DungeonCell>(35)));
        profile.programs={program};profile.sources={room.source};profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(program,&program->scripts)},{0,0},1}};
        auto puzzle=make_session(definitions);SessionRoomObjects objects;objects.bind(puzzle,{SessionRoomObjectKind::player,puzzle.player().id},profile.layout);RoomRuntimeState runtime;
        SessionRoomScriptHooks hooks;unsigned creates=0;
        hooks.create_prototype=[&](auto& owner,auto kind,auto subtype) {++creates;return owner.create_room_item_prototype(tables,context,kind,subtype);};
        hooks.tile_items=[&](const auto& owner,auto point) {return owner.room_tile_items(tables,point);};
        check(SessionRoomScriptExecutor::run(puzzle,profile,objects,runtime,hooks).presentation.empty());
        check(creates==1 && runtime.prototypes.size()==1 && puzzle.ground_items().empty());
        puzzle.create_ground_item(3,{3,2});const auto before_cached=saved(puzzle);
        check(SessionRoomScriptExecutor::run(puzzle,profile,objects,runtime,hooks).presentation.size()==1 && creates==1 && saved(puzzle)==before_cached);
        std::ostringstream encoded;SessionRoomScriptExecutor::save(encoded,puzzle,profile,objects,runtime);
        std::istringstream input(encoded.str());auto restored=SessionRoomScriptExecutor::load(input,profile,definitions);
        check(SessionRoomScriptExecutor::run(restored.session,profile,restored.objects,restored.runtime,hooks).presentation.size()==1 && creates==1);
        auto failed=make_session(definitions);failed.create_ground_item(3,{3,2});SessionRoomObjects failed_objects;
        failed_objects.bind(failed,{SessionRoomObjectKind::player,failed.player().id},profile.layout);RoomRuntimeState failed_runtime;
        auto invalid=std::make_shared<RoomScriptProgram>(*program);RoomAction move;move.kind=RoomActionKind::move;move.object_id="player";move.point={99,99};invalid->scripts[0].success.push_back(move);
        auto invalid_profile=profile;invalid_profile.programs[0]=invalid;invalid_profile.rooms[0].trigger.scripts=std::shared_ptr<const std::vector<RoomScriptDefinition>>(invalid,&invalid->scripts);
        const auto unchanged=saved(failed);bool rejected=false;
        try {SessionRoomScriptExecutor::run(failed,invalid_profile,failed_objects,failed_runtime,hooks);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && saved(failed)==unchanged && failed_runtime.prototypes.empty());
        std::cout<<"native item comparison types, prototype RNG/caching, live ground/trap queries and saved/failed script continuation agree\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
