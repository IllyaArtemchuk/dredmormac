#include "dredmor/traps.hpp"
#include <iostream>
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
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
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
