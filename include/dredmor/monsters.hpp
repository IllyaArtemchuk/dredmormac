#pragma once
#include "dredmor/catalog.hpp"
#include "dredmor/combat.hpp"
#include "dredmor/stats.hpp"
#include "dredmor/random.hpp"

namespace dredmor {
struct MonsterDefinition {
    std::string name,parent,layer;
    int level=0,experience=0,armour_ac=0,aggressiveness=0,span=0;
    ClassRanks ranks{};
    PrimaryStats primary{};
    SecondaryStats secondary{};
    Damage damage{},resistance{};
    std::array<std::string,4> idle_sprites{}; // left, right, up, down
    std::vector<XmlNode> source_chain;
};
MonsterDefinition read_monster(const XmlNode& node,const MonsterDefinition* parent=nullptr);
std::vector<MonsterDefinition> load_monsters(const Catalog& catalog);
// Constructor difficulty adjustment, before RebuildAttributes. Uses the libc
// stream and the original multiply-high mapping over three class ranks.
ClassRanks monster_creation_ranks(ClassRanks ranks,int level_adder,DarwinRandom& random);
enum class Difficulty : std::size_t { easy,medium,hard };
class Tweaks {
public:
    static Tweaks read(const XmlNode& root);
    int integer(Difficulty difficulty,std::string_view name) const;
    float real(Difficulty difficulty,std::string_view name) const;
    MonsterStatInput monster_input(const MonsterDefinition& definition,Difficulty difficulty,int dungeon_level) const;
private:
    struct Value { int integer=0;float real=0; };
    std::array<std::map<std::string,Value,std::less<>>,3> values_;
};
}
