#include "dredmor/content.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/legacy_number.hpp"
#include "dredmor/room_records.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace dredmor {
namespace {
std::string ascii_lower(std::string value) {
    for (auto& c:value) if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return value;
}
std::optional<int> legacy_attribute(const XmlNode& node,std::string_view key) {
    for (const auto& attribute:node.attributes) if (ascii_lower(attribute.first)==key) return legacy_integer(attribute.second);
    return std::nullopt;
}
Actor monster_actor(const MonsterDefinition& definition,Position position,const MonsterStats& stats) {
    Actor actor;actor.name=definition.name;actor.position=position;actor.primary=stats.primary;actor.secondary=stats.secondary;
    actor.health=stats.secondary[0];actor.mana=stats.secondary[1];actor.damage=definition.damage;actor.resistance=definition.resistance;
    actor.experience_reward=definition.experience;actor.sprite=definition.idle_sprites[3];return actor;
}
}
std::vector<RoomElementDefinition> RoomDefinition::elements() const {
    auto numeric=numeric_template();RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(numeric.width));++x)
        if (rows[y][x]>='1' && rows[y][x]<='9') markers[rows[y][x]]={int(x),int(y)};
    std::vector<RoomElementDefinition> result;
    for (const auto& node:source.children) if (ascii_lower(node.tag)=="element") result.push_back(read_room_element(node,markers,numeric));
    return result;
}
std::vector<CustomRoomDefinition> RoomDefinition::custom_objects() const {
    auto numeric=numeric_template();RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(numeric.width));++x)
        if (rows[y][x]>='1' && rows[y][x]<='9') markers[rows[y][x]]={int(x),int(y)};
    std::vector<CustomRoomDefinition> result;
    for (const auto& node:source.children) if (ascii_lower(node.tag)=="customblocker" || ascii_lower(node.tag)=="customengraving")
        result.push_back(read_custom_room_object(node,markers,numeric));
    return result;
}
std::vector<RoomPopulationRecord> RoomDefinition::population() const {
    auto numeric=numeric_template();RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(numeric.width));++x)
        if (rows[y][x]>='1' && rows[y][x]<='9') markers[rows[y][x]]={int(x),int(y)};
    std::vector<RoomPopulationRecord> result;
    for (const auto& node:source.children) {
        const auto tag=ascii_lower(node.tag);
        if (tag=="monster" || tag=="trap" || tag=="loot" || tag=="horde") result.push_back(read_room_population(node,markers,numeric));
    }
    return result;
}
std::vector<RoomRecord> RoomDefinition::records() const {
    auto numeric=numeric_template();RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(numeric.width));++x)
        if (rows[y][x]>='1' && rows[y][x]<='9') markers[rows[y][x]]={int(x),int(y)};
    std::vector<RoomRecord> result;
    for (const auto& node:source.children) {
        const auto tag=ascii_lower(node.tag);
        if (tag=="customblocker" || tag=="customengraving") result.emplace_back(read_custom_room_object(node,markers,numeric));
        else if (tag=="custombreakable") result.emplace_back(read_breakable_room_object(node,markers,numeric));
        else if (tag=="pedestal") result.emplace_back(read_room_pedestal(node,markers,numeric));
        else if (tag=="element") result.emplace_back(read_room_element(node,markers,numeric));
        else if (tag=="monster" || tag=="trap" || tag=="loot" || tag=="horde")
            std::visit([&](auto&& record){result.emplace_back(std::move(record));},read_room_population(node,markers,numeric));
        else if (tag=="lever" || tag=="clock" || tag=="magicblocker") result.emplace_back(UnimplementedRoomRecord{tag,node});
    }
    return result;
}
RoomStamp RoomDefinition::numeric_template() const {
    const auto width=legacy_attribute(source,"width"),height=legacy_attribute(source,"height");
    if (!width || !height || *width<1 || *height<1 || *width>512 || *height>512 || rows.size()>std::size_t(*height))
        throw std::runtime_error("invalid declared room dimensions: "+name);
    RoomStamp result;result.width=*width;result.height=*height;
    for (const auto& attribute:source.attributes) if (ascii_lower(attribute.first)=="usefixedname") result.fixed_name=attribute.second;
    for (const auto& node:source.children) if (ascii_lower(node.tag)=="flags") {
        auto flag=[&](std::string_view name,bool& target){if (const auto value=legacy_attribute(node,name)) target=*value!=0;};
        auto integer=[&](std::string_view name,int& target){if (const auto value=legacy_attribute(node,name)) target=*value;};
        flag("shop",result.shop);flag("zoo",result.zoo);flag("quest",result.rules.quest);
        flag("special",result.rules.unique);flag("digglegod",result.rules.diggle_god);
        flag("notraps",result.rules.no_traps);flag("nomonsters",result.rules.no_monsters);
        flag("noblockers",result.rules.no_blockers);flag("notreasure",result.rules.no_treasure);
        integer("minlevel",result.rules.minimum_level);integer("maxlevel",result.rules.maximum_level);
        integer("roomset",result.rules.room_set);
    }
    result.cells.resize(std::size_t(*width)*(*height));
    RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(*width));++x) {
        auto& cell=result.cells[y*(*width)+x];
        switch (const auto symbol=rows[y][x]) {
        case ' ': break;
        case '#':cell.kind=2;break;
        case '!':cell.kind=2;cell.wall_marker=4;break;
        case '.':case 'S':case 'i':case 's':cell.kind=1;break;
        case '@':cell.kind=1;cell.wall_marker=2;break;
        case '^':cell.kind=1;cell.wall_marker=3;break;
        case 'P':cell.kind=2;cell.wall_marker=1;break;
        case 'D':cell.kind=6;break;
        case 'd':cell.kind=6;cell.door_orientation=1;break;
        case 'W':cell.kind=7;break;
        case 'L':cell.kind=8;break;
        case 'G':cell.kind=9;break;
        case 'I':cell.kind=10;break;
        case 'X':cell.kind=15;break;
        default:
            // A digit initially stays void. PointProperty::SetFrom changes it
            // to floor when a room object or script references it during load.
            if (symbol<'1' || symbol>'9') throw std::runtime_error("unknown numeric room symbol: "+name);
            markers[symbol]={int(x),int(y)};
        }
    }
    std::function<void(const XmlNode&)> points=[&](const XmlNode& node) {
        const auto tag=ascii_lower(node.tag);std::optional<std::string> prefix;
        auto value=[&](std::string_view key) {
            for (const auto& attribute:node.attributes) if (ascii_lower(attribute.first)==key) return attribute.second;
            return std::string{};
        };
        if (tag=="customblocker" || tag=="customengraving" || tag=="custombreakable" || tag=="trap" || tag=="monster" ||
            tag=="pedestal" || tag=="element" || tag=="loot" || tag=="horde" || tag=="lever" || tag=="clock" || tag=="magicblocker") prefix="";
        if (tag=="condition" && value("condition_type")=="at") prefix="";
        if (tag=="action") {
            const auto type=value("action_type");
            if (type=="create" || type=="move") prefix="";
            if (type=="spell") prefix="target";
        }
        if (prefix) {auto attributes=node.attributes;(void)take_room_point(attributes,*prefix,markers,result);}
        if (tag=="script") for (const auto& child:node.children) points(child);
    };
    for (const auto& node:source.children) points(node);
    return result;
}
World RoomDefinition::instantiate(bool custom_blockers) const {
    if (rows.empty() || rows.size()>512 || rows[0].empty() || rows[0].size()>512) throw std::runtime_error("invalid room size");
    const auto preferred=custom_blockers?Position{-1,-1}:instantiate(true).player();
    std::set<char> floor={'.','^','@','s'},blocked={'#','!','X','P','W','L','I','G','d','D','S','i',' '};
    std::vector<Position> coordinate_blockers;
    std::function<void(const XmlNode&)> annotations=[&](const XmlNode& node) {
        const auto at=node.attribute("at");
        const auto tag=ascii_lower(node.tag);
        const bool blocker=(custom_blockers && (tag=="customblocker" || tag=="custombreakable" || tag=="pedestal")) || tag=="element";
        if (at.size()==1) {if (blocker) blocked.insert(at[0]);else floor.insert(at[0]);}
        if (blocker && node.integer("x") && node.integer("y")) coordinate_blockers.push_back({*node.integer("x"),*node.integer("y")});
        for (const auto& child:node.children) annotations(child);
    };
    annotations(source);
    std::vector<std::string> collision;
    Position spawn{-1,-1};int best=std::numeric_limits<int>::max();
    for (std::size_t y=0;y<rows.size();++y) {
        if (rows[y].size()!=rows[0].size()) throw std::runtime_error("ragged room rows: "+name);
        std::string row;
        for (std::size_t x=0;x<rows[y].size();++x) {
            const char symbol=rows[y][x];
            if (!floor.count(symbol) && !blocked.count(symbol)) throw std::runtime_error("unsupported room symbol in "+name+": "+std::string(1,symbol));
            bool passable=floor.count(symbol) && !blocked.count(symbol);
            for (auto blocker:coordinate_blockers) if (blocker.x==int(x) && blocker.y==int(y)) passable=false;
            row+=passable?'.':'#';
            const auto distance=std::abs(int(x)-int(rows[0].size()/2))+std::abs(int(y)-int(rows.size()/2));
            if (passable && distance<best) {best=distance;spawn={int(x),int(y)};}
        }
        collision.push_back(std::move(row));
    }
    if (spawn.x<0) throw std::runtime_error("room has no passable preview spawn");
    if (!custom_blockers) spawn=preferred;
    collision[spawn.y][spawn.x]='@';std::string text;
    for (const auto& row:collision) text+=row+'\n';
    std::istringstream input(text);return World::read(input);
}
GameContent GameContent::load(const std::filesystem::path& root,bool expansions) {
    GameContent content;content.catalog_=Catalog::load(root,expansions);
    if (!expansions) content.item_factory_tables_.emplace();
    bool found_tweaks=false;
    for (const auto& database:content.catalog_.databases()) {
        if (database->layer=="base" && database->source.filename()=="text.xml") {
            content.room_name_words_=read_room_name_words(database->root);content.artifact_words_=read_artifact_words(database->root);
        }
        if (database->layer=="base" && database->source.filename()=="tweakDB.xml") {content.tweaks_=Tweaks::read(database->root);found_tweaks=true;}
        for (const auto& node:database->root.children) {
            if (database->source.filename()=="skillDB.xml" && node.tag=="ability") content.abilities_.push_back(read_ability(node));
            if (database->source.filename()=="itemDB.xml" && node.tag=="item") {
                auto item=read_item(node);
                if (content.item_factory_tables_) register_factory_item(*content.item_factory_tables_,static_cast<std::uint32_t>(content.items_.size()),item,7);
                content.items_.push_back(std::move(item));
            }
            if (database->source.filename()=="skillDB.xml" && node.tag=="skill") {
                const auto category=node.attribute("type");
                const auto value=category=="warrior"?0:category=="wizard"?1:category=="rogue"?2:-1;
                const auto id=node.integer("id");
                if (!id || *id<0 || value<0) throw std::runtime_error("invalid skill definition");
                SkillDefinition skill{*id,node.attribute("name"),value,node,{}};
                for (const auto& child:node.children) if (child.tag=="loadout")
                    skill.loadouts.push_back({child.attribute("type"),child.attribute("subtype"),
                        legacy_attribute(child,"amount").value_or(1),legacy_attribute(child,"always").value_or(0)!=0});
                for (const auto& child:node.children) if (child.tag=="flags")
                    if (const auto deprecated=child.integer("deprecated")) skill.deprecated=*deprecated!=0;
                content.skills_.push_back(std::move(skill));
            }
            if (database->source.filename()=="rooms.xml" && node.tag=="room") {
                RoomDefinition room;room.name=node.attribute("name");room.layer=database->layer;room.source=node;
                for (const auto& child:node.children) if (child.tag=="row") room.rows.push_back(child.attribute("text"));
                content.rooms_.push_back(std::move(room));
            }
        }
    }
    for (auto& ability:content.abilities_) if (ability.skill_name.empty()) {
        for (const auto& skill:content.skills_) if (skill.id==ability.skill_id) {ability.skill_name=skill.name;break;}
    }
    for (int subtype=0;subtype<2;++subtype) {
        ItemDefinition item;item.kind=ItemKind::misc;item.builtin_subtype=subtype;
        item.name=subtype==0?"Lockpick":"Spores";item.icon=subtype==0?"items/lockpick.png":"items/fungus_spores.png";
        content.items_.push_back(std::move(item));
    }
    ItemDefinition currency;currency.kind=ItemKind::currency;currency.builtin_subtype=2;
    currency.name="Zorkmids";currency.icon="items/cash0.png";content.items_.push_back(std::move(currency));
    if (!found_tweaks) throw std::runtime_error("base tweakDB.xml is required");
    content.monsters_=load_monsters(content.catalog_);
    if (!expansions) content.monster_tables_=build_base_monster_tables(content.catalog_,content.monsters_);
    return content;
}
const ItemFactoryTables& GameContent::item_factory_tables() const {
    if (!item_factory_tables_) throw std::runtime_error("expansion item registration is not implemented");
    return *item_factory_tables_;
}
const MonsterTables& GameContent::monster_tables() const {
    if (!monster_tables_) throw std::runtime_error("expansion monster registration is not implemented");
    return *monster_tables_;
}
ItemFactoryContext GameContent::loot_context(SpawnDepthContext depth,std::size_t level_set,Difficulty difficulty) const {
    ItemFactoryContext context;context.depth=depth;context.level_set=level_set;
    context.gold_per_level=tweaks_.integer(difficulty,"gold drop per level");context.trap_deviation=tweaks_.real(difficulty,"box muller std dev");
    context.artifact_words=&artifact_words_;return context;
}
const RoomDefinition& GameContent::room(std::string_view name) const {
    const RoomDefinition* result=nullptr;
    for (const auto& room:rooms_) if (room.name==name) {if (result) throw std::runtime_error("ambiguous room name");result=&room;}
    if (!result) throw std::runtime_error("room not found: "+std::string(name));
    return *result;
}
const MonsterDefinition& GameContent::monster(std::string_view name) const {
    const MonsterDefinition* result=nullptr;
    for (const auto& monster:monsters_) if (monster.name==name) {if (result) throw std::runtime_error("ambiguous monster name");result=&monster;}
    if (!result) throw std::runtime_error("monster not found: "+std::string(name));
    return *result;
}
Actor GameContent::make_monster(std::string_view name,Position position,Difficulty difficulty,int dungeon_level) const {
    const auto& definition=monster(name);
    auto stats=derive_monster(tweaks_.monster_input(definition,difficulty,dungeon_level));
    return monster_actor(definition,position,stats);
}
Actor GameContent::create_monster(std::string_view name,Position position,Difficulty difficulty,int dungeon_level,DarwinRandom& random) const {
    const auto index=find_registered_monster(monster_tables(),monsters_,name);
    if (!index) throw std::invalid_argument("registered monster not found: "+std::string(name));
    const auto& definition=monsters_.at(*index);auto input=tweaks_.monster_input(definition,difficulty,dungeon_level);
    input.ranks=monster_creation_ranks(input.ranks,tweaks_.integer(difficulty,"monster level adder"),random);
    return monster_actor(definition,position,derive_monster(input));
}
Actor GameContent::create_room_monster(const RoomPopulationPlan& plan,MonsterSpawnContext context,Difficulty difficulty,
    GameRandom& random,MonsterSpawnState& state) const {
    if (plan.request==PopulationRequest::named_monster) return create_monster(plan.name,plan.position,difficulty,context.depth.depth,random.libc);
    if (plan.request!=PopulationRequest::random_monster) throw std::invalid_argument("expected a room monster request");
    auto candidate=random;auto next=state;
    const auto index=select_room_monster(monster_tables(),monsters_,context,tweaks_.integer(difficulty,"monster boost chance"),next,candidate);
    if (!index) throw std::runtime_error("room monster level has no registered candidates");
    const auto& definition=monsters_.at(*index);auto input=tweaks_.monster_input(definition,difficulty,context.depth.depth);
    input.ranks=monster_creation_ranks(input.ranks,tweaks_.integer(difficulty,"monster level adder"),candidate.libc);
    auto actor=monster_actor(definition,plan.position,derive_monster(input));actor.monster_flags=finish_random_monster_spawn(definition,next,candidate.mt);
    random=std::move(candidate);state=std::move(next);return actor;
}
ClassRanks GameContent::starting_ranks(const std::vector<int>& skill_ids) const {
    if (skill_ids.size()!=7 || std::set<int>(skill_ids.begin(),skill_ids.end()).size()!=7) throw std::invalid_argument("select exactly seven distinct skills");
    ClassRanks ranks{};
    for (auto id:skill_ids) {
        const SkillDefinition* found=nullptr;
        for (const auto& skill:skills_) if (skill.id==id) {if (found) throw std::runtime_error("ambiguous skill ID");found=&skill;}
        if (!found) throw std::invalid_argument("selected skill ID not found");
        ++ranks[found->category];
    }
    return ranks;
}
std::vector<StartingLoadout> GameContent::starting_loadout(const std::vector<int>& skill_ids) const {
    const auto ranks=starting_ranks(skill_ids);std::vector<StartingLoadout> result;
    // BuildPlayerFromSkills first writes selected IDs in database order, then
    // visits their loadouts. User selection order does not reorder those grants.
    for (const auto& skill:skills_) if (std::find(skill_ids.begin(),skill_ids.end(),skill.id)!=skill_ids.end())
        for (const auto& item:skill.loadouts) if (loadout_allowed(ranks,skill.category,item.always))
            result.push_back({skill.id,item});
    return result;
}
}
