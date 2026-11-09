#include "dredmor/content.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/legacy_number.hpp"
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
}
RoomStamp RoomDefinition::numeric_template() const {
    const auto width=legacy_attribute(source,"width"),height=legacy_attribute(source,"height");
    if (!width || !height || *width<1 || *height<1 || *width>512 || *height>512 || rows.size()>std::size_t(*height))
        throw std::runtime_error("invalid declared room dimensions: "+name);
    RoomStamp result;result.width=*width;result.height=*height;
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
            // The original records digit coordinates without changing the
            // zero-initialized cell. Later scripts/content establish that tile.
            if (symbol<'1' || symbol>'9') throw std::runtime_error("unknown numeric room symbol: "+name);
        }
    }
    return result;
}
World RoomDefinition::instantiate() const {
    if (rows.empty() || rows.size()>512 || rows[0].empty() || rows[0].size()>512) throw std::runtime_error("invalid room size");
    std::set<char> floor={'.','^','@','s'},blocked={'#','!','X','P','W','L','I','G','d','D','S','i',' '};
    std::vector<Position> coordinate_blockers;
    std::function<void(const XmlNode&)> annotations=[&](const XmlNode& node) {
        const auto at=node.attribute("at");
        const bool blocker=node.tag=="customblocker" || node.tag=="custombreakable" || node.tag=="pedestal" || node.tag=="element";
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
    collision[spawn.y][spawn.x]='@';std::string text;
    for (const auto& row:collision) text+=row+'\n';
    std::istringstream input(text);return World::read(input);
}
GameContent GameContent::load(const std::filesystem::path& root,bool expansions) {
    GameContent content;content.catalog_=Catalog::load(root,expansions);
    bool found_tweaks=false;
    for (const auto& database:content.catalog_.databases()) {
        if (database->layer=="base" && database->source.filename()=="tweakDB.xml") {content.tweaks_=Tweaks::read(database->root);found_tweaks=true;}
        for (const auto& node:database->root.children) {
            if (database->source.filename()=="skillDB.xml" && node.tag=="ability") content.abilities_.push_back(read_ability(node));
            if (database->source.filename()=="itemDB.xml" && node.tag=="item") content.items_.push_back(read_item(node));
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
    if (!found_tweaks) throw std::runtime_error("base tweakDB.xml is required");
    content.monsters_=load_monsters(content.catalog_);return content;
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
    Actor actor;actor.name=definition.name;actor.position=position;actor.primary=stats.primary;actor.secondary=stats.secondary;
    actor.health=stats.secondary[0];actor.mana=stats.secondary[1];actor.damage=definition.damage;actor.resistance=definition.resistance;
    actor.experience_reward=definition.experience;actor.sprite=definition.idle_sprites[3];return actor;
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
