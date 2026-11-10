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
#include "dredmor/traps.hpp"
#include "dredmor/spells.hpp"
#include "dredmor/regeneration.hpp"
#include <memory>
#include <map>

namespace dredmor {
struct ScheduledSpell {
    std::int32_t remaining=0,percent=100;
    bool repeat=false;
    std::size_t spell=0;
    std::uint64_t caster=0;
    std::string parent;
    bool requires_buff=false,forbids_buff=false;
    std::string required_buff,forbidden_buff;
};
struct ActiveBuff {
    std::size_t spell=0,buff=0;
    std::int32_t remaining=0;
    std::int32_t upkeep_remaining=0;
    bool deapplied=false;
    bool pending=false;
};
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
    int monster_terrain=0;
    std::vector<ScheduledSpell> triggers;
    std::optional<MonsterStatInput> stat_profile;
    std::vector<ActiveBuff> buffs;
    bool corpse=false;
    bool alive() const { return health>0 && !corpse; }
};
struct GroundItem { InventoryItem item;Position position; };
struct RoomLootCreation {bool accepted=false;std::optional<std::uint64_t> instance;};
struct RoomTrapCreation {bool accepted=false;std::optional<std::uint64_t> instance;};
enum class SpellUse { cast,unknown,cooling_down,equipment_required,invalid_target,insufficient_gold,insufficient_mana,muted,dead };
enum class EventKind { moved,blocked,attack,dodge,counter,death,level_up,pickup,inventory_full,currency_given,currency_taken,object_hit,object_broken,spell_damage,trap_click,trap_triggered,trap_spell_missing,spell_heal,spell_mana };
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
    const std::vector<TrapInstance>& traps() const {return traps_;}
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
    SpawnDepthContext depth_context() const {return depth_context_;}
    const std::optional<RegenerationState>& regeneration() const {return regeneration_;}
    void regeneration_profile(std::int32_t health_turns,std::int32_t mana_turns,bool vampire=false);
    void depth_context(SpawnDepthContext value) {depth_context_=value;}
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
    RoomTrapCreation create_room_trap(const RoomTrapRecord& record,const ItemFactoryTables& tables,
        const ItemFactoryContext& context,const TrapMap& map,Position origin={0,0});
    // Low-level cast: no mana/cooldown/turn scheduling. Unsupported dispatch or
    // effects fail atomically while the remaining spell engine is implemented.
    bool cast_spell(const SpellDefinition& spell,Position origin,Position target,std::uint64_t caster=0,bool allow_haywire=true);
    std::vector<std::size_t> learned_spells() const;
    const SpellDefinition& spell_definition(std::size_t index) const;
    std::int32_t spell_cooldown(std::size_t index) const;
    // Player action, distinct from internal triggered casts. The room scheduler
    // and visibility remain provisional; unsupported effects throw atomically.
    SpellUse use_spell(std::size_t index,Position target);
    bool drink_potion(std::uint64_t instance);
    bool eat_food(std::uint64_t instance);
    // One measured Hittable::UpdateTriggers pass; does not advance the world.
    void update_triggers(std::uint64_t actor);
    const std::vector<ActiveBuff>& player_buffs() const {return player_buffs_;}
    void update_player_buffs();
    void update_buffs(std::uint64_t actor);
    int player_facing() const {return player_facing_;}
    void player_facing(int direction);
    void spell_profile(std::shared_ptr<const std::vector<SpellDefinition>> definitions);
    std::vector<std::size_t> detect_traps();
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
                        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={},
                        std::shared_ptr<const std::vector<SpellDefinition>> spells={});
private:
    int player_facing_=2;
    std::optional<RegenerationState> regeneration_;
    std::map<std::size_t,std::int32_t> spell_cooldowns_;
    SpawnDepthContext depth_context_{};
    std::vector<ActiveBuff> player_buffs_;
    PrimaryStats buff_primary_flat_{};
    SecondaryStats buff_secondary_flat_{};
    MonsterSpawnState monster_state_;
    friend class CharacterBuilder;
    World world_;
    Actor player_;
    std::vector<Actor> enemies_;
    std::vector<GroundItem> ground_;
    std::vector<CustomRoomObject> custom_objects_;
    std::vector<BreakableRoomObject> breakables_;
    std::vector<TrapInstance> traps_;
    std::vector<TurnEvent> events_;
    Inventory inventory_;
    Equipment equipment_;
    ClassRanks ranks_{};
    Damage intrinsic_damage_{},intrinsic_resistance_{};
    std::shared_ptr<const std::vector<ItemDefinition>> definitions_;
    std::vector<SkillRank> skills_;
    std::shared_ptr<const std::vector<AbilityDefinition>> abilities_;
    std::shared_ptr<const std::vector<SpellDefinition>> spells_;
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
    bool cast_spell_impl(const SpellDefinition& spell,Position origin,Position target,std::uint64_t caster,bool allow_haywire);
    bool cast_spell_recursive(const SpellDefinition& spell,Position origin,Position target,std::uint64_t caster,bool allow_haywire,
        std::size_t depth,std::size_t& remaining);
    void apply_spell_effects(const SpellDefinition& spell,Position target,std::uint64_t caster,bool allow_haywire,std::size_t depth,std::size_t& remaining);
    void activate_traps_impl(Actor& actor);
    Actor& actor_by_id(std::uint64_t id);
    void update_triggers_impl(Actor& actor,std::size_t& remaining);
    void apply_spell_buffs(const SpellDefinition& spell,Position target,std::uint64_t caster,bool self);
    void apply_player_buff(std::size_t spell,std::size_t buff);
    void update_player_buffs_impl();
    void deapply_player_buff(std::size_t index);
    void update_drunkenness_impl();
    const BuffDefinition& buff_definition(const ActiveBuff& buff) const;
    std::string buff_icon(const ActiveBuff& buff) const;
    bool actor_has_buff(const Actor& actor,const std::string& name) const;
    bool effect_buff_gate(const SpellDefinition& spell,Position target) const;
    bool has_player_buff_state() const;
    void sort_player_buffs();
    void apply_owned_buff(Actor& owner,std::size_t spell,std::size_t buff);
    void validate_monster_buff(const Actor& owner,const BuffDefinition& definition) const;
    void update_monster_buffs_impl(Actor& owner);
    void rebuild_monster(Actor& owner);
    void validate_player_buff(const BuffDefinition& definition) const;
};
Actor make_player(const ClassRanks& ranks,std::string name="Player");
}
