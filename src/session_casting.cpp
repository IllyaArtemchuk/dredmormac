#include "dredmor/session.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t wrapping_add(std::int32_t a,std::int32_t b) {
    const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;
}
bool learned(const AbilityDefinition& ability,const std::vector<SkillRank>& skills) {
    for (const auto& skill:skills) if (skill.id==ability.skill_id) return ability.level<skill.rank;
    return false;
}
}
const SpellDefinition& Session::spell_definition(std::size_t index) const {
    if (!spells_) throw std::out_of_range("spell definitions unavailable");
    return spells_->at(index);
}
std::vector<std::size_t> Session::learned_spells() const {
    std::vector<std::size_t> result;
    if (abilities_ && spells_) for (const auto& ability:*abilities_) if (learned(ability,skills_)) {
        for (const auto& name:ability.spells) {
            auto found=std::find_if(spells_->begin(),spells_->end(),[&](const auto& spell){return spell.name==name;});
            if (found==spells_->end()) continue;
            const auto index=std::size_t(found-spells_->begin());
            if (std::find(result.begin(),result.end(),index)==result.end()) result.push_back(index);
        }
    }
    return result;
}
std::int32_t Session::spell_cooldown(std::size_t index) const {
    const auto found=spell_cooldowns_.find(index);return found==spell_cooldowns_.end()?0:found->second;
}
SpellUse Session::use_spell(std::size_t index,Position target) {
    if (!player_.alive()) return SpellUse::dead;
    const auto known=learned_spells();if (std::find(known.begin(),known.end(),index)==known.end()) return SpellUse::unknown;
    if (spell_cooldown(index)>0) return SpellUse::cooling_down;
    const auto& spell=spell_definition(index);const auto& cost=spell.casting;
    bool weapon=cost.weapon==-1,shield=!cost.shield;
    for (std::size_t hand:{1u,2u}) if (const auto& item=equipment_.slots()[hand]) {
        if (item->kind==ItemKind::weapon && item->weapon_type==cost.weapon) weapon=true;
        if (item->kind==ItemKind::armour) shield=true;
    }
    if (!weapon || !shield) return SpellUse::equipment_required;
    // Original click validation is separate from DoActualSpellLoop. Visibility
    // and non-actor obstructions need full level integration; floor bounds and
    // the supported actor/adjacency constraints are enforced here.
    if (spell.type=="self") target=player_.position;
    else {
        if (!passable(target)) return SpellUse::invalid_target;
        if (spell.type=="adjacent" && std::abs(std::int64_t(target.x)-player_.position.x)+std::abs(std::int64_t(target.y)-player_.position.y)!=1) return SpellUse::invalid_target;
        if (spell.type=="target" && (target.x!=player_.position.x || target.y!=player_.position.y) && !enemy_at(target)) return SpellUse::invalid_target;
    }
    if (cost.health || cost.booze || cost.level) throw std::invalid_argument("spell requirement dispatch is not implemented");
    auto next=*this;next.events_.clear();
    if (!cost.cooldown) {
        const auto mana=spell_mana_cost(cost,player_.secondary[3],player_.primary[5]);
        const auto gold=spell_gold_cost(cost,player_.primary[5],depth_context_);
        if (gold>player_.zorkmids || mana>player_.mana) {
            // Each failed resource check selects its own voice line. Both
            // branches run when both resources are insufficient.
            if (gold>player_.zorkmids) next.random_.libc.next();
            if (mana>player_.mana) next.random_.libc.next();
            *this=std::move(next);
            return gold>player_.zorkmids?SpellUse::insufficient_gold:SpellUse::insufficient_mana;
        }
        for (const auto& active:player_buffs_) if (buff_definition(active).mute) return SpellUse::muted;
        const auto bits=std::uint32_t(next.player_.mana)-std::uint32_t(mana);std::memcpy(&next.player_.mana,&bits,4);
        if (cost.gold) next.currency_transfer_impl(gold,CurrencyOperation::take);
    }
    if (spell.type!="self" && !cost.no_animation) {
        const auto dx=std::int64_t(target.x)-player_.position.x,dy=std::int64_t(target.y)-player_.position.y;
        next.player_facing_=std::abs(dx)<std::abs(dy)?(dy>=0?0:2):(dx>=0?3:1);
    }
    const auto count=cost.multi_attack?cost.multi_attack:1;
    if (count>4096) throw std::runtime_error("spell multi-attack execution budget exhausted");
    bool accepted=false;std::size_t remaining=4096;
    for (std::int32_t i=0;i<count;++i) accepted=next.cast_spell_recursive(spell,next.player_.position,target,next.player_.id,true,0,remaining);
    if (accepted && cost.cooldown) next.spell_cooldowns_[index]=wrapping_add(spell_cooldown(index),wrapping_add(1,cost.downtime));
    if (abilities_) for (const auto& ability:*abilities_) if (learned(ability,skills_) && !ability.cast_trigger.empty()) {
        const auto child=std::find_if(spells_->begin(),spells_->end(),[&](const auto& entry){return entry.name==ability.cast_trigger;});
        if (child!=spells_->end() && next.random_.libc.next()/21474837<ability.cast_trigger_percent)
            next.cast_spell_recursive(*child,next.player_.position,target,next.player_.id,false,0,remaining);
    }
    next.world_.step(Action::wait);next.enemy_turns();*this=std::move(next);return SpellUse::cast;
}
}
