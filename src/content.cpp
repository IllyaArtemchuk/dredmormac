#include "dredmor/content.hpp"
#include "dredmor/monster_sprites.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/legacy_number.hpp"
#include "dredmor/room_records.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

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
Actor monster_actor(const MonsterDefinition& definition,Position position,const MonsterStatInput& input) {
    const auto stats=derive_monster(input);
    Actor actor;actor.name=definition.name;actor.position=position;actor.primary=stats.primary;actor.secondary=stats.secondary;
    actor.health=stats.secondary[0];actor.mana=stats.secondary[1];actor.damage=definition.damage;actor.resistance=definition.resistance;
    actor.experience_reward=definition.experience;actor.sprite=definition.idle_sprites[3];actor.monster_terrain=definition.terrain;actor.stat_profile=input;return actor;
}
void monster_appearance(Actor& actor,const MonsterDefinition& definition,DarwinRandom& random) {
    if (!definition.palette_tint) {actor.sprite_palette=definition.palette;return;}
    MonsterSpriteState sprites;
    actor.sprite_tint=tint_monster_sprites(definition,sprites,definition.palette_tint,random,
        [](const auto&,auto) {return MonsterSpriteHandle(1);});
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
        if (tag=="customblocker" || tag=="customengraving" || tag=="custombreakable" || tag=="pedestal" || tag=="element" || tag=="clock" ||
            tag=="monster" || tag=="trap" || tag=="loot" || tag=="horde" || tag=="lever" || tag=="magicblocker") result.push_back(read_room_record(node,markers,numeric));
    }
    return result;
}
RoomScriptProgram RoomDefinition::script_program(const std::function<ScriptSpellBinding(std::string_view)>& spells) const {
    RoomScriptProgram result;RoomActionReaders readers;readers.spell=spells;
    readers.record=[&](const XmlNode& node,const RoomMarkers& markers,RoomStamp& numeric) {
        if (node.tag!=ascii_lower(node.tag)) throw std::invalid_argument("unknown case-sensitive created room record: "+node.tag);
        result.created_records.push_back(read_room_record(node,markers,numeric));return std::uint64_t(result.created_records.size());
    };
    result.scripts=scripts(readers);return result;
}
RoomScriptProgram GameContent::room_script_program(std::string_view name) const {
    return room_script_program(std::size_t(&room(name)-rooms_.data()));
}
RoomScriptProgram GameContent::room_script_program(std::size_t room_index) const {
    return rooms_.at(room_index).script_program([&](std::string_view spell_name) {
        const auto* spell=find_spell(spell_name);if (!spell) throw std::invalid_argument("unknown room script spell: "+std::string(spell_name));
        return ScriptSpellBinding{std::uint64_t(spell-spells_.data())+1,spell_cast_sound(*spell)};
    });
}
std::optional<SessionRoomObjectRef> GameContent::create_script_record(Session& session,const RoomRecord& record,
    const TrapMap& map,Position origin,Difficulty difficulty,std::size_t level_set) const {
    return std::visit([&](const auto& definition)->std::optional<SessionRoomObjectRef> {
        using T=std::decay_t<decltype(definition)>;
        if constexpr (std::is_same_v<T,RoomMonsterRecord>) {
            const auto depth=session.depth_context();
            if (auto id=session.create_room_monster(definition,[&](const auto& plan,auto& random,auto& counts) {
                return create_room_monster(plan,{depth},difficulty,random,counts);
            },origin)) return SessionRoomObjectRef{SessionRoomObjectKind::monster,*id};
        } else if constexpr (std::is_same_v<T,RoomLootRecord>) {
            if (auto result=session.create_room_loot(definition,item_factory_tables(),loot_context(session.depth_context(),level_set,difficulty),origin);result.instance)
                return SessionRoomObjectRef{SessionRoomObjectKind::item,*result.instance};
        } else if constexpr (std::is_same_v<T,RoomTrapRecord>) {
            if (auto result=session.create_room_trap(definition,item_factory_tables(),loot_context(session.depth_context(),level_set,difficulty),map,origin);result.instance)
                return SessionRoomObjectRef{SessionRoomObjectKind::trap,*result.instance};
        } else throw std::invalid_argument("room script record requires another constructor");
        return std::nullopt;
    },record);
}
std::optional<HordeResult> GameContent::queue_script_horde(Session& session,const RoomHordeRecord& record,const DungeonLayout& layout,
    const std::vector<HordeRoomBounds>& rooms,Position origin,Difficulty difficulty) const {
    const auto depth=session.depth_context().depth;
    return session.queue_room_horde(record,layout,rooms,[&](const auto& plan,const auto& world,const auto& counts,auto& random,auto first) {
        HordeOptions options;options.position=plan.position;options.minimum=plan.minimum;options.additional=plan.additional;
        options.depth=depth;if (!plan.name.empty()) options.named=plan.name;
        return create_horde(options,world,difficulty,depth,counts,random,first);
    },origin);
}
RoomStamp RoomDefinition::numeric_template() const {
    const auto width=legacy_attribute(source,"width"),height=legacy_attribute(source,"height");
    if (!width || !height || *width<1 || *height<1 || *width>512 || *height>512 || rows.size()>std::size_t(*height))
        throw std::runtime_error("invalid declared room dimensions: "+name);
    RoomStamp result;result.width=*width;result.height=*height;
    result.shop_markers=read_shop_markers(rows,*width,*height);
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
        if (tag=="condition" && (value("condition_type")=="at" || value("condition_type")=="type_at")) prefix="";
        if (tag=="action") {
            const auto type=value("action_type");
            if (type=="create" || type=="move") prefix="";
            if (type=="spell") {
                auto attributes=node.attributes;(void)take_room_point(attributes,"source",markers,result);prefix="target";
            }
        }
        if (prefix) {auto attributes=node.attributes;(void)take_room_point(attributes,*prefix,markers,result);}
        if (tag=="script") for (const auto& child:node.children) points(child);
    };
    for (const auto& node:source.children) points(node);
    return result;
}
std::vector<RoomScriptDefinition> RoomDefinition::scripts(const RoomActionReaders& readers) const {
    auto numeric=numeric_template();RoomMarkers markers;
    for (std::size_t y=0;y<rows.size();++y) for (std::size_t x=0;x<std::min(rows[y].size(),std::size_t(numeric.width));++x)
        if (rows[y][x]>='1' && rows[y][x]<='9') markers[rows[y][x]]={int(x),int(y)};
    std::vector<RoomScriptDefinition> result;
    for (const auto& node:source.children) if (ascii_lower(node.tag)=="script") result.push_back(read_room_script(node,markers,numeric,readers));
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
            content.room_decorations_=read_room_decorations(database->root);
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
    std::vector<ManeuverTemplate> maneuvers;
    for (const auto& entry:content.catalog_.entries()) if (entry.node->tag=="template" && entry.database->layer=="base" && entry.database->source.filename()=="manTemplateDB.xml")
        maneuvers.push_back(read_maneuver_template(*entry.node));
    const auto maneuver_rules=maneuvers.empty()?std::shared_ptr<const std::vector<ManeuverTemplate>>{}:
        std::make_shared<const std::vector<ManeuverTemplate>>(std::move(maneuvers));
    for (const auto& entry:content.catalog_.entries()) if (entry.node->tag=="spell" && entry.database->source.filename()=="spellDB.xml") {
        auto spell=read_spell(*entry.node);spell.layer=entry.database->layer;spell.maneuvers=maneuver_rules;content.spells_.push_back(std::move(spell));
    }
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
const SpellDefinition* GameContent::find_spell(std::string_view name) const {
    // Original findSpell scans registration order and returns the first match.
    for (const auto& spell:spells_) if (spell.name==name) return &spell;
    return nullptr;
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
    if (definition.palette_tint==-1) throw std::invalid_argument("random monster tint requires create_monster and an RNG");
    auto actor=monster_actor(definition,position,tweaks_.monster_input(definition,difficulty,dungeon_level));
    DarwinRandom unused;monster_appearance(actor,definition,unused);return actor;
}
Actor GameContent::create_monster(std::string_view name,Position position,Difficulty difficulty,int dungeon_level,DarwinRandom& random) const {
    const auto index=find_registered_monster(monster_tables(),monsters_,name);
    if (!index) throw std::invalid_argument("registered monster not found: "+std::string(name));
    const auto& definition=monsters_.at(*index);auto input=tweaks_.monster_input(definition,difficulty,dungeon_level);
    input.ranks=monster_creation_ranks(input.ranks,tweaks_.integer(difficulty,"monster level adder"),random);
    auto actor=monster_actor(definition,position,input);monster_appearance(actor,definition,random);return actor;
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
    auto actor=monster_actor(definition,plan.position,input);monster_appearance(actor,definition,candidate.libc);
    actor.monster_flags=finish_random_monster_spawn(definition,next,candidate.mt);
    random=std::move(candidate);state=std::move(next);return actor;
}
HordePopulation GameContent::create_horde(HordeOptions options,const HordeWorld& world,Difficulty difficulty,
    std::int32_t constructor_depth,const MonsterSpawnState& counts,GameRandom& random,std::uint64_t first_identity) const {
    auto candidate=random;HordePopulation population;std::map<std::uint64_t,std::size_t> actors;
    auto next_identity=first_identity;
    auto actor_for=[&](const HordeMember& member)->Actor& {return population.actors.at(actors.at(member.identity));};
    HordeHooks hooks;
    hooks.tweak=[&](auto key) {return tweaks_.integer(difficulty,key);};
    hooks.can_spawn=[&](auto index,auto depth,bool horde) {return can_spawn_monster(monsters_.at(index),depth,counts,horde);};
    hooks.find_named=[&](auto name) {
        const auto index=find_registered_monster(monster_tables(),monsters_,name);
        if (!index) throw std::invalid_argument("registered horde monster not found: "+std::string(name));
        return *index;
    };
    hooks.construct=[&](auto index,auto depth,GameRandom& rng) {
        if (!next_identity || next_identity==std::numeric_limits<std::uint64_t>::max() || population.actors.size()>=512) throw std::runtime_error("horde actor identity/count budget exhausted");
        const auto& definition=monsters_.at(index);
        auto input=tweaks_.monster_input(definition,difficulty,&world.argument_level==&world.current_level?depth:constructor_depth);
        input.ranks=monster_creation_ranks(input.ranks,tweaks_.integer(difficulty,"monster level adder"),rng.libc);
        auto actor=monster_actor(definition,{-1,-1},input);actor.id=next_identity++;
        monster_appearance(actor,definition,rng.libc);
        HordeMember member;member.identity=actor.id;member.primary_percent=input.primary_percent;
        actors.emplace(actor.id,population.actors.size());population.actors.push_back(std::move(actor));return member;
    };
    hooks.monster_name=[&](auto& rng) {return generate_monster_name(artifact_words_,rng);};
    hooks.platoon_name=[&](auto& rng) {return generate_platoon_name(artifact_words_,rng);};
    hooks.tint=[&](auto& member,auto angle,auto& rng) {
        auto& actor=actor_for(member);MonsterSpriteState sprites;
        actor.sprite_tint=tint_monster_sprites(monsters_.at(member.definition),sprites,angle,rng.libc,
            [](const auto&,auto) {return MonsterSpriteHandle(1);});actor.sprite_palette.clear();
    };
    hooks.rebuild=[&](auto& member,auto&) {
        auto& actor=actor_for(member);actor.stat_profile->primary_percent=member.primary_percent;
        const auto stats=derive_monster(*actor.stat_profile);actor.primary=stats.primary;actor.secondary=stats.secondary;
        // RebuildAttributes changes maxima, preserving constructor HP/MP.
    };
    population.result=spawn_horde(monster_tables(),monsters_,std::move(options),world,candidate,hooks);
    random=std::move(candidate);return population;
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
