#pragma once
#include "dredmor/equipment.hpp"
#include "dredmor/progression.hpp"
#include "dredmor/random.hpp"
#include "dredmor/world.hpp"
#include "dredmor/room_records.hpp"
#include <memory>

namespace dredmor {
struct Actor {
    std::uint64_t id=0;
    std::string name,sprite;
    Position position{};
    PrimaryStats primary{};
    SecondaryStats secondary{};
    Damage damage{},resistance{};
    std::int32_t health=0,mana=0,experience_reward=0;
    bool alive() const { return health>0; }
};
struct GroundItem { InventoryItem item;Position position; };
enum class EventKind { moved,blocked,attack,dodge,counter,death,level_up,pickup,inventory_full };
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
    bool passable(Position position) const;
    std::optional<std::size_t> create_custom_object(const CustomRoomDefinition& definition,Position origin={0,0});
    const std::vector<TurnEvent>& events() const { return events_; }
    const Progression& progression() const { return progression_; }
    const Equipment& equipment() const { return equipment_; }
    Inventory& inventory() { return inventory_; }
    const Inventory& inventory() const { return inventory_; }
    std::uint64_t spawn_enemy(Actor enemy);
    void drop(InventoryItem item,Position position);
    std::uint64_t create_ground_item(std::uint32_t definition,Position position,std::int32_t quantity=1);
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
    friend class CharacterBuilder;
    World world_;
    Actor player_;
    std::vector<Actor> enemies_;
    std::vector<GroundItem> ground_;
    std::vector<CustomRoomObject> custom_objects_;
    std::vector<TurnEvent> events_;
    Inventory inventory_;
    Equipment equipment_;
    ClassRanks ranks_{};
    Damage intrinsic_damage_{},intrinsic_resistance_{};
    std::shared_ptr<const std::vector<ItemDefinition>> definitions_;
    std::vector<SkillRank> skills_;
    std::shared_ptr<const std::vector<AbilityDefinition>> abilities_;
    GameRandom random_;
    Progression progression_;
    std::uint64_t next_actor_=2;
    int experience_n_;
    Actor* enemy_at(Position position,std::uint64_t excluding=0);
    void attack(Actor& attacker,Actor& defender);
    void step_impl(Action action);
    void enemy_turns();
    std::optional<Position> next_step(const Actor& enemy) const;
    void rebuild_equipment();
};
Actor make_player(const ClassRanks& ranks,std::string name="Player");
}
