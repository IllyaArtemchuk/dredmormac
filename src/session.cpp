#include "dredmor/session.hpp"
#include <algorithm>
#include <array>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cstring>
#include "dredmor/legacy_number.hpp"

namespace dredmor {
namespace {
bool same(Position a,Position b) { return a.x==b.x && a.y==b.y; }
std::int32_t add(std::int32_t a,std::int32_t b) {
    const std::uint32_t bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;
}
void artifact_resources(Actor& actor,const Artifact& artifact,bool remove) {
    auto change=[&](std::int32_t value,std::int32_t amount) {
        const auto bits=remove?std::uint32_t(value)-std::uint32_t(amount):std::uint32_t(value)+std::uint32_t(amount);
        std::int32_t result;std::memcpy(&result,&bits,4);return result;
    };
    actor.health=std::max(remove?1:0,change(actor.health,artifact.modifiers.secondary[0]));
    actor.mana=std::max(0,change(actor.mana,artifact.modifiers.secondary[1]));
}
Position target(Position p,Action action) {
    switch (action) {
        case Action::north:--p.y;break;case Action::south:++p.y;break;
        case Action::west:--p.x;break;case Action::east:++p.x;break;
        case Action::wait:break;default:throw std::invalid_argument("invalid session action");
    }
    return p;
}
std::string quoted(const std::string& value) {
    std::string result="\"";
    const char* digits="0123456789abcdef";
    for (unsigned char c:value) {
        if (c=='"' || c=='\\') { result+='\\';result+=char(c); }
        else if (c<32) { result+="\\u00";result+=digits[c>>4];result+=digits[c&15]; }
        else result+=char(c);
    }
    return result+'"';
}
}
Actor make_player(const ClassRanks& ranks,std::string name) {
    Actor actor;actor.id=1;actor.name=std::move(name);actor.primary=derive_primary(ranks);
    actor.secondary=derive_secondary(actor.primary);actor.health=actor.secondary[0];actor.mana=actor.secondary[1];
    return actor;
}
Session::Session(World world,Actor player,std::uint32_t seed,int experience_n)
    :world_(std::move(world)),player_(std::move(player)),experience_n_(experience_n) {
    if (!player_.id || player_.id==std::numeric_limits<std::uint64_t>::max() || player_.health<0 || player_.health>player_.secondary[0])
        throw std::invalid_argument("invalid player");
    player_.position=world_.player();next_actor_=player_.id+1;random_.seed(seed);
    intrinsic_damage_=player_.damage;intrinsic_resistance_=player_.resistance;
}
Actor* Session::enemy_at(Position position,std::uint64_t excluding) {
    for (auto& enemy:enemies_) if (enemy.id!=excluding && enemy.alive() && same(enemy.position,position)) return &enemy;
    return nullptr;
}
bool Session::passable(Position position) const {
    if (!world_.passable(position)) return false;
    for (const auto& object:custom_objects_) if (object.blocks_movement && same(object.position,position)) return false;
    for (const auto& object:breakables_) if (object.object.blocks_movement && same(object.object.position,position)) return false;
    for (const auto& trap:traps_) if (trap.has_origin_object && same(trap.origin,position)) return false;
    return true;
}
TrapSelection Session::select_trap_candidate(const std::vector<TrapCandidate>& pool,SpawnDepthContext depth,float deviation) {
    return select_trap(pool,random_,depth,deviation);
}
std::optional<std::size_t> Session::create_custom_object(const CustomRoomDefinition& definition,Position origin) {
    auto candidate=random_.libc;auto object=create_custom_room_object(definition,origin,candidate);
    if (!object) {random_.libc=candidate;return std::nullopt;}
    if (custom_objects_.size()>=10000) throw std::runtime_error("session custom object limit exceeded");
    if (object->position.x<0 || object->position.y<0 || object->position.x>=world_.width() || object->position.y>=world_.height())
        throw std::invalid_argument("custom object outside session world");
    custom_objects_.push_back(std::move(*object));random_.libc=candidate;return custom_objects_.size()-1;
}
std::optional<std::size_t> Session::create_pedestal(const RoomPedestalRecord& definition,Position origin) {
    auto candidate=random_.libc;auto object=create_room_pedestal(definition,origin,candidate);
    if (!object) {random_.libc=candidate;return std::nullopt;}
    if (custom_objects_.size()>=10000) throw std::runtime_error("session custom object limit exceeded");
    if (object->position.x<0 || object->position.y<0 || object->position.x>=world_.width() || object->position.y>=world_.height())
        throw std::invalid_argument("pedestal outside session world");
    custom_objects_.push_back(std::move(*object));random_.libc=candidate;return custom_objects_.size()-1;
}
std::optional<std::size_t> Session::create_breakable(const BreakableRoomDefinition& definition,Position origin,BreakableFrames frames) {
    return create_breakable(definition,origin,[frames](const auto&){return frames;});
}
std::optional<std::size_t> Session::create_breakable(const BreakableRoomDefinition& definition,Position origin,
    const std::function<BreakableFrames(const BreakableRoomDefinition&)>& resources) {
    auto candidate=random_.libc;auto object=create_breakable_room_object(definition,origin,candidate,resources);
    if (!object) {random_.libc=candidate;return std::nullopt;}
    if (breakables_.size()>=10000) throw std::runtime_error("session breakable limit exceeded");
    const auto p=object->object.position;
    if (p.x<0 || p.y<0 || p.x>=world_.width() || p.y>=world_.height()) throw std::invalid_argument("breakable outside session world");
    breakables_.push_back(std::move(*object));random_.libc=candidate;return breakables_.size()-1;
}
bool Session::hit_breakable_impl(std::size_t index) {
    if (index>=breakables_.size()) throw std::out_of_range("invalid breakable index");
    const auto result=hit_breakable(breakables_[index]);
    for (const auto& event:result.triggers) events_.push_back({event=="hit"?EventKind::object_hit:EventKind::object_broken,player_.id,index+1,breakables_[index].hits_taken});
    return result.handled;
}
bool Session::strike_breakable(std::size_t index) {
    if (index>=breakables_.size()) throw std::out_of_range("invalid breakable index");
    if (!player_.alive()) return false;
    const auto p=breakables_[index].object.position;
    if (std::abs(p.x-player_.position.x)+std::abs(p.y-player_.position.y)!=1) return false;
    if (world_.turn()==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("turn counter exhausted");
    auto next=*this;next.events_.clear();if (!next.hit_breakable_impl(index)) return false;
    next.world_.step(Action::wait);next.enemy_turns();*this=std::move(next);return true;
}
std::uint64_t Session::spawn_enemy(Actor enemy) {
    if (!passable(enemy.position) || same(enemy.position,player_.position) || enemy_at(enemy.position) ||
        !enemy.alive() || enemy.health>enemy.secondary[0]) throw std::invalid_argument("invalid enemy spawn");
    if (enemies_.size()>=512) throw std::runtime_error("session enemy limit exceeded");
    if (enemy.id==std::numeric_limits<std::uint64_t>::max() || next_actor_==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("actor ID sequence exhausted");
    if (!enemy.id) enemy.id=next_actor_++;
    else {
        if (enemy.id==player_.id || std::any_of(enemies_.begin(),enemies_.end(),[&](const Actor& a){return a.id==enemy.id;}))
            throw std::invalid_argument("duplicate actor ID");
        next_actor_=std::max(next_actor_,enemy.id+1);
    }
    enemies_.push_back(std::move(enemy));return enemies_.back().id;
}
void Session::drop(InventoryItem item,Position position) {
    if (!world_.passable(position) || !item.instance || item.quantity<1 || inventory_.find(item.instance) || ground_.size()>=10000 ||
        (item.charges && (item.kind!=ItemKind::wand || *item.charges<0)) ||
        (item.currency_value.has_value()!=(item.kind==ItemKind::currency)) ||
        (item.artifact && item.kind!=ItemKind::weapon && item.kind!=ItemKind::armour))
        throw std::invalid_argument("invalid ground item");
    if (definitions_ && (item.definition>=definitions_->size() || definitions_->at(item.definition).kind!=item.kind))
        throw std::invalid_argument("ground item definition mismatch");
    for (const auto& slot:equipment_.slots()) if (slot && slot->instance==item.instance) throw std::invalid_argument("ground item is already equipped");
    for (const auto& existing:ground_) if (existing.item.instance==item.instance) throw std::invalid_argument("duplicate ground item");
    for (const auto& existing:traps_) if (existing.instance==item.instance) throw std::invalid_argument("ground item ID is owned by a trap");
    ground_.push_back({item,position});
}
std::uint64_t Session::next_item_instance() const {
    std::uint64_t identifier=1;
    auto observe=[&](const InventoryItem& item) {
        if (item.instance==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("item IDs exhausted");
        identifier=std::max(identifier,item.instance+1);
    };
    for (const auto& slot:inventory_.bag()) if (slot) observe(*slot);
    for (const auto& slot:inventory_.belt()) if (slot) observe(*slot);
    for (const auto& slot:equipment_.slots()) if (slot) observe(*slot);
    for (const auto& ground:ground_) observe(ground.item);
    for (const auto& trap:traps_) {InventoryItem identity;identity.instance=trap.instance;observe(identity);}
    return identifier;
}
std::uint64_t Session::create_ground_item(std::uint32_t definition,Position position,std::int32_t quantity) {
    if (!definitions_ || definition>=definitions_->size() || quantity<1) throw std::invalid_argument("invalid ground item creation");
    const auto identifier=next_item_instance();
    auto next=*this;
    InventoryItem item;
    if (definitions_->at(definition).kind==ItemKind::currency) {
        static const std::vector<AbilityDefinition> empty;
        item=make_currency_item(definitions_->at(definition),definition,identifier,1,skills_,abilities_?*abilities_:empty);
        if (quantity>1) item.stackable=true;
    } else item=make_inventory_item(definitions_->at(definition),definition,identifier,next.random_.libc);
    item.quantity=quantity;
    next.drop(item,position);*this=std::move(next);return identifier;
}
std::uint64_t Session::create_ground_currency(std::int32_t amount,Position position) {
    if (!definitions_) throw std::invalid_argument("currency creation needs item definitions");
    const auto definition=std::find_if(definitions_->begin(),definitions_->end(),[](const auto& item){return item.kind==ItemKind::currency && item.builtin_subtype==2;});
    if (definition==definitions_->end()) throw std::invalid_argument("currency definition missing");
    const auto identifier=next_item_instance();static const std::vector<AbilityDefinition> empty;
    const auto item=make_currency_item(*definition,std::uint32_t(definition-definitions_->begin()),identifier,amount,skills_,abilities_?*abilities_:empty);
    drop(item,position);return identifier;
}
std::uint64_t Session::create_ground_artifact(std::uint32_t definition,int quality,const ArtifactWords& words,Position position) {
    if (!definitions_ || definition>=definitions_->size()) throw std::invalid_argument("invalid artifact definition");
    const auto& entry=definitions_->at(definition);
    if (entry.kind!=ItemKind::weapon && entry.kind!=ItemKind::armour) throw std::invalid_argument("artifact requires weapon or armour");
    auto next=*this;const auto identifier=next_item_instance();auto item=make_inventory_item(entry,definition,identifier,next.random_.libc);
    item.artifact=create_artifact(words,artifact_request(entry,quality),next.random_);
    next.drop(std::move(item),position);*this=std::move(next);return identifier;
}
void Session::currency_transfer_impl(std::int32_t amount,CurrencyOperation operation) {
    const auto result=transfer_currency(player_.zorkmids,amount,operation);
    events_.push_back({result.money_sound?EventKind::currency_given:EventKind::currency_taken,player_.id,0,result.delta});
    player_.zorkmids=result.balance;
}
std::optional<std::uint64_t> Session::create_ground_loot(const ItemFactoryTables& tables,const ItemFactoryContext& context,
    std::string_view type,std::string_view subtype,int quantity,bool artifact,Position position) {
    auto next=*this;const auto result=next.create_ground_loot_impl(tables,context,type,subtype,quantity,artifact,position);*this=std::move(next);return result;
}
std::optional<std::uint64_t> Session::create_ground_loot_impl(const ItemFactoryTables& tables,const ItemFactoryContext& context,
    std::string_view type,std::string_view subtype,int quantity,bool artifact,Position position) {
    if (!definitions_) throw std::invalid_argument("loot creation needs item definitions");
    const auto identifier=next_item_instance();static const std::vector<AbilityDefinition> empty;
    auto item=make_loot_item(*definitions_,tables,context,type,subtype,quantity,artifact,identifier,random_,skills_,abilities_?*abilities_:empty,&factory_state_);
    if (!item) return std::nullopt;
    drop(*item,position);return identifier;
}
RoomLootCreation Session::create_room_loot(const RoomLootRecord& record,const ItemFactoryTables& tables,
    const ItemFactoryContext& context,Position origin) {
    auto next=*this;const auto plan=plan_room_population(RoomPopulationRecord{record},origin,next.random_.libc);RoomLootCreation result;
    if (plan.request==PopulationRequest::loot) {
        result.accepted=true;result.instance=next.create_ground_loot_impl(tables,context,plan.type,plan.subtype,plan.amount,plan.artifact,plan.position);
        if (result.instance) {next.ground_.back().item.room_local_id=record.placement.id;next.ground_.back().item.can_push=record.placement.can_push;}
    }
    *this=std::move(next);return result;
}
void Session::give_currency(std::int32_t amount) {auto next=*this;next.currency_transfer_impl(amount,CurrencyOperation::give);*this=std::move(next);}
RoomTrapCreation Session::create_room_trap(const RoomTrapRecord& record,const ItemFactoryTables& tables,
    const ItemFactoryContext& context,const TrapMap& map,Position origin) {
    static const std::vector<ItemDefinition> empty;auto next=*this;
    auto created=dredmor::create_room_trap(record,definitions_?*definitions_:empty,tables,context,map,next.next_item_instance(),next.random_,origin);
    RoomTrapCreation result{created.accepted,{}};
    if (created.object) {
        const auto position=created.object->position;
        if (map.width!=world_.width() || map.height!=world_.height() || position.x<0 || position.y<0 || position.x>=world_.width() || position.y>=world_.height() || next.traps_.size()>=10000)
            throw std::invalid_argument("invalid session trap position/count");
        result.instance=created.object->instance;next.traps_.push_back(std::move(*created.object));
    }
    *this=std::move(next);return result;
}
std::optional<std::uint64_t> Session::create_room_monster(const RoomMonsterRecord& record,
    const std::function<Actor(const RoomPopulationPlan&,GameRandom&,MonsterSpawnState&)>& constructor,Position origin) {
    auto next=*this;const auto plan=plan_room_population(RoomPopulationRecord{record},origin,next.random_.libc);
    if (plan.request==PopulationRequest::none) {random_.libc=next.random_.libc;return std::nullopt;}
    auto actor=constructor(plan,next.random_,next.monster_state_);actor.position=plan.position;
    actor.room_local_id=record.placement.id;actor.can_push=record.placement.can_push;actor.room_spawned=true;
    const auto result=next.spawn_enemy(std::move(actor));*this=std::move(next);return result;
}
bool Session::cast_spell(const SpellDefinition& spell,Position origin,Position target,std::uint64_t caster,bool allow_haywire) {
    auto next=*this;next.events_.clear();const auto result=next.cast_spell_impl(spell,origin,target,caster,allow_haywire);*this=std::move(next);return result;
}
bool Session::cast_spell_impl(const SpellDefinition& spell,Position origin,Position target,std::uint64_t caster,bool allow_haywire) {
    if (spell.type=="self") target=origin;
    else if (spell.type=="adjacent") {
        const auto dx=std::uint32_t(target.x)-std::uint32_t(origin.x),dy=std::uint32_t(target.y)-std::uint32_t(origin.y);
        const auto bits=dx*dx+dy*dy;std::int32_t distance;std::memcpy(&distance,&bits,4);if (distance>=2) return false;
    } else if (spell.type!="target" && spell.type!="targetfloor") throw std::invalid_argument("spell dispatch is not implemented: "+spell.type);
    if (target.x<0 || target.y<0 || target.x>=world_.width() || target.y>=world_.height()) return false;
    for (const auto& node:spell.source.children) if (node.tag!="effect" && node.tag!="anim" && node.tag!="impact" && node.tag!="description" && node.tag!="requirements")
        throw std::invalid_argument("spell child execution is not implemented: "+node.tag);
    for (const auto& attribute:spell.source.attributes) {
        auto key=attribute.first;for (auto& c:key) if (c>='A' && c<='Z') c+=32;
        if ((key=="mine" || key=="attack") && legacy_integer(attribute.second)) throw std::invalid_argument("spell mine/attack dispatch is not implemented");
    }
    Actor* source=nullptr;if (!caster || caster==player_.id) source=&player_;
    else for (auto& enemy:enemies_) if (enemy.id==caster) {source=&enemy;break;}
    if (!source) throw std::invalid_argument("spell caster not found");
    for (const auto& effect:spell.effects) {
        if ((effect.type!="damage" && effect.type!="heal") || effect.midas || effect.burn || effect.is_sleeping || effect.regeneration_graphic || !effect.taxa.empty() || !effect.source.children.empty())
            throw std::invalid_argument("spell effect execution is not implemented: "+effect.type);
        for (const auto& attribute:effect.source.attributes) {
            auto key=attribute.first;for (auto& c:key) if (c>='A' && c<='Z') c+=32;
            if (key.rfind("requirebuff",0)==0 && legacy_integer(attribute.second)) throw std::invalid_argument("spell buff gate execution is not implemented");
        }
        if (effect.primary_scale< -1 || effect.primary_scale>=6 || (effect.primary_scale==-1 && (effect.secondary_scale<0 || effect.secondary_scale>=24)))
            throw std::invalid_argument("spell scaling attribute outside range");
        if (effect.percent!=100 && random_.libc.next()/21474837>effect.percent) continue;
        Actor* victim=same(player_.position,target)?&player_:enemy_at(target);
        if (!victim || !victim->alive()) continue;
        if (effect.type=="heal") {
            const auto attribute=effect.primary_scale==-1?source->secondary[effect.secondary_scale]:source->primary[effect.primary_scale];
            const auto health=resolve_healing(victim->health,victim->secondary[0],effect.amount,effect.amount_scale,attribute);
            if (health<0) throw std::runtime_error("negative healing HP is outside current session save invariants");
            events_.push_back({EventKind::spell_heal,source->id,victim->id,health-victim->health});victim->health=health;continue;
        }
        if (!effect.affects_caster && source->id==victim->id) continue;
        SpellDamageInput input;input.haywire=source->secondary[5];input.resist=victim->secondary[11];input.armour=victim->secondary[10];
        input.scaling_attribute=effect.primary_scale==-1?source->secondary[effect.secondary_scale]:source->primary[effect.primary_scale];
        input.allow_haywire=allow_haywire || spell.type=="self" || spell.type=="adjacent";
        input.force_unresisted=effect.unblockable!=0;input.damage=effect.damage;input.scaling=effect.scaling;input.resistance=victim->resistance;
        const auto result=resolve_spell_damage(input,[&]{return random_.libc.next();});
        events_.push_back({EventKind::spell_damage,source->id,victim->id,result.total,result.haywire,result.resisted});
        if (result.total>0) victim->health=std::max(0,victim->health-result.total);
        if (!victim->alive()) {
            events_.push_back({EventKind::death,victim->id,source->id});
            if (source->id==player_.id && victim->id!=player_.id && progression_.gain(victim->experience_reward,experience_n_)) {
                player_.health=player_.secondary[0];player_.mana=player_.secondary[1];events_.push_back({EventKind::level_up,player_.id,0,progression_.level});
            }
        }
    }
    return true;
}
std::vector<std::size_t> Session::detect_traps() {
    std::vector<std::size_t> visible;
    for (std::size_t i=0;i<traps_.size();++i) {
        auto& trap=traps_[i];
        if (!trap.pending_destruction && definitions_ && detect_trap(trap,definitions_->at(trap.definition),player_.position,player_.secondary)) visible.push_back(i);
    }
    return visible;
}
void Session::activate_traps_impl(Actor& actor) {
    const bool monster=actor.id!=player_.id;
    if (monster && actor.monster_terrain!=0) return;
    for (auto& trap:traps_) {
        if (!actor.alive()) return;
        if (trap.pending_destruction || !same(trap.position,actor.position)) continue;
        const auto step=step_on_trap(trap,actor.secondary,random_.mt,monster);
        if (step.clicked) events_.push_back({EventKind::trap_click,actor.id,trap.instance});
        if (!step.triggered) continue;
        if (!spells_ || !definitions_) throw std::runtime_error("trap activation needs spell and item definitions");
        const auto& definition=definitions_->at(trap.definition);const SpellDefinition* spell=nullptr;
        for (const auto& candidate:*spells_) if (candidate.name==definition.trap->spell) {spell=&candidate;break;}
        const auto request=trigger_trap(trap,definition,actor.position,spell!=nullptr,monster);
        if (!request) {events_.push_back({EventKind::trap_spell_missing,actor.id,trap.instance});continue;}
        events_.push_back({EventKind::trap_triggered,actor.id,trap.instance});
        cast_spell_impl(*spell,request->origin,request->target,request->caster_is_target?actor.id:player_.id,request->monster_cast);
    }
}
void Session::take_currency(std::int32_t amount) {auto next=*this;next.currency_transfer_impl(amount,CurrencyOperation::take);*this=std::move(next);}
void Session::pickup_currency() {
    for (auto item=ground_.begin();item!=ground_.end();) {
        if (item->item.kind!=ItemKind::currency || !same(item->position,player_.position)) {++item;continue;}
        if (!item->item.currency_value) throw std::logic_error("currency item lacks value");
        currency_transfer_impl(*item->item.currency_value,CurrencyOperation::give);
        events_.push_back({EventKind::pickup,player_.id,item->item.instance,*item->item.currency_value});item=ground_.erase(item);
    }
}
void Session::attack(Actor& attacker,Actor& defender) {
    Actor* source=&attacker;Actor* victim=&defender;
    // This guard makes a pathological endless counter chain fail atomically;
    // it does not silently substitute a different combat result.
    for (int chain=0;chain<128;++chain) {
        CombatInput input;
        input.critical=source->secondary[4];input.melee_power=source->secondary[2];input.damage=source->damage;
        input.dodge=victim->secondary[6];input.counter=victim->secondary[8];input.block=victim->secondary[7];
        input.armour=victim->secondary[10];input.resistance=victim->resistance;
        const auto result=resolve_combat(input,[&]{return random_.libc.next();});
        if (result.outcome==CombatOutcome::countered) {
            events_.push_back({EventKind::counter,victim->id,source->id});std::swap(source,victim);continue;
        }
        if (result.outcome==CombatOutcome::dodged) { events_.push_back({EventKind::dodge,victim->id,source->id});return; }
        events_.push_back({EventKind::attack,source->id,victim->id,result.total,result.critical,result.blocked});
        if (result.total>0) victim->health=std::max(0,victim->health-result.total);
        if (!victim->alive()) {
            events_.push_back({EventKind::death,victim->id,source->id});
            if (source->id==player_.id && victim->id!=player_.id && progression_.gain(victim->experience_reward,experience_n_)) {
                player_.health=player_.secondary[0];player_.mana=player_.secondary[1];
                events_.push_back({EventKind::level_up,player_.id,0,progression_.level});
            }
        }
        return;
    }
    throw std::runtime_error("counter chain exceeded integration session limit");
}
std::optional<Position> Session::next_step(const Actor& enemy) const {
    const int width=world_.width(),height=world_.height();
    const auto index=[&](Position p){return p.y*width+p.x;};
    const int start=index(enemy.position),goal=index(player_.position);
    std::vector<int> previous(static_cast<std::size_t>(width)*height,-1);
    std::queue<Position> queue;queue.push(enemy.position);previous[start]=start;
    const std::array<Position,4> directions={Position{0,-1},Position{-1,0},Position{1,0},Position{0,1}};
    while (!queue.empty() && previous[goal]<0) {
        auto p=queue.front();queue.pop();
        for (auto delta:directions) {
            Position next{p.x+delta.x,p.y+delta.y};
            if (!passable(next) || previous[index(next)]>=0) continue;
            bool occupied=false;
            for (const auto& other:enemies_) if (other.id!=enemy.id && other.alive() && same(other.position,next)) {occupied=true;break;}
            if (occupied) continue;
            previous[index(next)]=index(p);queue.push(next);
        }
    }
    if (previous[goal]<0) return std::nullopt;
    auto step=goal;
    while (previous[step]!=start) step=previous[step];
    return Position{step%width,step/width};
}
void Session::enemy_turns() {
    for (auto& enemy:enemies_) if (enemy.alive() && player_.alive()) {
        const auto dx=std::abs(enemy.position.x-player_.position.x),dy=std::abs(enemy.position.y-player_.position.y);
        if (dx+dy==1) attack(enemy,player_);
        else if (auto next=next_step(enemy)) { enemy.position=*next;events_.push_back({EventKind::moved,enemy.id});activate_traps_impl(enemy); }
    }
}
void Session::step_impl(Action action) {
    events_.clear();if (!player_.alive()) return;
    if (world_.turn()==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("turn counter exhausted");
    const auto destination=target(player_.position,action);
    if (action!=Action::wait && world_.passable(destination)) for (std::size_t index=0;index<breakables_.size();++index) {
        const auto& object=breakables_[index].object;
        if (object.blocks_movement && same(object.position,destination) && hit_breakable_impl(index)) {
            world_.step(Action::wait);enemy_turns();return;
        }
    }
    if (action!=Action::wait && !passable(destination)) {events_.push_back({EventKind::blocked,player_.id});return;}
    if (action!=Action::wait) if (auto* enemy=enemy_at(destination)) {
        attack(player_,*enemy);world_.step(Action::wait);enemy_turns();return;
    }
    const auto outcome=world_.step(action);
    if (outcome==Outcome::blocked) { events_.push_back({EventKind::blocked,player_.id});return; }
    player_.position=world_.player();
    if (outcome==Outcome::moved) events_.push_back({EventKind::moved,player_.id});
    if (outcome==Outcome::moved) pickup_currency();
    if (outcome==Outcome::moved) activate_traps_impl(player_);
    enemy_turns();
}
void Session::step(Action action) { auto next=*this;next.step_impl(action);*this=std::move(next); }
void Session::pickup() {auto next=*this;next.pickup_impl();*this=std::move(next);}
void Session::pickup_impl() {
    events_.clear();if (!player_.alive()) return;
    pickup_currency();
    for (auto item=ground_.begin();item!=ground_.end();) {
        if (!same(item->position,player_.position)) {++item;continue;}
        if (inventory_.insert(item->item).outcome==InsertOutcome::full) {
            events_.push_back({EventKind::inventory_full,player_.id,item->item.instance});++item;
        } else {
            events_.push_back({EventKind::pickup,player_.id,item->item.instance,item->item.quantity});item=ground_.erase(item);
        }
    }
}
void Session::equipment_profile(ClassRanks ranks,std::shared_ptr<const std::vector<ItemDefinition>> definitions) {
    if (!definitions) throw std::invalid_argument("equipment profile needs item definitions");
    if (definitions_) throw std::invalid_argument("equipment profile is already configured");
    ranks_=ranks;definitions_=std::move(definitions);rebuild_equipment();
}
void Session::rebuild_equipment() {
    if (!definitions_) return;
    static const std::vector<AbilityDefinition> empty;
    auto stats=equipment_.rebuild(ranks_,*definitions_,{},{},skills_,abilities_?*abilities_:empty);
    player_.primary=stats.primary;player_.secondary=stats.secondary;
    player_.health=std::min(player_.health,stats.secondary[0]);player_.mana=std::min(player_.mana,stats.secondary[1]);
    for (std::size_t i=0;i<16;++i) {
        player_.damage[i]=add(intrinsic_damage_[i],stats.damage_bonus[i]);
        player_.resistance[i]=add(intrinsic_resistance_[i],stats.resistance[i]);
    }
    for (const auto& slot:equipment_.slots()) if (slot && slot->kind==ItemKind::weapon) {
        const auto& definition=definitions_->at(slot->definition);
        if (definition.weapon_type==4) continue;
        for (std::size_t i=0;i<16;++i) {
            player_.damage[i]=add(player_.damage[i],definition.weapon_damage[i]);
            if (slot->artifact) player_.damage[i]=add(player_.damage[i],slot->artifact->weapon_damage[i]);
        }
    }
}
void Session::character_profile(ClassRanks ranks,std::shared_ptr<const std::vector<ItemDefinition>> definitions,
                                std::vector<SkillRank> skills,std::shared_ptr<const std::vector<AbilityDefinition>> abilities) {
    if (definitions_ || world_.turn()!=0 || !ground_.empty() || !enemies_.empty() || skills.size()!=7 || !abilities)
        throw std::invalid_argument("character profile requires a fresh session and seven selected skills");
    auto occupied=[](const auto& slot){return slot.has_value();};
    if (std::any_of(inventory_.bag().begin(),inventory_.bag().end(),occupied) ||
        std::any_of(inventory_.belt().begin(),inventory_.belt().end(),occupied))
        throw std::invalid_argument("character inventory must start empty");
    for (std::size_t i=0;i<skills.size();++i) {
        if (skills[i].id<0 || skills[i].rank<1) throw std::invalid_argument("invalid selected skill");
        for (std::size_t j=0;j<i;++j) if (skills[j].id==skills[i].id) throw std::invalid_argument("duplicate selected skill");
    }
    auto next=*this;next.equipment_profile(ranks,std::move(definitions));
    next.skills_=std::move(skills);next.abilities_=std::move(abilities);next.rebuild_equipment();
    next.player_.health=next.player_.secondary[0];next.player_.mana=next.player_.secondary[1];
    *this=std::move(next);
}
bool Session::equip(std::uint64_t instance,EquipmentSlot slot) {
    if (!definitions_ || !player_.alive()) return false;
    auto next=*this;
    // Higher-level original dual-wield attack/ability semantics are not recovered
    // yet. Keep this integration path to one melee weapon until they are.
    const auto* incoming=inventory_.find(instance);
    if (incoming && incoming->kind==ItemKind::weapon && (slot==EquipmentSlot::hand_left || slot==EquipmentSlot::hand_right)) {
        const auto other=slot==EquipmentSlot::hand_left?2:1;
        if (equipment_.slots()[other] && equipment_.slots()[other]->kind==ItemKind::weapon) return false;
    }
    if (!equip_from_inventory(next.inventory_,next.equipment_,instance,slot,*definitions_)) return false;
    const auto index=static_cast<std::size_t>(slot);
    if (equipment_.slots()[index] && equipment_.slots()[index]->artifact) artifact_resources(next.player_,*equipment_.slots()[index]->artifact,true);
    if (next.equipment_.slots()[index]->artifact) artifact_resources(next.player_,*next.equipment_.slots()[index]->artifact,false);
    next.rebuild_equipment();*this=std::move(next);return true;
}
bool Session::unequip(EquipmentSlot slot) {
    if (!definitions_ || !player_.alive()) return false;
    auto next=*this;
    if (!unequip_to_inventory(next.inventory_,next.equipment_,slot)) return false;
    const auto& previous=equipment_.slots()[static_cast<std::size_t>(slot)];
    if (previous && previous->artifact) artifact_resources(next.player_,*previous->artifact,true);
    next.rebuild_equipment();*this=std::move(next);return true;
}
std::string Session::snapshot() const {
    std::ostringstream out;
    out<<"{\"turn\":"<<world_.turn()<<",\"player\":{\"id\":"<<player_.id<<",\"name\":"<<quoted(player_.name)
       <<",\"x\":"<<player_.position.x<<",\"y\":"<<player_.position.y<<",\"hp\":"<<player_.health
       <<",\"mana\":"<<player_.mana<<",\"zorkmids\":"<<player_.zorkmids<<",\"level\":"<<progression_.level<<",\"xp\":"<<progression_.experience<<"},\"enemies\":[";
    bool first=true;
    for (const auto& enemy:enemies_) {
        if (!first) out<<',';
        first=false;
        out<<"{\"id\":"<<enemy.id<<",\"name\":"<<quoted(enemy.name)<<",\"x\":"<<enemy.position.x
           <<",\"y\":"<<enemy.position.y<<",\"hp\":"<<enemy.health<<'}';
    }
    out<<"],\"inventory\":[";first=true;
    for (const auto* slots:{&inventory_.bag()[0],&inventory_.belt()[0]}) {
        const auto count=slots==&inventory_.bag()[0]?48:9;
        for (int i=0;i<count;++i) if (slots[i]) {
            if (!first) out<<',';
            first=false;
            out<<"{\"id\":"<<slots[i]->instance<<",\"quantity\":"<<slots[i]->quantity<<'}';
        }
    }
    out<<"],\"objects\":[";first=true;
    for (const auto& object:custom_objects_) {
        if (!first) out<<',';
        first=false;
        out<<"{\"kind\":"<<unsigned(object.kind)<<",\"name\":"<<quoted(object.name)<<",\"id\":"<<quoted(object.room_local_id)
            <<",\"x\":"<<object.position.x<<",\"y\":"<<object.position.y<<",\"movement\":"<<object.blocks_movement<<",\"vision\":"<<object.blocks_vision<<'}';
    }
    out<<"]}";return out.str();
}
}
