#include "dredmor/monsters.hpp"
#include <charconv>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <cstring>
#include <stdexcept>

namespace dredmor {
std::optional<std::uint32_t> select_room_monster(const MonsterTables& tables,const std::vector<MonsterDefinition>& definitions,
    MonsterSpawnContext context,int boost_chance,const MonsterSpawnState& state,GameRandom& random,std::size_t budget) {
    auto candidate=random;const int level=select_monster_level(context.depth,boost_chance,context.expansion,candidate.mt);
    if (level<0 || level>=15 || context.level_set>=4) throw std::invalid_argument("random monster table index outside range");
    const auto& pool=tables.levels[level][context.level_set];
    if (pool.empty()) {random=std::move(candidate);return std::nullopt;}
    if (pool.size()>0x7ffffffeU) throw std::invalid_argument("random monster pool too large");
    for (std::size_t attempt=0;attempt<budget;++attempt) {
        const auto index=pool[candidate.libc.bounded(static_cast<std::uint32_t>(pool.size()))];const auto& definition=definitions.at(index);
        const auto found=state.counts.find(definition.name);const auto count=found==state.counts.end()?0:found->second;
        const std::uint32_t bits=std::uint32_t(count)+1U;std::int32_t next;std::memcpy(&next,&bits,4);
        if ((definition.max_spawns<next || context.depth.depth==-1) && definition.max_spawns>=1) continue;
        random=std::move(candidate);return index;
    }
    throw std::runtime_error("random monster selection budget exhausted");
}
std::uint32_t finish_random_monster_spawn(const MonsterDefinition& definition,MonsterSpawnState& state,Random& random) {
    auto& count=state.counts[definition.name];const auto bits=std::uint32_t(count)+1U;std::memcpy(&count,&bits,4);
    std::uint32_t flags=0;if (random.bounded(5)==0) flags|=8;if (random.bounded(10)==0) flags|=2;return flags;
}
void register_monster(MonsterTables& tables,std::uint32_t index,const MonsterDefinition& definition,std::uint32_t level_sets) {
    if ((level_sets&15) && !definition.special && (definition.level<0 || definition.level>=15)) throw std::invalid_argument("monster level outside original table range");
    for (std::size_t set=0;set<4;++set) if (level_sets&(1U<<set)) {
        tables.named[set].push_back(index);
        if (!definition.special) tables.levels[definition.level][set].push_back(index);
    }
    if (definition.diggle_hell) tables.diggle_hell.push_back(index);
}
MonsterTables build_base_monster_tables(const Catalog& catalog,const std::vector<MonsterDefinition>& definitions) {
    MonsterTables tables;std::map<const XmlNode*,std::uint32_t> indices;std::size_t index=0;
    for (const auto& entry:catalog.entries()) if (entry.node->tag=="monster") {
        if (index>=definitions.size() || entry.database->layer!="base") throw std::invalid_argument("base monster table content mismatch");
        indices.emplace(entry.node,static_cast<std::uint32_t>(index++));
    }
    if (index!=definitions.size()) throw std::invalid_argument("base monster table definition count mismatch");
    std::function<void(const XmlNode&)> close=[&](const XmlNode& node) {
        for (const auto& child:node.children) if (child.tag=="monster") close(child);
        const auto index=indices.at(&node);register_monster(tables,index,definitions[index],7);
    };
    for (const auto& entry:catalog.entries()) if (entry.node->tag=="monster" && !entry.parent_variant) close(*entry.node);
    return tables;
}
std::optional<std::uint32_t> find_registered_monster(const MonsterTables& tables,const std::vector<MonsterDefinition>& definitions,std::string_view display_name) {
    for (const auto& pool:tables.named) for (auto index:pool) if (definitions.at(index).display_name==display_name) return index;
    return std::nullopt;
}
ClassRanks monster_creation_ranks(ClassRanks ranks,int level_adder,DarwinRandom& random) {
    const auto steps=level_adder<0?-std::int64_t(level_adder):std::int64_t(level_adder);
    if (steps>1000000) throw std::invalid_argument("monster level adjustment exceeds execution budget");
    for (std::int64_t i=0;i<steps;++i) {
        const auto index=(std::uint64_t(static_cast<std::uint32_t>(random.next()))*6)>>32;
        const std::uint32_t bits=std::uint32_t(ranks[index])+(level_adder<0?0xffffffffU:1U);
        std::memcpy(&ranks[index],&bits,4);
    }
    if (level_adder<0) for (auto& rank:ranks) rank=std::max(0,rank);
    return ranks;
}
namespace {
std::string lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return value;
}
std::optional<std::string> attribute(const XmlNode& node,std::string_view key) {
    for (const auto& pair:node.attributes) if (lower(pair.first)==key) return pair.second;
    return std::nullopt;
}
int integer(const XmlNode& node,std::string_view key,int fallback=0) {
    auto value=attribute(node,key);if (!value) return fallback;
    int result;auto parsed=std::from_chars(value->data(),value->data()+value->size(),result);
    if (parsed.ec!=std::errc() || parsed.ptr!=value->data()+value->size()) throw std::runtime_error("invalid monster/tweak integer");
    return result;
}
void damage(const XmlNode& node,Damage& values) {
    for (std::size_t i=0;i<16;++i) if (attribute(node,damage_names[i])) values[i]=integer(node,damage_names[i]);
}
}
MonsterDefinition read_monster(const XmlNode& node,const MonsterDefinition* parent) {
    if (lower(node.tag)!="monster") throw std::runtime_error("expected monster definition");
    MonsterDefinition result=parent?*parent:MonsterDefinition{};
    result.parent=parent?parent->name:"";
    result.name=attribute(node,"name").value_or("");
    if (result.name.empty()) throw std::runtime_error("monster name missing");
    result.display_name=attribute(node,"displayname").value_or("");if (result.display_name.empty()) result.display_name=result.name;
    result.special=integer(node,"special",result.special);result.diggle_hell=integer(node,"digglehell",result.diggle_hell)!=0;
    result.max_spawns=integer(node,"maxspawns",result.max_spawns);
    result.terrain=integer(node,"terrain",result.terrain);
    result.level=integer(node,"level",result.level);
    result.source_chain.push_back(node);
    for (const auto& child:node.children) {
        const auto tag=lower(child.tag);
        if (tag=="stats") {
            result.ranks[0]=integer(child,"numfig",result.ranks[0]);
            result.ranks[1]=integer(child,"numwiz",result.ranks[1]);
            result.ranks[2]=integer(child,"numrog",result.ranks[2]);
            result.experience=integer(child,"xpvalue",result.experience);
            result.armour_ac=integer(child,"ac",result.armour_ac);
        } else if (tag=="damage") damage(child,result.damage);
        else if (tag=="resistances") damage(child,result.resistance);
        else if (tag=="primarybuff" || tag=="secondarybuff") {
            const auto id=integer(child,"id"),amount=integer(child,"amount");
            if (id<0 || id>=(tag=="primarybuff"?6:24)) throw std::runtime_error("monster buff ID out of bounds");
            if (tag=="primarybuff") result.primary[id]=amount;else result.secondary[id]=amount;
        } else if (tag=="idlesprite") {
            const std::array<std::string_view,4> keys={"left","right","up","down"};
            for (std::size_t i=0;i<4;++i) if (auto value=attribute(child,keys[i])) result.idle_sprites[i]=*value;
        } else if (tag=="ai") {
            result.aggressiveness=integer(child,"aggressiveness",result.aggressiveness);
            result.span=integer(child,"span",result.span);
        }
    }
    return result;
}
std::vector<MonsterDefinition> load_monsters(const Catalog& catalog) {
    std::vector<MonsterDefinition> monsters;
    std::map<const XmlNode*,std::size_t> indexed;
    for (const auto& entry:catalog.entries()) if (entry.node->tag=="monster") {
        const MonsterDefinition* parent=nullptr;
        if (entry.parent_variant) parent=&monsters.at(indexed.at(entry.parent_variant));
        auto monster=read_monster(*entry.node,parent);monster.layer=entry.database->layer;
        indexed.emplace(entry.node,monsters.size());monsters.push_back(std::move(monster));
    }
    return monsters;
}
Tweaks Tweaks::read(const XmlNode& root) {
    Tweaks tweaks;
    std::function<void(const XmlNode&,int)> visit=[&](const XmlNode& node,int difficulty) {
        const auto tag=lower(node.tag);
        if (tag=="easy") difficulty=0;
        else if (tag=="medium") difficulty=1;
        else if (tag=="hard") difficulty=2;
        if (tag=="tweak") {
            const auto name=attribute(node,"name").value_or("");
            if (name.empty()) throw std::runtime_error("tweak name missing");
            Value value;value.integer=dredmor::integer(node,"ival");
            if (auto text=attribute(node,"fval")) {
                char* end=nullptr;value.real=std::strtof(text->c_str(),&end);
                if (end!=text->c_str()+text->size() || !std::isfinite(value.real)) throw std::runtime_error("invalid float tweak");
            }
            for (int d=0;d<3;++d) if (difficulty<0 || difficulty==d) tweaks.values_[d][name]=value;
        }
        for (const auto& child:node.children) visit(child,difficulty);
    };
    visit(root,-1);return tweaks;
}
int Tweaks::integer(Difficulty difficulty,std::string_view name) const {
    const auto& table=values_.at(static_cast<std::size_t>(difficulty));
    const auto found=table.find(name);return found==table.end()?0:found->second.integer;
}
float Tweaks::real(Difficulty difficulty,std::string_view name) const {
    const auto& table=values_.at(static_cast<std::size_t>(difficulty));
    const auto found=table.find(name);return found==table.end()?0:found->second.real;
}
MonsterStatInput Tweaks::monster_input(const MonsterDefinition& definition,Difficulty difficulty,int dungeon_level) const {
    MonsterStatInput input;
    input.ranks=definition.ranks;input.definition_primary=definition.primary;input.definition_secondary=definition.secondary;
    input.dungeon_level=dungeon_level;
    input.health_scale=real(difficulty,"monster HP global scale");
    input.health_percent_per_level=real(difficulty,"monster additional HP percent per level");
    // The original asks the integer table here, even though the supplied entry
    // uses fval. Preserve that distinction instead of converting automatically.
    input.health_per_level=integer(difficulty,"monster additional HP per level");
    return input;
}
}
