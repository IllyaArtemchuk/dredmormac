#pragma once
#include <array>
#include <cstdint>

namespace dredmor {
// IDs follow the supplied game's primarybuff/secondarybuff tables.
enum class Primary : std::size_t { burliness, sagacity, nimbleness, caddishness, stubbornness, savvy };
enum class Secondary : std::size_t {
    health, mana, melee_power, magic_power, critical, haywire, dodge, block,
    counter, accuracy, armour, magic_resistance, sneak, life_regen, mana_regen,
    wand_affinity, trap_sense, trap_sight, sight, reserved19, reserved20, reserved21,
    reserved22, reserved23
};
using PrimaryStats=std::array<std::int32_t,6>;
using SecondaryStats=std::array<std::int32_t,24>;
using ClassRanks=std::array<std::int32_t,3>; // warrior, wizard, rogue
PrimaryStats default_primary_percentages();
PrimaryStats derive_primary(const ClassRanks& ranks,const PrimaryStats& flat={},
                           const PrimaryStats& percent=default_primary_percentages());
SecondaryStats default_percentages();
// Verified against the original primary-to-secondary calculation fragment.
// Skill/equipment/buff aggregation takes place outside this calculation.
SecondaryStats derive_secondary(const PrimaryStats& primary,
    const SecondaryStats& flat={},const SecondaryStats& percent=default_percentages());
struct MonsterStatInput {
    ClassRanks ranks{};
    PrimaryStats definition_primary{},primary_flat{},primary_percent=default_primary_percentages();
    SecondaryStats definition_secondary{},secondary_flat{},secondary_percent=default_percentages();
    std::int32_t dungeon_level=0,health_per_level=0;
    float health_scale=1.0f,health_percent_per_level=0.0f;
};
struct MonsterStats { PrimaryStats primary;SecondaryStats secondary; };
MonsterStats derive_monster(const MonsterStatInput& input);
}
