#include "dredmor/session.hpp"
#include <algorithm>
#include <array>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cstring>

namespace dredmor {
namespace {
bool same(Position a,Position b) { return a.x==b.x && a.y==b.y; }
std::int32_t add(std::int32_t a,std::int32_t b) {
    const std::uint32_t bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;
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
    return true;
}
std::optional<std::size_t> Session::create_custom_object(const CustomRoomDefinition& definition,Position origin) {
    auto candidate=random_.libc;auto object=create_custom_room_object(definition,origin,candidate);
    if (!object) {random_.libc=candidate;return std::nullopt;}
    if (custom_objects_.size()>=10000) throw std::runtime_error("session custom object limit exceeded");
    if (object->position.x<0 || object->position.y<0 || object->position.x>=world_.width() || object->position.y>=world_.height())
        throw std::invalid_argument("custom object outside session world");
    custom_objects_.push_back(std::move(*object));random_.libc=candidate;return custom_objects_.size()-1;
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
        (item.charges && (item.kind!=ItemKind::wand || *item.charges<0)))
        throw std::invalid_argument("invalid ground item");
    if (definitions_ && (item.definition>=definitions_->size() || definitions_->at(item.definition).kind!=item.kind))
        throw std::invalid_argument("ground item definition mismatch");
    for (const auto& slot:equipment_.slots()) if (slot && slot->instance==item.instance) throw std::invalid_argument("ground item is already equipped");
    for (const auto& existing:ground_) if (existing.item.instance==item.instance) throw std::invalid_argument("duplicate ground item");
    ground_.push_back({item,position});
}
std::uint64_t Session::create_ground_item(std::uint32_t definition,Position position,std::int32_t quantity) {
    if (!definitions_ || definition>=definitions_->size() || quantity<1) throw std::invalid_argument("invalid ground item creation");
    std::uint64_t identifier=1;
    auto observe=[&](const InventoryItem& item) {
        if (item.instance==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("item IDs exhausted");
        identifier=std::max(identifier,item.instance+1);
    };
    for (const auto& slot:inventory_.bag()) if (slot) observe(*slot);
    for (const auto& slot:inventory_.belt()) if (slot) observe(*slot);
    for (const auto& slot:equipment_.slots()) if (slot) observe(*slot);
    for (const auto& ground:ground_) observe(ground.item);
    auto next=*this;
    auto item=make_inventory_item(definitions_->at(definition),definition,identifier,next.random_.libc);item.quantity=quantity;
    next.drop(item,position);*this=std::move(next);return identifier;
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
        else if (auto next=next_step(enemy)) { enemy.position=*next;events_.push_back({EventKind::moved,enemy.id}); }
    }
}
void Session::step_impl(Action action) {
    events_.clear();if (!player_.alive()) return;
    if (world_.turn()==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("turn counter exhausted");
    const auto destination=target(player_.position,action);
    if (action!=Action::wait && !passable(destination)) {events_.push_back({EventKind::blocked,player_.id});return;}
    if (action!=Action::wait) if (auto* enemy=enemy_at(destination)) {
        attack(player_,*enemy);world_.step(Action::wait);enemy_turns();return;
    }
    const auto outcome=world_.step(action);
    if (outcome==Outcome::blocked) { events_.push_back({EventKind::blocked,player_.id});return; }
    player_.position=world_.player();
    if (outcome==Outcome::moved) events_.push_back({EventKind::moved,player_.id});
    enemy_turns();
}
void Session::step(Action action) { auto next=*this;next.step_impl(action);*this=std::move(next); }
void Session::pickup() {
    events_.clear();if (!player_.alive()) return;
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
        for (std::size_t i=0;i<16;++i) player_.damage[i]=add(player_.damage[i],definition.weapon_damage[i]);
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
    next.rebuild_equipment();*this=std::move(next);return true;
}
bool Session::unequip(EquipmentSlot slot) {
    if (!definitions_ || !player_.alive()) return false;
    auto next=*this;
    if (!unequip_to_inventory(next.inventory_,next.equipment_,slot)) return false;
    next.rebuild_equipment();*this=std::move(next);return true;
}
std::string Session::snapshot() const {
    std::ostringstream out;
    out<<"{\"turn\":"<<world_.turn()<<",\"player\":{\"id\":"<<player_.id<<",\"name\":"<<quoted(player_.name)
       <<",\"x\":"<<player_.position.x<<",\"y\":"<<player_.position.y<<",\"hp\":"<<player_.health
       <<",\"mana\":"<<player_.mana<<",\"level\":"<<progression_.level<<",\"xp\":"<<progression_.experience<<"},\"enemies\":[";
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
