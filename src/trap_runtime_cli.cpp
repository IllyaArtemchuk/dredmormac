#include "dredmor/traps.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

void state(const dredmor::TrapInstance& trap) {
    std::cout<<trap.position.x<<' '<<trap.position.y<<' '<<trap.origin.x<<' '<<trap.origin.y<<' '<<trap.has_origin_object<<' '
        <<trap.charges<<' '<<trap.discovered<<' '<<trap.spent<<' '<<trap.disarmed<<' '<<trap.pending_destruction<<' '<<trap.sprite_revealed<<' ';
}
void request_output(const std::optional<dredmor::TrapSpellRequest>& request) {
    std::cout<<request.has_value()<<' ';
    if (!request) {std::cout<<"0 0 0 0 0 0 - ";return;}
    std::cout<<request->origin.x<<' '<<request->origin.y<<' '<<request->target.x<<' '<<request->target.y<<' '<<request->caster_is_target<<' '<<request->monster_cast<<' ';
    if (request->death_description.empty()) std::cout<<'-';
    else {const char* digits="0123456789abcdef";for (unsigned char c:request->death_description) std::cout<<digits[c>>4]<<digits[c&15];}
    std::cout<<' ';
}
int main(int argc,char** argv) {
    try {
        const bool room_mode=argc==2 && std::string(argv[1])=="--room-record";
        if (argc!=1 && !room_mode) throw std::invalid_argument("expected --room-record or no arguments");
        std::uint32_t seed;
        while (std::cin>>seed) {
            if (room_mode) {
                dredmor::ItemFactoryContext context;dredmor::RoomTrapRecord record;dredmor::Position origin;unsigned push;std::size_t count;
                if (!(std::cin>>context.depth.depth>>context.depth.alternate>>context.depth.alternate_depth>>context.trap_deviation>>record.placement.percent>>context.level_set
                    >>origin.x>>origin.y>>record.placement.relative.x>>record.placement.relative.y>>push>>std::quoted(record.placement.id)>>std::quoted(record.name)>>count) || count>10000 || push>255)
                    throw std::invalid_argument("invalid room trap fixture");
                record.placement.can_push=push;std::vector<dredmor::ItemDefinition> definitions(count);dredmor::ItemFactoryTables tables;
                for (std::size_t i=0;i<count;++i) {
                    auto& def=definitions[i];std::uint32_t mask;unsigned mount,facing;def.kind=dredmor::ItemKind::trap;def.trap=dredmor::TrapDefinition{};
                    if (!(std::cin>>def.level>>def.special>>def.debug>>mask>>std::quoted(def.name)>>def.trap->charges>>mount>>facing) || mount>2 || facing>3) throw std::invalid_argument("invalid room trap candidate");
                    def.trap->mount=static_cast<dredmor::TrapMount>(mount);def.trap->facing=static_cast<dredmor::TrapFacing>(facing);
                    dredmor::register_factory_item(tables,i,def,mask);
                }
                dredmor::TrapMap map{9,9,std::vector<std::uint8_t>(81,1)};
                for (int i=0;i<9;++i) map.tiles[i]=map.tiles[72+i]=map.tiles[i*9]=map.tiles[i*9+8]=0;
                dredmor::GameRandom random;random.seed(seed);const auto result=dredmor::create_room_trap(record,definitions,tables,context,map,1,random,origin);
                std::cout<<result.accepted<<' ';
                if (result.object) {
                    const auto& trap=*result.object;std::cout<<trap.definition<<' ';state(trap);
                    if (trap.room_local_id.empty()) std::cout<<'-';
                    else {const char* digits="0123456789abcdef";for (unsigned char c:trap.room_local_id) std::cout<<digits[c>>4]<<digits[c&15];}
                    std::cout<<' '<<unsigned(trap.can_push)<<' ';
                } else std::cout<<"-1 0 0 0 0 0 0 0 0 0 0 0 - 0 ";
                const auto cache=random.normal.state();std::uint32_t spare;std::memcpy(&spare,&cache.spare,4);
                std::cout<<random.libc.state()<<' '<<cache.available<<' '<<spare<<' ';for (int i=0;i<8;++i) std::cout<<random.mt.u32()<<' ';std::cout<<'\n';continue;
            }
            unsigned trigger,mount,facing;int charges,level,experience_scale;dredmor::SecondaryStats secondary{};
            bool wizard,disabled,placed,spell_exists,target_is_caster;dredmor::Position player,position;dredmor::TrapMap map;
            if (!(std::cin>>trigger>>mount>>facing>>charges>>level>>experience_scale>>secondary[16]>>secondary[17]>>secondary[12]
                >>wizard>>disabled>>placed>>spell_exists>>target_is_caster>>player.x>>player.y>>position.x>>position.y>>map.width>>map.height) ||
                trigger>2 || mount>2 || facing>3 || map.width<1 || map.height<1 || map.width>100 || map.height>100)
                throw std::invalid_argument("invalid trap runtime fixture");
            map.tiles.resize(std::size_t(map.width)*map.height);
            for (auto& cell:map.tiles) {unsigned value;if (!(std::cin>>value) || value>255) throw std::invalid_argument("invalid trap tile");cell=value;}
            dredmor::ItemDefinition definition;definition.kind=dredmor::ItemKind::trap;definition.level=level;definition.name="Fixture Trap";
            definition.trap=dredmor::TrapDefinition{};auto& spec=*definition.trap;
            spec.trigger=static_cast<dredmor::TrapTrigger>(trigger);spec.mount=static_cast<dredmor::TrapMount>(mount);spec.facing=static_cast<dredmor::TrapFacing>(facing);
            spec.charges=charges;spec.spell="Fixture Spell";spec.origin="Fixture Origin.png";
            spec.target_is_caster=target_is_caster;
            auto trap=dredmor::make_trap(1,0,definition);dredmor::Random random(seed);
            dredmor::place_trap(trap,definition,position,map,random);trap.disabled=disabled;trap.player_placed=placed;
            state(trap);std::cout<<dredmor::detect_trap(trap,definition,player,secondary,wizard)<<' ';state(trap);
            auto step=dredmor::step_on_trap(trap,secondary,random,false,true,wizard);std::cout<<step.clicked<<' '<<step.triggered<<' ';state(trap);
            step=dredmor::step_on_trap(trap,secondary,random,true,true,wizard);
            auto request=step.triggered?dredmor::trigger_trap(trap,definition,player,spell_exists,true):std::nullopt;
            std::cout<<step.clicked<<' '<<step.triggered<<' ';request_output(request);state(trap);
            request=dredmor::trigger_trap(trap,definition,player,spell_exists,false);request_output(request);state(trap);
            for (int i=0;i<2;++i) {
                const auto disarm=dredmor::disarm_trap(trap,definition,secondary[16],experience_scale,random);
                std::cout<<disarm.first<<' '<<disarm.pickup<<' '<<disarm.destroy_message<<' '<<disarm.experience<<' ';state(trap);
            }
            for (int i=0;i<8;++i) std::cout<<random.u32()<<' ';std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
