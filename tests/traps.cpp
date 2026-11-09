#include "dredmor/traps.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("trap assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
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
        std::cout<<"trap charge/missing-spell policy, sticky discovery, disarm XP/messages and origin rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
