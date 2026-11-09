#pragma once
#include "dredmor/monsters.hpp"
#include "dredmor/session.hpp"
#include "dredmor/generation.hpp"
#include "dredmor/abilities.hpp"
#include "dredmor/names.hpp"
#include "dredmor/room_records.hpp"
#include "dredmor/artifact.hpp"

namespace dredmor {
struct SkillLoadout { std::string type,subtype;int amount=1;bool always=false; };
struct StartingLoadout { int skill_id;SkillLoadout item; };
struct SkillDefinition { int id;std::string name;int category;XmlNode source;std::vector<SkillLoadout> loadouts;bool deprecated=false; };
struct RoomDefinition {
    std::string name,layer;
    std::vector<std::string> rows;
    XmlNode source;
    // Conservative developer-room collision import. Liquids/doors/blockers stay
    // blocked until their interactions are implemented; unknown symbols fail.
    // false leaves custom/pedestal collision to Session, retaining the same preview spawn.
    World instantiate(bool custom_blockers=true) const;
    // Original numeric cell/door/wall arrays, before scripts and population.
    RoomStamp numeric_template() const;
    std::vector<RoomElementDefinition> elements() const;
    std::vector<CustomRoomDefinition> custom_objects() const;
    std::vector<RoomPopulationRecord> population() const;
    // All recognized object records in original source order. Scripts have a
    // separate list; lever/clock/magicblocker retain raw pending definitions.
    std::vector<RoomRecord> records() const;
};
class GameContent {
public:
    static GameContent load(const std::filesystem::path& root,bool expansions=false);
    const std::vector<ItemDefinition>& items() const { return items_; }
    const std::vector<MonsterDefinition>& monsters() const { return monsters_; }
    const std::vector<SkillDefinition>& skills() const { return skills_; }
    const std::vector<AbilityDefinition>& abilities() const { return abilities_; }
    const std::vector<RoomDefinition>& rooms() const { return rooms_; }
    const RoomDefinition& room(std::string_view name) const;
    const MonsterDefinition& monster(std::string_view name) const;
    const Tweaks& tweaks() const { return tweaks_; }
    const RoomNameWords& room_name_words() const { return room_name_words_; }
    const ArtifactWords& artifact_words() const { return artifact_words_; }
    const ItemFactoryTables& item_factory_tables() const;
    ItemFactoryContext loot_context(SpawnDepthContext depth={},std::size_t level_set=0,Difficulty difficulty=Difficulty::medium) const;
    Actor make_monster(std::string_view name,Position position,Difficulty difficulty,int dungeon_level=0) const;
    ClassRanks starting_ranks(const std::vector<int>& skill_ids) const;
    std::vector<StartingLoadout> starting_loadout(const std::vector<int>& skill_ids) const;
private:
    Catalog catalog_;
    std::vector<ItemDefinition> items_;
    std::vector<MonsterDefinition> monsters_;
    std::vector<SkillDefinition> skills_;
    std::vector<AbilityDefinition> abilities_;
    std::vector<RoomDefinition> rooms_;
    Tweaks tweaks_;
    RoomNameWords room_name_words_;
    ArtifactWords artifact_words_;
    std::optional<ItemFactoryTables> item_factory_tables_;
};
}
