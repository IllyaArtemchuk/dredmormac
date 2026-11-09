#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

namespace dredmor {
enum class DamageType : std::size_t {
    crushing, slashing, blasting, piercing, conflagratory, hyperborean, voltaic,
    toxic, righteous, necromantic, acidic, putrefying, transmutative, aethereal,
    asphyxiative, existential
};
inline constexpr std::array<std::string_view,16> damage_names={
    "crushing","slashing","blasting","piercing","conflagratory","hyperborean",
    "voltaic","toxic","righteous","necromantic","acidic","putrefying",
    "transmutative","aethereal","asphyxiative","existential"};
using Damage=std::array<std::int32_t,16>;
struct CombatInput {
    std::int32_t critical=0, dodge=0, counter=0, block=0, melee_power=0, armour=0;
    Damage damage{},resistance{};
};
enum class CombatOutcome { hit, dodged, countered, no_effect };
struct CombatResult {
    CombatOutcome outcome=CombatOutcome::hit;
    bool critical=false,blocked=false;
    std::int32_t physical=0,elemental=0,total=0;
    unsigned draws=0;
};
// Low-level DoCombatDamage rules. Draw source supplies original libc rand()
// values in [0, INT32_MAX], not a 0..99 uniform distribution or an MT stream.
// Counterattacks are returned to the caller; messages/VFX/triggers are separate.
CombatResult resolve_combat(const CombatInput& input,const std::function<std::int32_t()>& draw);
struct SpellDamageInput {
    std::int32_t haywire=0,resist=0,armour=0,scaling_attribute=0;
    bool allow_haywire=false,force_unresisted=false;
    Damage damage{},resistance{};
    std::array<float,16> scaling{};
};
struct SpellDamageResult {
    bool haywire=false,resisted=false;
    std::int32_t physical=0,elemental=0,total=0;
    unsigned draws=0;
};
// Arithmetic/decisions from DoSpellDamage; delivery, effects and UI are external.
SpellDamageResult resolve_spell_damage(const SpellDamageInput& input,const std::function<std::int32_t()>& draw);
}
