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
    const auto& buffs=actor.id==player_.id?player_buffs_:actor.buffs;
    for (const auto& buff:buffs) if (spells_->at(buff.spell).name==name) return true;
    return false;
}
bool Session::effect_buff_gate(const SpellDefinition& spell,Position target) const {
    for (const auto& definition:spell.buffs) if (definition.apply_to_player) {
        if (same(target,player_.position)) {
            bool present=false;for (const auto& buff:player_buffs_) if (buff_icon(buff)==definition.icon) {present=true;break;}
            if (!present) return true;
        } else {
            for (const auto& enemy:enemies_) if (enemy.alive() && same(enemy.position,target)) {
                bool present=false;for (const auto& buff:enemy.buffs) if (buff_icon(buff)==definition.icon) {present=true;break;}
                if (!present) return true;
            }
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
            apply_owned_buff(actor_by_id(caster),index,i);continue;
        }
        if (!buff.apply_to_player) continue;
        if (same(target,player_.position)) apply_player_buff(index,i);
        for (auto& enemy:enemies_) if (same(target,enemy.position)) {
            if (!buff.allow_stacking) for (const auto& present:enemy.buffs) if (buff_icon(present)==buff.icon) return;
            apply_owned_buff(buff.self?actor_by_id(caster):enemy,index,i);
        }
    }
}
void Session::validate_player_buff(const BuffDefinition& definition) const {
    if (!definitions_) throw std::invalid_argument("player buffs require a character stat profile");
    if (definition.destroy_on_move || definition.brittle || definition.attacks || definition.requires_weapon!=-1 || definition.requires_shield ||
        definition.invisible || definition.mute || definition.diggle_god) throw std::invalid_argument("buff status/upkeep execution is not implemented");
    for (const auto& node:definition.source.children) {
        const auto tag=lower(node.tag);
        if (tag!="primarybuff" && tag!="highbuff" && tag!="secondarybuff" && tag!="damagebuff" && tag!="resistbuff" && tag!="sightbuff" && tag!="halo" && tag!="description")
            throw std::invalid_argument("buff child execution is not implemented: "+node.tag);
    }
}
void Session::validate_monster_buff(const Actor& owner,const BuffDefinition& definition) const {
    if (!owner.stat_profile) throw std::invalid_argument("monster buffs require a retained stat profile");
    if (definition.destroy_on_move || definition.brittle || definition.attacks || definition.invisible || definition.mute || definition.diggle_god)
        throw std::invalid_argument("monster buff status execution is not implemented");
    for (const auto& node:definition.source.children) {
        const auto tag=lower(node.tag);
        if (tag!="primarybuff" && tag!="highbuff" && tag!="secondarybuff" && tag!="damagebuff" && tag!="resistbuff" && tag!="sightbuff" && tag!="halo" && tag!="description")
            throw std::invalid_argument("monster buff child execution is not implemented: "+node.tag);
    }
}
void Session::rebuild_monster(Actor& owner) {
    if (!owner.stat_profile) throw std::invalid_argument("monster stat profile unavailable");
    const auto stats=derive_monster(*owner.stat_profile);owner.primary=stats.primary;owner.secondary=stats.secondary;
    owner.health=std::min(owner.health,owner.secondary[0]);owner.mana=std::min(owner.mana,owner.secondary[1]);
}
void Session::apply_owned_buff(Actor& owner,std::size_t spell,std::size_t buff) {
    if (owner.id==player_.id) {apply_player_buff(spell,buff);return;}
    ActiveBuff active{spell,buff};const auto& definition=buff_definition(active);validate_monster_buff(owner,definition);
    if (!owner.alive()) throw std::invalid_argument("dead monster buff transitions are not implemented");
    if (owner.buffs.size()>=4096) throw std::runtime_error("monster buff capacity exhausted");
    active.remaining=definition.time;active.upkeep_remaining=definition.mana_upkeep;owner.buffs.push_back(active);
    auto& input=*owner.stat_profile;
    for (std::size_t i=0;i<6;++i) input.primary_flat[i]=add(input.primary_flat[i],definition.modifiers.primary[i]);
    for (std::size_t i=0;i<24;++i) input.secondary_flat[i]=add(input.secondary_flat[i],definition.modifiers.secondary[i]);
    for (std::size_t i=0;i<16;++i) {owner.damage[i]=add(owner.damage[i],definition.modifiers.damage[i]);owner.resistance[i]=add(owner.resistance[i],definition.modifiers.resistance[i]);}
    owner.health=add(owner.health,definition.modifiers.secondary[0]);owner.mana=add(owner.mana,definition.modifiers.secondary[1]);
    rebuild_monster(owner);
    if (!owner.alive() || owner.secondary[0]<1 || owner.secondary[1]<0) throw std::runtime_error("monster buff lethal stat transition is not implemented");
    owner.mana=std::max(0,owner.mana);
}
void Session::update_monster_buffs_impl(Actor& owner) {
    for (auto& buff:owner.buffs) {
        const auto& definition=buff_definition(buff);
        const auto tick=tick_buff({buff.remaining,false,definition.use_timer,false});buff.remaining=tick.clock.remaining;
        if (!tick.deapply) continue;
        const bool was_dead=!owner.alive();
        auto& input=*owner.stat_profile;
        for (std::size_t i=0;i<6;++i) input.primary_flat[i]=add(input.primary_flat[i],add(~definition.modifiers.primary[i],1));
        for (std::size_t i=0;i<24;++i) input.secondary_flat[i]=add(input.secondary_flat[i],add(~definition.modifiers.secondary[i],1));
        for (std::size_t i=0;i<16;++i) {owner.damage[i]=add(owner.damage[i],add(~definition.modifiers.damage[i],1));owner.resistance[i]=add(owner.resistance[i],add(~definition.modifiers.resistance[i],1));}
        owner.health=add(owner.health,add(~definition.modifiers.secondary[0],1));owner.mana=add(owner.mana,add(~definition.modifiers.secondary[1],1));
        const auto& name=spells_->at(buff.spell).name;
        owner.triggers.erase(std::remove_if(owner.triggers.begin(),owner.triggers.end(),[&](const auto& trigger){return trigger.parent==name;}),owner.triggers.end());
        rebuild_monster(owner);owner.health=std::max(1,owner.health);owner.mana=std::max(0,owner.mana);
        if (was_dead) owner.corpse=true;
        if (owner.health>owner.secondary[0] || owner.mana>owner.secondary[1]) throw std::runtime_error("monster buff removal exceeds stat invariants");
        buff.deapplied=true;
    }
    owner.buffs.erase(std::remove_if(owner.buffs.begin(),owner.buffs.end(),[](const auto& buff){return buff.deapplied;}),owner.buffs.end());
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
    update_buffs(player_.id);
}
void Session::update_buffs(std::uint64_t actor) {
    auto next=*this;next.events_.clear();auto& owner=next.actor_by_id(actor);
    if (owner.id==next.player_.id) next.update_player_buffs_impl();else next.update_monster_buffs_impl(owner);
    *this=std::move(next);
}
void Session::update_player_buffs_impl() {
    // Player::UpdateBuffs handles every upkeep payment before the shared
    // Hittable timer pass. A nonstacking refresh leaves this clock unchanged.
    std::vector<std::string> insufficient;
    for (std::size_t i=0;i<player_buffs_.size();++i) {
        auto& buff=player_buffs_[i];const auto& definition=buff_definition(buff);const auto interval=definition.mana_upkeep;
        if (interval) {
            buff.upkeep_remaining=add(buff.upkeep_remaining,-1);
            if (!buff.upkeep_remaining) {
                if (player_.mana<1) buff.pending=true;
                else {--player_.mana;buff.upkeep_remaining=interval;}
            }
        }
        if (definition.zorkmid_upkeep>0) {
            const auto cost=buff_gold_upkeep(definition.zorkmid_upkeep,definition.zorkmid_scale,depth_context_);
            if (player_.zorkmids<cost) {
                if (!definition.insufficient_funds.empty()) insufficient.push_back(definition.insufficient_funds);
                buff.pending=true;
            } else currency_transfer_impl(cost,CurrencyOperation::take);
        }
    }
    for (const auto& name:insufficient) if (spells_) for (const auto& spell:*spells_) if (spell.name==name) {
        cast_spell_impl(spell,player_.position,player_.position,player_.id,true);break;
    }
    for (std::size_t index=0;index<player_buffs_.size();++index) {
        auto& buff=player_buffs_[index];const auto& definition=buff_definition(buff);
        const auto tick=tick_buff({buff.remaining,false,definition.use_timer,buff.pending});buff.remaining=tick.clock.remaining;
        if (tick.deapply) {
            const bool was_dead=!player_.alive();
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
            if (was_dead) player_.corpse=true;
            if (player_.health>player_.secondary[0] || player_.mana>player_.secondary[1]) throw std::runtime_error("buff removal exceeds current stat invariants");
            // Player::DeapplyBuff sorts before Hittable resumes traversal.
            // A moved entry can be visited again or skipped in this same pass.
            sort_player_buffs();
        }
    }
    player_buffs_.erase(std::remove_if(player_buffs_.begin(),player_buffs_.end(),[&](const auto& buff){return buff.pending || (buff.remaining<0 && buff_definition(buff).use_timer);}),player_buffs_.end());
}
}
