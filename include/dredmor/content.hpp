#pragma once
#include "dredmor/monsters.hpp"
#include "dredmor/session.hpp"

namespace dredmor {
struct SkillDefinition { int id;std::string name;int category;XmlNode source; };
struct RoomDefinition {
    std::string name,layer;
    std::vector<std::string> rows;
    XmlNode source;
    // Conservative developer-room collision import. Liquids/doors/blockers stay
    // blocked until their interactions are implemented; unknown symbols fail.
    World instantiate() const;
};
class GameContent {
public:
    static GameContent load(const std::filesystem::path& root,bool expansions=false);
    const std::vector<ItemDefinition>& items() const { return items_; }
    const std::vector<MonsterDefinition>& monsters() const { return monsters_; }
    const std::vector<SkillDefinition>& skills() const { return skills_; }
    const std::vector<RoomDefinition>& rooms() const { return rooms_; }
    const RoomDefinition& room(std::string_view name) const;
    const MonsterDefinition& monster(std::string_view name) const;
    const Tweaks& tweaks() const { return tweaks_; }
    Actor make_monster(std::string_view name,Position position,Difficulty difficulty,int dungeon_level=0) const;
    ClassRanks starting_ranks(const std::vector<int>& skill_ids) const;
private:
    Catalog catalog_;
    std::vector<ItemDefinition> items_;
    std::vector<MonsterDefinition> monsters_;
    std::vector<SkillDefinition> skills_;
    std::vector<RoomDefinition> rooms_;
    Tweaks tweaks_;
};
}
