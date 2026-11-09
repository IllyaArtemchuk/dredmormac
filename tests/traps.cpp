#include "dredmor/traps.hpp"
#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("trap assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto definition=read_item(parse_xml("<item name='Fixture trap' level='2'><trap trigger='charges' charges='2' originMount='ceiling' casts='Fixture spell'/></item>"));
        auto trap=make_trap(9,3,definition);check(trap.charges==2 && !trap.discovered && !trap.spent);
        const auto first=trigger_trap(trap,definition,{3,4},true,true);
        check(first && first->origin.x==3 && first->origin.y==4 && first->monster_cast && !first->caster_is_target);
        check(trap.charges==1 && !trap.pending_destruction);
        check(!trigger_trap(trap,definition,{3,4},false,false) && trap.charges==1);
        const auto player_spell=trigger_trap(trap,definition,{3,4},true,false);
        check(player_spell && player_spell->death_description=="by a Fixture trap" && player_spell->caster_is_target);
        trap.charges=1;trap.spent=trap.pending_destruction=false;
        check(!trigger_trap(trap,definition,{3,4},false,true) && trap.charges==0 && trap.pending_destruction && trap.spent);
        trap=make_trap(10,3,definition);trap.position={3,3};SecondaryStats secondary{};
        check(!detect_trap(trap,definition,{1,1},secondary) && !trap.discovered);
        secondary[17]=4;secondary[16]=3;check(detect_trap(trap,definition,{1,1},secondary) && trap.sprite_revealed);
        secondary[17]=0;check(detect_trap(trap,definition,{0,0},secondary)); // Discovery persists outside sight range.
        Random random(1);auto unchanged=random;
        check(!step_on_trap(trap,secondary,random,false,false).clicked && random.u32()==unchanged.u32());
        auto disarmed=make_trap(11,3,definition);random.seed(1);
        auto result=disarm_trap(disarmed,definition,3,5,random);
        check(result.first && result.experience==15 && result.destroy_message && !result.pickup);
        check(disarmed.disarmed && !disarmed.pending_destruction); // The original message branch does not queue destruction.
        result=disarm_trap(disarmed,definition,0,5,random);
        check(!result.first && result.experience==0 && disarmed.pending_destruction);
        disarmed=make_trap(12,3,definition);disarmed.player_placed=true;
        check(disarm_trap(disarmed,definition,0,5,random).experience==0);
        definition.trap->mount=TrapMount::floor;definition.trap->facing=TrapFacing::west;
        TrapMap map{5,5,std::vector<std::uint8_t>(25,1)};
        for (int i=0;i<5;++i) {map.tiles[i]=map.tiles[20+i]=map.tiles[i*5]=map.tiles[i*5+4]=0;}
        trap=make_trap(13,3,definition);place_trap(trap,definition,{2,2},map,random);
        check(trap.has_origin_object && trap.origin.x==3 && trap.origin.y==2);
        definition.trap->mount=TrapMount::wall;map.tiles[2*5+3]=3;
        trap=make_trap(14,3,definition);place_trap(trap,definition,{2,2},map,random);
        check(trap.pending_destruction && !trap.spent && !trap.has_origin_object);
        definition.trap->mount=TrapMount::floor;definition.trap->facing=TrapFacing::north;
        map.tiles.assign(25,1);random.seed(1);unchanged=random;const auto previous=trap;bool failed=false;
        try {place_trap(trap,definition,{2,2},map,random);} catch(const std::invalid_argument&) {failed=true;}
        check(failed && trap.origin.x==previous.origin.x && trap.position.x==previous.position.x && trap.position.y==previous.position.y && random.u32()==unchanged.u32());
        std::istringstream world("#####\n#@..#\n#...#\n#####\n");Session session(World::read(world),make_player({3,2,2}),1);
        RoomTrapRecord record;record.placement.percent=-1;ItemFactoryTables tables;ItemFactoryContext context;
        const auto initial=saved(session);check(!session.create_room_trap(record,tables,context,{}).accepted && saved(session)!=initial);
        std::vector<ItemDefinition> items{
            read_item(parse_xml("<item name='Same' level='2'><trap charges='2'/></item>")),
            read_item(parse_xml("<item name='Same' level='1'><trap charges='7' originMount='floor' originFacing='west'/></item>")),
            read_item(parse_xml("<item name='Fixture wand'><wand/></item>"))};
        auto definitions=std::make_shared<const std::vector<ItemDefinition>>(items);session.equipment_profile({3,2,2},definitions);
        tables.traps[0]={1};tables.traps[1]={0};record.placement.percent=100;record.name="Missing";
        const auto missing=saved(session);check(session.create_room_trap(record,tables,context,{}).accepted && saved(session)==missing);
        record.name="Same";record.placement.relative={2,1};record.placement.id="trap<&id";record.placement.can_push=0;
        map={5,4,{0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,0}};
        const auto created=session.create_room_trap(record,tables,context,map);
        check(created.instance && session.traps()[0].definition==1 && session.traps()[0].charges==7 && session.traps()[0].origin.x==3);
        check(session.traps()[0].room_local_id==record.placement.id && session.traps()[0].can_push==0);
        check(!session.passable({3,1}) && session.world().passable({3,1}));
        const auto wand=session.create_ground_item(2,{2,1});check(wand>*created.instance);
        DarwinRandom item_random(1);auto duplicate=make_inventory_item(items[0],0,*created.instance,item_random);failed=false;
        try {session.drop(duplicate,{2,1});} catch(const std::invalid_argument&) {failed=true;}
        check(failed);
        const auto owned=saved(session);std::istringstream input(owned);auto resumed=Session::load(input,definitions);check(saved(resumed)==owned);
        record.name="";record.placement.percent=50;
        for (int i=0;i<8;++i) {session.create_room_trap(record,tables,context,map);resumed.create_room_trap(record,tables,context,map);check(saved(session)==saved(resumed));}
        record.placement.percent=100;const auto before=saved(session);failed=false;auto invalid_map=map;invalid_map.width=6;invalid_map.tiles.assign(24,1);
        try {session.create_room_trap(record,tables,context,invalid_map);} catch(const std::invalid_argument&) {failed=true;}
        check(failed && saved(session)==before);
        auto bad=owned;const auto id=bad.find("<trap instance=\"");check(id!=std::string::npos);const auto value=id+std::string("<trap instance=\"").size();
        bad.replace(value,bad.find('"',value)-value,std::to_string(wand));failed=false;
        try {std::istringstream invalid(bad);Session::load(invalid,definitions);} catch(const std::runtime_error&) {failed=true;}
        check(failed);
        std::cout<<"trap runtime/requests, origin rollback, room gates/lookup/IDs, persistence and resumed creation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
