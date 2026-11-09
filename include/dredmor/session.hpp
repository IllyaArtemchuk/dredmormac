#pragma once
#include "dredmor/equipment.hpp"
#include "dredmor/progression.hpp"
#include "dredmor/random.hpp"
#include "dredmor/world.hpp"
#include "dredmor/room_records.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/currency.hpp"
#include "dredmor/factory.hpp"
#include "dredmor/monsters.hpp"
#include <memory>

namespace dredmor {
struct Actor {
    std::uint64_t id=0;
    std::string name,sprite;
    Position position{};
    PrimaryStats primary{};
    SecondaryStats secondary{};
    Damage damage{},resistance{};
    std::int32_t health=0,mana=0,experience_reward=0,zorkmids=0;
    std::string room_local_id;
    std::uint8_t can_push=1;
    bool room_spawned=false;
    std::uint32_t monster_flags=0;
    bool alive() const { return health>0; }
};
struct GroundItem { InventoryItem item;Position position; };
struct RoomLootCreation {bool accepted=false;std::optional<std::uint64_t> instance;};
enum class EventKind { moved,blocked,attack,dodge,counter,death,level_up,pickup,inventory_full,currency_given,currency_taken,object_hit,object_broken };
struct TurnEvent {
    EventKind kind;
    std::uint64_t actor=0,target=0;
    std::int32_t amount=0;
    bool critical=false,blocked=false;
};
// Integration session around measured primitives. Turn scheduling and enemy
// pathfinding are provisional until compared with full original gameplay.
class Session {
public:
    Session(World world,Actor player,std::uint32_t seed,int experience_n=190);
    const World& world() const { return world_; }
    const Actor& player() const { return player_; }
    const std::vector<Actor>& enemies() const { return enemies_; }
    const std::vector<GroundItem>& ground_items() const { return ground_; }
    const std::vector<CustomRoomObject>& custom_objects() const { return custom_objects_; }
    const std::vector<BreakableRoomObject>& breakables() const { return breakables_; }
    bool passable(Position position) const;
    std::optional<std::size_t> create_custom_object(const CustomRoomDefinition& definition,Position origin={0,0});
    std::optional<std::size_t> create_pedestal(const RoomPedestalRecord& definition,Position origin={0,0});
    std::optional<std::size_t> create_breakable(const BreakableRoomDefinition& definition,Position origin={0,0},BreakableFrames frames={});
    std::optional<std::size_t> create_breakable(const BreakableRoomDefinition& definition,Position origin,
        const std::function<BreakableFrames(const BreakableRoomDefinition&)>& resources);
    bool strike_breakable(std::size_t index);
    TrapSelection select_trap_candidate(const std::vector<TrapCandidate>& pool,SpawnDepthContext depth,float deviation);
    const std::vector<TurnEvent>& events() const { return events_; }
    const Progression& progression() const { return progression_; }
    const Equipment& equipment() const { return equipment_; }
    Inventory& inventory() { return inventory_; }
    const Inventory& inventory() const { return inventory_; }
    std::uint64_t spawn_enemy(Actor enemy);
    void drop(InventoryItem item,Position position);
    std::uint64_t create_ground_item(std::uint32_t definition,Position position,std::int32_t quantity=1);
    std::uint64_t create_ground_currency(std::int32_t amount,Position position);
    std::uint64_t create_ground_artifact(std::uint32_t definition,int quality,const ArtifactWords& words,Position position);
    std::optional<std::uint64_t> create_ground_loot(const ItemFactoryTables& tables,const ItemFactoryContext& context,
        std::string_view type,std::string_view subtype,int quantity,bool artifact,Position position);
    RoomLootCreation create_room_loot(const RoomLootRecord& record,const ItemFactoryTables& tables,
        const ItemFactoryContext& context,Position origin={0,0});
    std::optional<std::uint64_t> create_room_monster(const RoomMonsterRecord& record,
        const std::function<Actor(const RoomPopulationPlan&,GameRandom&,MonsterSpawnState&)>& constructor,Position origin={0,0});
    const MonsterSpawnState& monster_spawn_state() const {return monster_state_;}
    void give_currency(std::int32_t amount);
    void take_currency(std::int32_t amount);
    void step(Action action);
    void pickup();
    void equipment_profile(ClassRanks ranks,std::shared_ptr<const std::vector<ItemDefinition>> definitions);
    // One-time new-character initialization, before turns or content spawning.
    void character_profile(ClassRanks ranks,std::shared_ptr<const std::vector<ItemDefinition>> definitions,
                           std::vector<SkillRank> skills,std::shared_ptr<const std::vector<AbilityDefinition>> abilities);
    const std::vector<SkillRank>& skills() const { return skills_; }
    bool equip(std::uint64_t instance,EquipmentSlot slot);
    bool unequip(EquipmentSlot slot);
    std::string snapshot() const;
    void save(std::ostream& output) const;
    static Session load(std::istream& input,std::shared_ptr<const std::vector<ItemDefinition>> definitions={},
                        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={});
private:
    MonsterSpawnState monster_state_;
    friend class CharacterBuilder;
    World world_;
    Actor player_;
    std::vector<Actor> enemies_;
    std::vector<GroundItem> ground_;
    std::vector<CustomRoomObject> custom_objects_;
    std::vector<BreakableRoomObject> breakables_;
    std::vector<TurnEvent> events_;
    Inventory inventory_;
    Equipment equipment_;
    ClassRanks ranks_{};
    Damage intrinsic_damage_{},intrinsic_resistance_{};
    std::shared_ptr<const std::vector<ItemDefinition>> definitions_;
    std::vector<SkillRank> skills_;
    std::shared_ptr<const std::vector<AbilityDefinition>> abilities_;
    GameRandom random_;
    ItemFactoryState factory_state_;
    Progression progression_;
    std::uint64_t next_actor_=2;
    int experience_n_;
    Actor* enemy_at(Position position,std::uint64_t excluding=0);
    void attack(Actor& attacker,Actor& defender);
    void step_impl(Action action);
    void enemy_turns();
    std::optional<Position> next_step(const Actor& enemy) const;
    void rebuild_equipment();
    std::uint64_t next_item_instance() const;
    void currency_transfer_impl(std::int32_t amount,CurrencyOperation operation);
    void pickup_impl();
    void pickup_currency();
    bool hit_breakable_impl(std::size_t index);
    std::optional<std::uint64_t> create_ground_loot_impl(const ItemFactoryTables& tables,const ItemFactoryContext& context,
        std::string_view type,std::string_view subtype,int quantity,bool artifact,Position position);
};
Actor make_player(const ClassRanks& ranks,std::string name="Player");
}
