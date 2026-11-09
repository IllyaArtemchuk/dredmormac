#include "dredmor/traps.hpp"
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t signed_bits(std::uint32_t bits) {std::int32_t result;std::memcpy(&result,&bits,4);return result;}
std::int32_t add(std::int32_t left,std::int32_t right) {return signed_bits(std::uint32_t(left)+std::uint32_t(right));}
const TrapDefinition& trap_definition(const ItemDefinition& definition) {
    if (definition.kind!=ItemKind::trap || !definition.trap) throw std::invalid_argument("expected trap definition");
    return *definition.trap;
}
}
std::uint8_t TrapMap::tile(Position position) const {
    if (width<1 || height<1 || tiles.size()!=std::size_t(width)*std::size_t(height) || position.x<0 || position.y<0 || position.x>=width || position.y>=height)
        throw std::invalid_argument("trap origin scan outside tile map");
    return tiles[std::size_t(position.y)*width+position.x];
}
TrapInstance make_trap(std::uint64_t instance,std::uint32_t index,const ItemDefinition& definition) {
    if (!instance) throw std::invalid_argument("trap instance needs an ID");
    TrapInstance result;result.instance=instance;result.definition=index;result.charges=trap_definition(definition).charges;return result;
}
void place_trap(TrapInstance& instance,const ItemDefinition& definition,Position position,const TrapMap& map,Random& random) {
    const auto& trap=trap_definition(definition);auto next=instance;auto candidate=random;next.position=position;
    const std::array<Position,4> directions={Position{0,-1},Position{1,0},Position{0,1},Position{-1,0}};
    const auto facing=static_cast<std::size_t>(trap.facing);if (facing>=directions.size()) throw std::invalid_argument("invalid trap origin facing");
    const auto direction=directions[facing];auto advance=[&](Position p){return Position{add(p.x,direction.x),add(p.y,direction.y)};};
    if (trap.mount==TrapMount::wall) {
        auto origin=position;
        for (;;) {
            const auto tile=map.tile(origin);
            if (tile==0 || tile==2) {next.origin=origin;next.has_origin_object=true;break;}
            if (tile==3 || tile==4) {next.pending_destruction=true;break;}
            origin=advance(origin);
        }
    } else if (trap.mount==TrapMount::floor) {
        auto origin=advance(position);
        while (map.tile(advance(origin))==1) {
            const auto roll=candidate.bounded(3);origin=advance(origin);if (!roll) break;
        }
        next.origin=origin;next.has_origin_object=true;
    }
    instance=std::move(next);random=std::move(candidate);
}
bool detect_trap(TrapInstance& instance,const ItemDefinition& definition,Position player,const SecondaryStats& secondary,bool wizard_level) {
    if (!instance.disabled && !wizard_level) {
        auto absolute=[](std::int32_t value){return value<0?0U-std::uint32_t(value):std::uint32_t(value);};
        const auto distance=signed_bits(absolute(signed_bits(std::uint32_t(player.x)-std::uint32_t(instance.position.x)))+
                                        absolute(signed_bits(std::uint32_t(player.y)-std::uint32_t(instance.position.y))));
        if (secondary[17]<distance && !instance.discovered) return false;
        instance.discovered=true;if (definition.level<secondary[16]) instance.sprite_revealed=true;
    }
    return true;
}
TrapStep step_on_trap(TrapInstance& instance,const SecondaryStats& secondary,Random& random,bool monster,bool eligible,bool wizard_level) {
    TrapStep result;if (instance.disabled || !eligible || wizard_level) return result;
    result.clicked=true;result.roll=static_cast<int>(random.bounded(100));instance.discovered=true;
    const auto total=add(add(secondary[17],secondary[12]),secondary[16]);
    result.triggered=(monster?total:total/2)<result.roll;return result;
}
std::optional<TrapSpellRequest> trigger_trap(TrapInstance& instance,const ItemDefinition& definition,Position target,bool spell_exists,bool monster) {
    const auto& trap=trap_definition(definition);if (!spell_exists && !monster) return std::nullopt;
    std::optional<TrapSpellRequest> result;
    if (spell_exists) result=TrapSpellRequest{trap.spell,trap.mount==TrapMount::ceiling?target:instance.origin,target,!monster || trap.target_is_caster,monster,monster?"":"by a "+definition.name};
    if (trap.trigger==TrapTrigger::once || (trap.trigger==TrapTrigger::charges && (instance.charges=add(instance.charges,-1))<1)) {
        instance.spent=true;instance.pending_destruction=true;
    }
    return result;
}
TrapDisarm disarm_trap(TrapInstance& instance,const ItemDefinition& definition,int affinity,int experience_scale,Random& random) {
    const auto& trap=trap_definition(definition);TrapDisarm result;
    if (!instance.disarmed) {
        result.first=true;instance.disarmed=true;
        if (!instance.player_placed) result.experience=signed_bits(std::uint32_t(experience_scale)*std::uint32_t(add(definition.level,1)));
    }
    if (definition.level<affinity && trap.mount==TrapMount::ceiling) {
        result.destroy_message=random.bounded(2)==0;result.pickup=!result.destroy_message;
    } else {instance.spent=true;instance.pending_destruction=true;}
    return result;
}
}
