#include "dredmor/session.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t add(std::int32_t a,std::int32_t b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;}
bool same(Position a,Position b) {return a.x==b.x && a.y==b.y;}
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+=32;return value;}
}
bool Session::actor_has_buff(const Actor& actor,const std::string& name) const {
    if (actor.id!=player_.id) return false; // Monster-owned buffs are not yet supported.
    for (const auto& buff:player_buffs_) if (spells_->at(buff.spell).name==name) return true;
    return false;
}
bool Session::effect_buff_gate(const SpellDefinition& spell,Position target) const {
    for (const auto& definition:spell.buffs) if (definition.apply_to_player) {
        if (same(target,player_.position)) {
            bool present=false;for (const auto& buff:player_buffs_) if (buff_icon(buff)==definition.icon) {present=true;break;}
            if (!present) return true;
        } else {
            // Owned monster buffs are unavailable in the current session;
            // the modeled monster buff vector is empty.
            for (const auto& enemy:enemies_) if (enemy.alive() && same(enemy.position,target)) return true;
        }
    }
    return false;
}
bool Session::has_player_buff_state() const {
    return !player_buffs_.empty() || std::any_of(buff_primary_flat_.begin(),buff_primary_flat_.end(),[](auto n){return n!=0;}) ||
        std::any_of(buff_secondary_flat_.begin(),buff_secondary_flat_.end(),[](auto n){return n!=0;});
}
void Session::sort_player_buffs() {
    std::vector<std::string> names;for (const auto& buff:player_buffs_) names.push_back(spells_->at(buff.spell).name);
    const auto order=buff_order(names);auto previous=std::move(player_buffs_);player_buffs_.clear();player_buffs_.reserve(previous.size());
    for (auto index:order) player_buffs_.push_back(std::move(previous[index]));
}
const BuffDefinition& Session::buff_definition(const ActiveBuff& buff) const {
    if (!spells_ || buff.spell>=spells_->size() || buff.buff>=spells_->at(buff.spell).buffs.size()) throw std::invalid_argument("active buff rules unavailable");
    return spells_->at(buff.spell).buffs[buff.buff];
}
std::string Session::buff_icon(const ActiveBuff& buff) const {
    const auto& definition=buff_definition(buff);if (!definition.icon.empty()) return definition.icon;
    for (const auto& a:spells_->at(buff.spell).source.attributes) if (lower(a.first)=="icon") return a.second;
    return {};
}
void Session::apply_spell_buffs(const SpellDefinition& spell,Position target,std::uint64_t caster,bool self) {
    if (spell.buffs.empty()) return;
    if (!spells_) throw std::invalid_argument("buffs require registered spell rules");
    std::size_t index=0;while (index<spells_->size() && spells_->at(index).name!=spell.name) ++index;
    if (index==spells_->size() || spells_->at(index).buffs.size()!=spell.buffs.size()) throw std::invalid_argument("buff spell is not registered");
    for (std::size_t i=0;i<spell.buffs.size();++i) {
        const auto& buff=spell.buffs[i];
        if (buff_signature(buff)!=buff_signature(spells_->at(index).buffs[i])) throw std::invalid_argument("cast buff differs from registered rules");
        if (self) {
            if (actor_by_id(caster).id!=player_.id) throw std::invalid_argument("monster buff rebuilding is not implemented");
            apply_player_buff(index,i);continue;
        }
        if (!buff.apply_to_player) continue;
        if (same(target,player_.position)) apply_player_buff(index,i);
        for (const auto& enemy:enemies_) if (same(target,enemy.position)) {
            if (!buff.self || actor_by_id(caster).id!=player_.id) throw std::invalid_argument("monster buff rebuilding is not implemented");
            // Nonstacking monster-target icon checks are not needed here: the
            // supported path redirects to the player (buff self default true).
            apply_player_buff(index,i);
        }
    }
}
void Session::validate_player_buff(const BuffDefinition& definition) const {
    if (!definitions_) throw std::invalid_argument("player buffs require a character stat profile");
    if (definition.destroy_on_move || definition.brittle || definition.attacks || definition.zorkmid_upkeep || definition.requires_weapon!=-1 || definition.requires_shield ||
        definition.invisible || definition.mute || definition.diggle_god) throw std::invalid_argument("buff status/upkeep execution is not implemented");
    for (const auto& node:definition.source.children) {
        const auto tag=lower(node.tag);
        if (tag!="primarybuff" && tag!="highbuff" && tag!="secondarybuff" && tag!="damagebuff" && tag!="resistbuff" && tag!="sightbuff" && tag!="halo" && tag!="description")
            throw std::invalid_argument("buff child execution is not implemented: "+node.tag);
    }
}
void Session::apply_player_buff(std::size_t spell,std::size_t buff) {
    ActiveBuff active{spell,buff};const auto& definition=buff_definition(active);validate_player_buff(definition);
    active.remaining=definition.time;active.upkeep_remaining=definition.mana_upkeep;const auto icon=buff_icon(active);int count=0;
    for (auto& present:player_buffs_) if (buff_icon(present)==icon) {
        if (!definition.allow_stacking && !present.deapplied) {present.remaining=definition.time;return;}
        ++count;
    }
    if (definition.stack_size>0 && count>=definition.stack_size) return;
    if (player_buffs_.size()>=4096) throw std::runtime_error("active buff capacity exhausted");
    player_.health=add(player_.health,definition.modifiers.secondary[0]);player_.mana=add(player_.mana,definition.modifiers.secondary[1]);
    for (std::size_t i=0;i<6;++i) buff_primary_flat_[i]=add(buff_primary_flat_[i],definition.modifiers.primary[i]);
    for (std::size_t i=0;i<24;++i) buff_secondary_flat_[i]=add(buff_secondary_flat_[i],definition.modifiers.secondary[i]);
    player_buffs_.push_back(active);rebuild_equipment();
    if (player_.health<1 || player_.secondary[0]<1 || player_.secondary[1]<0) throw std::runtime_error("buff lethal stat transition is not implemented");
    player_.mana=std::max(0,player_.mana);
    sort_player_buffs();
}
void Session::update_player_buffs() {
    auto next=*this;next.events_.clear();next.update_player_buffs_impl();*this=std::move(next);
}
void Session::update_player_buffs_impl() {
    // Player::UpdateBuffs handles every upkeep payment before the shared
    // Hittable timer pass. A nonstacking refresh leaves this clock unchanged.
    for (std::size_t i=0;i<player_buffs_.size();++i) {
        auto& buff=player_buffs_[i];const auto interval=buff_definition(buff).mana_upkeep;
        if (!interval) continue;
        buff.upkeep_remaining=add(buff.upkeep_remaining,-1);
        if (!buff.upkeep_remaining) {
            if (player_.mana<1) buff.pending=true;
            else {--player_.mana;buff.upkeep_remaining=interval;}
        }
    }
    for (std::size_t index=0;index<player_buffs_.size();++index) {
        auto& buff=player_buffs_[index];const auto& definition=buff_definition(buff);
        const auto tick=tick_buff({buff.remaining,false,definition.use_timer,buff.pending});buff.remaining=tick.clock.remaining;
        if (tick.deapply) {
            buff.deapplied=true;
            for (std::size_t i=0;i<6;++i) buff_primary_flat_[i]=add(buff_primary_flat_[i],add(~definition.modifiers.primary[i],1));
            for (std::size_t i=0;i<24;++i) buff_secondary_flat_[i]=add(buff_secondary_flat_[i],add(~definition.modifiers.secondary[i],1));
            player_.health=add(player_.health,add(~definition.modifiers.secondary[0],1));
            player_.mana=add(player_.mana,add(~definition.modifiers.secondary[1],1));
            const auto& name=spells_->at(buff.spell).name;
            player_.triggers.erase(std::remove_if(player_.triggers.begin(),player_.triggers.end(),[&](const auto& trigger){return trigger.parent==name;}),player_.triggers.end());
            // Original rebuilding happens before the buff vector erases the
            // expired entry. Damage/resistance therefore survive until a later
            // rebuild; flat primary/secondary modifiers are already removed.
            rebuild_equipment();player_.health=std::max(1,player_.health);player_.mana=std::max(0,player_.mana);
            if (player_.health>player_.secondary[0] || player_.mana>player_.secondary[1]) throw std::runtime_error("buff removal exceeds current stat invariants");
            // Player::DeapplyBuff sorts before Hittable resumes traversal.
            // A moved entry can be visited again or skipped in this same pass.
            sort_player_buffs();
        }
    }
    player_buffs_.erase(std::remove_if(player_buffs_.begin(),player_buffs_.end(),[&](const auto& buff){return buff.pending || (buff.remaining<0 && buff_definition(buff).use_timer);}),player_buffs_.end());
}
}
