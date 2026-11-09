#pragma once
#include "dredmor/items.hpp"
#include "dredmor/random.hpp"
#include "dredmor/world.hpp"
#include "dredmor/factory.hpp"
#include "dredmor/room_records.hpp"

namespace dredmor {
struct TrapInstance {
    std::uint64_t instance=0;
    std::uint32_t definition=0;
    Position position{},origin{};
    std::string room_local_id;
    std::uint8_t can_push=1;
    std::int32_t charges=0;
    bool discovered=false,spent=false,disarmed=false,pending_destruction=false;
    bool has_origin_object=false,sprite_revealed=false,disabled=false,player_placed=false;
};
struct TrapMap {
    int width=0,height=0;
    std::vector<std::uint8_t> tiles;
    std::uint8_t tile(Position position) const;
};
TrapInstance make_trap(std::uint64_t instance,std::uint32_t index,const ItemDefinition& definition);
struct TrapConstruction {bool accepted=false;std::optional<TrapInstance> object;};
TrapConstruction create_room_trap(const RoomTrapRecord& record,const std::vector<ItemDefinition>& definitions,
    const ItemFactoryTables& tables,const ItemFactoryContext& context,const TrapMap& map,
    std::uint64_t instance,GameRandom& random,Position origin={0,0});
// Produces the original origin-blocker request. Resource loading and actual
// square/list attachment belong to the owner. Invalid original reads fail here.
void place_trap(TrapInstance& instance,const ItemDefinition& definition,Position position,const TrapMap& map,Random& random);
bool detect_trap(TrapInstance& instance,const ItemDefinition& definition,Position player,const SecondaryStats& secondary,bool wizard_level=false);
struct TrapStep {bool clicked=false,triggered=false;int roll=0;};
// Monster tile/level/terrain eligibility is an explicit input; it is resolved
// by the owning level before this measured avoidance decision.
TrapStep step_on_trap(TrapInstance& instance,const SecondaryStats& secondary,Random& random,bool monster=false,bool eligible=true,bool wizard_level=false);
struct TrapSpellRequest {std::string spell;Position origin{},target{};bool caster_is_target=true,monster_cast=false;std::string death_description;};
std::optional<TrapSpellRequest> trigger_trap(TrapInstance& instance,const ItemDefinition& definition,Position target,bool spell_exists,bool monster=false);
struct TrapDisarm {bool first=false,pickup=false,destroy_message=false;std::int32_t experience=0;};
TrapDisarm disarm_trap(TrapInstance& instance,const ItemDefinition& definition,int affinity,int experience_scale,Random& random);
}
