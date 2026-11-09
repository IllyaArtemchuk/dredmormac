#include "dredmor/session.hpp"
#include "dredmor/xml.hpp"
#include <charconv>
#include <functional>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cmath>

namespace dredmor {
namespace {
std::string escape(const std::string& value) {
    std::string result;
    for (unsigned char c:value) {
        switch (c) {
            case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;
            case '"':result+="&quot;";break;case '\'':result+="&apos;";break;
            case '\n':result+="&#10;";break;case '\r':result+="&#13;";break;case '\t':result+="&#9;";break;
            default:if (c<32) throw std::runtime_error("save contains unsupported control character");result+=char(c);
        }
    }
    return result;
}
const XmlNode& child(const XmlNode& node,std::string_view name) {
    const XmlNode* result=nullptr;
    for (const auto& candidate:node.children) if (candidate.tag==name) {
        if (result) throw std::runtime_error("duplicate save field: "+std::string(name));
        result=&candidate;
    }
    if (!result) throw std::runtime_error("save field missing: "+std::string(name));
    return *result;
}
std::string attribute(const XmlNode& node,std::string_view name) {
    auto found=node.attributes.find(name);if (found==node.attributes.end()) throw std::runtime_error("save attribute missing");return found->second;
}
template<class T> T number(const std::string& text) {
    T result;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if (parsed.ec!=std::errc() || parsed.ptr!=text.data()+text.size()) throw std::runtime_error("invalid save number");
    return result;
}
template<class T> T attribute_number(const XmlNode& node,std::string_view name) { return number<T>(attribute(node,name)); }
bool flag(const XmlNode& node,std::string_view name) {
    const auto value=attribute_number<unsigned>(node,name);
    if (value>1) throw std::runtime_error("invalid saved flag");
    return value!=0;
}
template<class T,std::size_t N> std::array<T,N> array(const XmlNode& node) {
    std::istringstream input(node.text);std::array<T,N> result;std::string token;
    for (auto& value:result) {if (!(input>>token)) throw std::runtime_error("save array too short");value=number<T>(token);}
    if (input>>token) throw std::runtime_error("save array too long");
    return result;
}
template<class T,std::size_t N> void write_array(std::ostream& out,std::string_view tag,const std::array<T,N>& values) {
    out<<'<'<<tag<<'>';for (auto value:values) out<<value<<' ';out<<"</"<<tag<<">\n";
}
std::uint64_t signature(const std::shared_ptr<const std::vector<ItemDefinition>>& definitions,bool legacy_types=false) {
    if (!definitions) return 0;
    std::uint64_t hash=14695981039346656037ull;
    auto add=[&](const std::string& text){for (unsigned char c:text) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    std::function<void(const XmlNode&)> node=[&](const XmlNode& n) {
        add(n.tag);add(n.text);for (const auto& a:n.attributes) {add(a.first);add(a.second);}
        for (const auto& c:n.children) node(c);
    };
    for (const auto& item:*definitions) {
        // Builtins are engine rules appended after external definitions. Keeping
        // them out of the external-content identity preserves earlier saves.
        if (item.builtin_subtype>=0) continue;
        auto kind=static_cast<unsigned>(item.kind);
        if (legacy_types && (kind==11 || kind==12)) kind=23-kind;
        add(item.name);add(std::to_string(kind));add(std::to_string(item.weapon_type));
        add(std::to_string(item.armour_type));add(std::to_string(item.armour_ac));
        for (auto n:item.modifiers.primary) add(std::to_string(n));
        for (auto n:item.modifiers.secondary) add(std::to_string(n));
        for (auto n:item.weapon_damage) add(std::to_string(n));
        for (auto n:item.modifiers.damage) add(std::to_string(n));
        for (auto n:item.modifiers.resistance) add(std::to_string(n));
        node(item.source);
    }
    return hash?hash:1;
}
std::uint64_t ability_signature(const std::shared_ptr<const std::vector<AbilityDefinition>>& definitions,bool currency_fields=true) {
    if (!definitions) return 0;
    std::uint64_t hash=14695981039346656037ull;
    auto add=[&](const std::string& text){for (unsigned char c:text) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    std::function<void(const XmlNode&)> node=[&](const XmlNode& n) {
        add(n.tag);add(n.text);for (const auto& a:n.attributes) {add(a.first);add(a.second);}
        for (const auto& c:n.children) node(c);
    };
    for (const auto& ability:*definitions) {
        add(ability.name);add(ability.skill_name);add(std::to_string(ability.skill_id));add(std::to_string(ability.level));
        add(std::to_string(ability.starting));add(std::to_string(ability.polymorphed_only));add(ability.polymorph_tag);
        for (auto n:ability.modifiers.primary) add(std::to_string(n));
        for (auto n:ability.modifiers.secondary) add(std::to_string(n));
        for (auto n:ability.modifiers.damage) add(std::to_string(n));
        for (auto n:ability.modifiers.resistance) add(std::to_string(n));
        if (currency_fields) {std::uint32_t bits;std::memcpy(&bits,&ability.currency_bonus,4);add(std::to_string(bits));}
        node(ability.source);
    }
    return hash?hash:1;
}
std::uint64_t spell_signature(const std::shared_ptr<const std::vector<SpellDefinition>>& definitions) {
    if (!definitions) return 0;
    std::uint64_t hash=14695981039346656037ull;
    auto add=[&](const std::string& text){for (unsigned char c:text) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    auto real=[&](float value){std::uint32_t bits;std::memcpy(&bits,&value,4);add(std::to_string(bits));};
    std::function<void(const XmlNode&)> node=[&](const XmlNode& n) {add(n.tag);add(n.text);for (const auto& a:n.attributes) {add(a.first);add(a.second);}for (const auto& c:n.children) node(c);};
    for (const auto& spell:*definitions) {
        add(spell.name);add(spell.type);node(spell.source);
        for (const auto& effect:spell.effects) {
            add(effect.type);add(effect.spell);add(effect.taxa);add(effect.buff_tag);node(effect.source);
            for (auto n:effect.damage) add(std::to_string(n));
            for (auto n:effect.scaling) real(n);
            for (std::int32_t n:{effect.amount,effect.percent,effect.turns,effect.primary_scale,effect.secondary_scale,effect.self,effect.affects_caster,
                    effect.unblockable,effect.permanent,effect.kickback,int(effect.resistable),int(effect.affects_corpses),int(effect.is_sleeping),int(effect.midas),int(effect.burn),int(effect.regeneration_graphic)}) add(std::to_string(n));
            real(effect.amount_scale);real(effect.floor_scale);
        }
    }
    return hash?hash:1;
}
std::uint64_t maneuver_signature(const std::shared_ptr<const std::vector<SpellDefinition>>& definitions) {
    if (!definitions) return 0;
    std::uint64_t hash=14695981039346656037ull;std::map<const std::vector<ManeuverTemplate>*,std::size_t> groups;
    auto add=[&](const std::string& text){for (unsigned char c:text) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    std::function<void(const XmlNode&)> node=[&](const XmlNode& n){add(n.tag);add(n.text);for (const auto& a:n.attributes) {add(a.first);add(a.second);}for (const auto& c:n.children) node(c);};
    for (const auto& spell:*definitions) if (spell.maneuvers) {
        add(spell.name);const auto inserted=groups.emplace(spell.maneuvers.get(),groups.size());add(std::to_string(inserted.first->second));
        if (inserted.second) {
            add(std::to_string(spell.maneuvers->size()));
            for (const auto& t:*spell.maneuvers) {
                add(std::to_string(t.id));add(std::to_string(t.width));add(std::to_string(t.height));add(std::to_string(t.affects_player));node(t.source);
                for (auto cell:t.cells) add(std::to_string(cell));
            }
        }
    }
    return groups.empty()?0:hash?hash:1;
}
std::uint64_t buffs_signature(const std::shared_ptr<const std::vector<SpellDefinition>>& definitions) {
    std::uint64_t hash=14695981039346656037ull;bool any=false;
    auto add=[&](const std::string& value){for (unsigned char c:value) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    if (definitions) for (const auto& spell:*definitions) {
        add(spell.name);add(std::to_string(spell.buffs.size()));
        for (const auto& buff:spell.buffs) {any=true;add(std::to_string(buff_signature(buff)));}
    }
    return any?(hash?hash:1):0;
}
void write_actor(std::ostream& out,std::string_view tag,const Actor& actor) {
    out<<'<'<<tag<<" id=\""<<actor.id<<"\" name=\""<<escape(actor.name)<<"\" sprite=\""<<escape(actor.sprite)
       <<"\" x=\""<<actor.position.x<<"\" y=\""<<actor.position.y<<"\" hp=\""<<actor.health
       <<"\" mana=\""<<actor.mana<<"\" reward=\""<<actor.experience_reward<<"\" zorkmids=\""<<actor.zorkmids<<"\"";
    if (!actor.room_local_id.empty() || actor.can_push!=1 || actor.room_spawned)
        out<<" room-id=\""<<escape(actor.room_local_id)<<"\" push=\""<<unsigned(actor.can_push)<<"\" room-spawned=\""<<actor.room_spawned<<"\"";
    if (actor.monster_flags) out<<" monster-flags=\""<<actor.monster_flags<<"\"";
    if (actor.monster_terrain) out<<" monster-terrain=\""<<actor.monster_terrain<<"\"";
    out<<">\n";
    write_array(out,"primary",actor.primary);write_array(out,"secondary",actor.secondary);
    write_array(out,"damage",actor.damage);write_array(out,"resistance",actor.resistance);
    out<<"</"<<tag<<">\n";
}
Actor read_actor(const XmlNode& node,bool currency_fields,bool room_fields) {
    Actor actor;actor.id=attribute_number<std::uint64_t>(node,"id");actor.name=attribute(node,"name");actor.sprite=attribute(node,"sprite");
    const bool has_room=node.attributes.count("room-id") || node.attributes.count("push") || node.attributes.count("room-spawned");
    if (has_room && !room_fields) throw std::runtime_error("actor room metadata requires save version seven");
    if (has_room) {
        actor.room_local_id=attribute(node,"room-id");actor.can_push=attribute_number<std::uint8_t>(node,"push");actor.room_spawned=flag(node,"room-spawned");
    }
    if (node.attributes.count("monster-flags")) {
        if (!room_fields) throw std::runtime_error("monster flags require save version seven");
        actor.monster_flags=attribute_number<std::uint32_t>(node,"monster-flags");
    }
    if (node.attributes.count("monster-terrain")) {
        if (!room_fields) throw std::runtime_error("monster terrain requires save version seven");
        actor.monster_terrain=attribute_number<int>(node,"monster-terrain");
    }
    actor.position={attribute_number<int>(node,"x"),attribute_number<int>(node,"y")};
    actor.health=attribute_number<std::int32_t>(node,"hp");actor.mana=attribute_number<std::int32_t>(node,"mana");
    actor.experience_reward=attribute_number<std::int32_t>(node,"reward");
    if (currency_fields) actor.zorkmids=attribute_number<std::int32_t>(node,"zorkmids");
    actor.primary=array<std::int32_t,6>(child(node,"primary"));actor.secondary=array<std::int32_t,24>(child(node,"secondary"));
    actor.damage=array<std::int32_t,16>(child(node,"damage"));actor.resistance=array<std::int32_t,16>(child(node,"resistance"));
    if (!actor.id || actor.id==std::numeric_limits<std::uint64_t>::max() || actor.health<0 || actor.health>actor.secondary[0] || actor.mana<0 || actor.mana>actor.secondary[1])
        throw std::runtime_error("invalid saved actor");
    return actor;
}
void write_item(std::ostream& out,const InventoryItem& item,const std::string& extra) {
    out<<"<item id=\""<<item.instance<<"\" kind=\""<<static_cast<std::uint32_t>(item.kind)<<"\" definition=\""<<item.definition
       <<"\" weapon=\""<<item.weapon_type<<"\" quantity=\""<<item.quantity<<"\" stackable=\""<<item.stackable<<"\"";
    if (item.charges) out<<" charges=\""<<*item.charges<<"\"";
    if (item.currency_value) out<<" currency=\""<<*item.currency_value<<"\"";
    if (!item.room_local_id.empty() || item.can_push!=1) out<<" room-id=\""<<escape(item.room_local_id)<<"\" push=\""<<unsigned(item.can_push)<<"\"";
    out<<' '<<extra;
    if (!item.artifact) {out<<"/>\n";return;}
    const auto& artifact=*item.artifact;
    out<<"><artifact name=\""<<escape(artifact.name)<<"\" description=\""<<escape(artifact.description)
       <<"\" buff-description=\""<<escape(artifact.buff_description)<<"\" quality=\""<<artifact.quality<<"\">\n";
    write_array(out,"primary",artifact.modifiers.primary);write_array(out,"secondary",artifact.modifiers.secondary);
    write_array(out,"damage",artifact.modifiers.damage);write_array(out,"resistance",artifact.modifiers.resistance);
    write_array(out,"weapon-damage",artifact.weapon_damage);out<<"</artifact></item>\n";
}
InventoryItem read_item_instance(const XmlNode& node) {
    InventoryItem item;item.instance=attribute_number<std::uint64_t>(node,"id");item.kind=static_cast<ItemKind>(attribute_number<std::uint32_t>(node,"kind"));
    item.definition=attribute_number<std::uint32_t>(node,"definition");item.weapon_type=attribute_number<int>(node,"weapon");
    item.quantity=attribute_number<std::int32_t>(node,"quantity");const auto stack=attribute_number<unsigned>(node,"stackable");
    if (!item.instance || item.quantity<1 || stack>1) throw std::runtime_error("invalid saved item");
    if (node.attributes.count("charges")) {
        item.charges=attribute_number<std::int32_t>(node,"charges");
        if (item.kind!=ItemKind::wand || *item.charges<0) throw std::runtime_error("invalid saved wand charges");
    }
    if (node.attributes.count("currency")) {
        item.currency_value=attribute_number<std::int32_t>(node,"currency");
        if (item.kind!=ItemKind::currency) throw std::runtime_error("unexpected saved currency value");
    }
    if (item.kind==ItemKind::currency && !item.currency_value) throw std::runtime_error("saved currency value missing");
    if (node.attributes.count("room-id")) item.room_local_id=attribute(node,"room-id");
    if (node.attributes.count("push")) item.can_push=attribute_number<std::uint8_t>(node,"push");
    if (!node.children.empty()) {
        if (node.children.size()!=1 || node.children[0].tag!="artifact" ||
            (item.kind!=ItemKind::weapon && item.kind!=ItemKind::armour)) throw std::runtime_error("invalid saved artifact payload");
        const auto& payload=node.children[0];Artifact artifact;
        artifact.name=attribute(payload,"name");artifact.description=attribute(payload,"description");
        artifact.buff_description=attribute(payload,"buff-description");artifact.quality=attribute_number<int>(payload,"quality");
        artifact.modifiers.primary=array<std::int32_t,6>(child(payload,"primary"));
        artifact.modifiers.secondary=array<std::int32_t,24>(child(payload,"secondary"));
        artifact.modifiers.damage=array<std::int32_t,16>(child(payload,"damage"));
        artifact.modifiers.resistance=array<std::int32_t,16>(child(payload,"resistance"));
        artifact.weapon_damage=array<std::int32_t,16>(child(payload,"weapon-damage"));
        if (item.kind==ItemKind::armour && artifact.weapon_damage!=Damage{}) throw std::runtime_error("armour artifact has weapon damage");
        item.artifact=std::move(artifact);
    }
    item.stackable=stack!=0;return item;
}
void write_room_sprite(std::ostream& out,const RoomSprite& sprite) {
    out<<" image=\""<<escape(sprite.image)<<"\" series=\""<<escape(sprite.series)<<"\" first=\""<<sprite.first
       <<"\" count=\""<<sprite.count<<"\" rate=\""<<sprite.rate<<'"';
}
RoomSprite read_room_sprite(const XmlNode& node) {
    return {attribute(node,"image"),attribute(node,"series"),attribute_number<int>(node,"first"),attribute_number<int>(node,"count"),attribute_number<int>(node,"rate")};
}
bool same_sprite(const RoomSprite& a,const RoomSprite& b) {
    return a.image==b.image && a.series==b.series && a.first==b.first && a.count==b.count && a.rate==b.rate;
}
void write_room_object(std::ostream& out,const CustomRoomObject& object) {
    out<<"<object kind=\""<<unsigned(object.kind)<<"\" name=\""<<escape(object.name)<<"\" description=\""<<escape(object.description)
       <<"\" id=\""<<escape(object.room_local_id)<<"\" x=\""<<object.position.x<<"\" y=\""<<object.position.y<<"\" push=\""<<unsigned(object.can_push)
       <<"\" movement=\""<<object.blocks_movement<<"\" vision=\""<<object.blocks_vision<<"\" frame=\""<<object.frame<<"\" playing=\""<<object.playing
       <<"\" looping=\""<<object.looping<<'"';write_room_sprite(out,object.sprite);out<<"/>\n";
}
CustomRoomObject read_room_object(const XmlNode& node,const World& world) {
    const auto kind=attribute_number<unsigned>(node,"kind"),push=attribute_number<unsigned>(node,"push");
    if (kind>3 || push>255) throw std::runtime_error("invalid saved room object type/push");
    CustomRoomObject object;object.kind=static_cast<CustomRoomKind>(kind);object.can_push=push;
    object.position={attribute_number<int>(node,"x"),attribute_number<int>(node,"y")};
    if (object.position.x<0 || object.position.y<0 || object.position.x>=world.width() || object.position.y>=world.height())
        throw std::runtime_error("saved room object outside room");
    object.name=attribute(node,"name");object.description=attribute(node,"description");object.room_local_id=attribute(node,"id");
    object.blocks_movement=flag(node,"movement");object.blocks_vision=flag(node,"vision");object.frame=attribute_number<std::uint32_t>(node,"frame");
    object.playing=flag(node,"playing");object.looping=flag(node,"looping");object.sprite=read_room_sprite(node);return object;
}
}
void Session::save(std::ostream& out) const {
    bool scheduled=!player_.triggers.empty();for (const auto& actor:enemies_) scheduled=scheduled || !actor.triggers.empty();
    bool gated=false;auto gates=[&](const Actor& actor){for (const auto& trigger:actor.triggers) gated=gated || trigger.requires_buff || trigger.forbids_buff || !trigger.required_buff.empty() || !trigger.forbidden_buff.empty();};
    gates(player_);for (const auto& actor:enemies_) gates(actor);
    out<<"<dredmor-session version=\""<<(gated?10:!player_buffs_.empty()?9:scheduled?8:7)<<"\" turn=\""<<world_.turn()<<"\" next-actor=\""<<next_actor_<<"\" experience-n=\""<<experience_n_
       <<"\" content=\""<<signature(definitions_)<<"\"";
    if (player_facing_!=2) out<<" facing=\""<<player_facing_<<"\"";
    out<<">\n<room>"<<escape(world_.room_text())<<"</room>\n";
    write_actor(out,"player",player_);out<<"<enemies>\n";for (const auto& enemy:enemies_) write_actor(out,"enemy",enemy);out<<"</enemies>\n";
    out<<"<progress level=\""<<progression_.level<<"\" xp=\""<<progression_.experience<<"\" lifetime=\""<<progression_.lifetime_experience
       <<"\" score=\""<<progression_.score_experience<<"\" points=\""<<progression_.unspent_points<<"\"/>\n";
    auto mt=random_.mt.state();const auto normal=random_.normal.state();std::uint32_t normal_bits;
    std::memcpy(&normal_bits,&normal.spare,sizeof(normal_bits));
    out<<"<random libc=\""<<random_.libc.state()<<"\" index=\""<<mt.index<<"\" normal-available=\""<<normal.available
       <<"\" normal-spare=\""<<normal_bits<<"\">\n";
    for (auto word:mt.words) out<<word<<' ';
    out<<"</random>\n";
    out<<"<inventory><bag>\n";
    for (std::size_t i=0;i<48;++i) if (inventory_.bag()[i]) write_item(out,*inventory_.bag()[i],"slot=\""+std::to_string(i)+"\"");
    out<<"</bag><belt>\n";
    for (std::size_t i=0;i<9;++i) if (inventory_.belt()[i]) write_item(out,*inventory_.belt()[i],"slot=\""+std::to_string(i)+"\"");
    out<<"</belt></inventory><equipment>\n";
    for (std::size_t i=0;i<12;++i) if (equipment_.slots()[i]) write_item(out,*equipment_.slots()[i],"slot=\""+std::to_string(i)+"\"");
    out<<"</equipment><ground>\n";
    for (const auto& item:ground_) write_item(out,item.item,"x=\""+std::to_string(item.position.x)+"\" y=\""+std::to_string(item.position.y)+"\"");
    out<<"</ground>\n";
    if (!traps_.empty()) {
        out<<"<traps>\n";
        for (const auto& trap:traps_) out<<"<trap instance=\""<<trap.instance<<"\" definition=\""<<trap.definition<<"\" x=\""<<trap.position.x<<"\" y=\""<<trap.position.y
            <<"\" origin-x=\""<<trap.origin.x<<"\" origin-y=\""<<trap.origin.y<<"\" origin-object=\""<<trap.has_origin_object<<"\" charges=\""<<trap.charges
            <<"\" discovered=\""<<trap.discovered<<"\" spent=\""<<trap.spent<<"\" disarmed=\""<<trap.disarmed<<"\" pending=\""<<trap.pending_destruction
            <<"\" revealed=\""<<trap.sprite_revealed<<"\" disabled=\""<<trap.disabled<<"\" placed=\""<<trap.player_placed<<"\" id=\""<<escape(trap.room_local_id)
            <<"\" push=\""<<unsigned(trap.can_push)<<"\"/>\n";
        out<<"</traps>\n";
    }
    write_array(out,"ranks",ranks_);write_array(out,"intrinsic-damage",intrinsic_damage_);
    write_array(out,"intrinsic-resistance",intrinsic_resistance_);
    if (!factory_state_.spawned_artifacts.empty()) {
        out<<"<spawned-artifacts>";for (auto index:factory_state_.spawned_artifacts) out<<index<<' ';out<<"</spawned-artifacts>\n";
    }
    if (!monster_state_.counts.empty()) {
        out<<"<monster-counts>\n";
        for (const auto& entry:monster_state_.counts) out<<"<entry name=\""<<escape(entry.first)<<"\" count=\""<<entry.second<<"\"/>\n";
        out<<"</monster-counts>\n";
    }
    out<<"<skills content=\""<<ability_signature(abilities_)<<"\">\n";
    for (auto skill:skills_) out<<"<skill id=\""<<skill.id<<"\" rank=\""<<skill.rank<<"\"/>\n";
    out<<"</skills>\n<custom-objects>\n";
    for (const auto& object:custom_objects_) write_room_object(out,object);
    out<<"</custom-objects>\n";
    if (scheduled) {
        if (!spells_) throw std::runtime_error("scheduled spells require spell content");
        out<<"<scheduled-spells>\n";
        auto write=[&](const Actor& actor) {
            for (const auto& trigger:actor.triggers) {
                if (trigger.spell>=spells_->size()) throw std::runtime_error("invalid scheduled spell index");
                out<<"<trigger owner=\""<<actor.id<<"\" caster=\""<<trigger.caster<<"\" spell=\""<<trigger.spell
                   <<"\" remaining=\""<<trigger.remaining<<"\" percent=\""<<trigger.percent<<"\" repeat=\""<<trigger.repeat
                   <<"\" parent=\""<<escape(trigger.parent)<<"\"";
                if (gated) out<<" requires=\""<<trigger.requires_buff<<"\" forbids=\""<<trigger.forbids_buff<<"\" required-name=\""<<escape(trigger.required_buff)<<"\" forbidden-name=\""<<escape(trigger.forbidden_buff)<<"\"";
                out<<"/>\n";
            }
        };
        write(player_);for (const auto& actor:enemies_) write(actor);
        out<<"</scheduled-spells>\n";
    }
    if (!player_buffs_.empty()) {
        out<<"<player-buffs>\n";
        for (const auto& buff:player_buffs_) {
            buff_definition(buff);if (buff.deapplied) throw std::runtime_error("cannot save a transient deapplied buff");
            out<<"<buff spell=\""<<buff.spell<<"\" definition=\""<<buff.buff<<"\" remaining=\""<<buff.remaining<<"\" upkeep=\""<<buff.upkeep_remaining<<"\"/>\n";
        }
        out<<"</player-buffs>\n";
    }
    if (spells_) {
        out<<"<spell-rules content=\""<<spell_signature(spells_)<<"\"";const auto maneuvers=maneuver_signature(spells_);
        if (maneuvers) out<<" maneuvers=\""<<maneuvers<<"\"";
        const auto buffs=buffs_signature(spells_);if (buffs) out<<" buffs=\""<<buffs<<"\"";
        out<<"/>\n";
    }
    if (!breakables_.empty()) {
        out<<"<breakables>\n";
        for (const auto& object:breakables_) {
            out<<"<breakable hits-needed=\""<<object.hits_needed<<"\" hits-taken=\""<<object.hits_taken<<"\" intact-frames=\""<<object.frames.intact
               <<"\" breaking-frames=\""<<object.frames.breaking<<"\" broken-frames=\""<<object.frames.broken<<"\" single-delay=\""<<object.single_breaking_delay<<"\">\n";
            write_room_object(out,object.object);
            for (const auto& entry:{std::pair<const char*,const RoomSprite*>{"intact",&object.intact},{"breaking",&object.breaking},{"broken",&object.broken}}) {
                out<<'<'<<entry.first;write_room_sprite(out,*entry.second);out<<"/>\n";
            }
            out<<"</breakable>\n";
        }
        out<<"</breakables>\n";
    }
    out<<"</dredmor-session>\n";
    if (!out) throw std::runtime_error("cannot write session save");
}
Session Session::load(std::istream& input,std::shared_ptr<const std::vector<ItemDefinition>> definitions,
                       std::shared_ptr<const std::vector<AbilityDefinition>> abilities,std::shared_ptr<const std::vector<SpellDefinition>> spells) {
    std::string content;std::array<char,4096> buffer;
    while (input) {input.read(buffer.data(),buffer.size());content.append(buffer.data(),static_cast<std::size_t>(input.gcount()));if (content.size()>64*1024*1024) throw std::runtime_error("save exceeds size limit");}
    if (input.bad()) throw std::runtime_error("cannot read session save");
    const auto root=parse_xml(content,"session save");
    const auto version=attribute(root,"version");
    const bool modern=version=="7" || version=="8" || version=="9" || version=="10";
    if (version=="10" && !root.first("scheduled-spells")) throw std::runtime_error("version ten requires gated spell state");
    if (version=="8" && !root.first("scheduled-spells")) throw std::runtime_error("version eight requires scheduled spell state");
    if (version=="9" && !root.first("player-buffs")) throw std::runtime_error("version nine requires active buff state");
    if (root.tag!="dredmor-session" || (version!="1" && version!="2" && version!="3" && version!="4" && version!="5" && version!="6" && !modern)) throw std::runtime_error("unsupported session save version");
    const bool legacy_types=version=="1" || version=="2";
    if (attribute_number<std::uint64_t>(root,"content")!=signature(definitions,legacy_types)) throw std::runtime_error("save item content does not match loaded definitions");
    if (root.first("spell-rules")) {
        if (!modern || !spells || attribute_number<std::uint64_t>(child(root,"spell-rules"),"content")!=spell_signature(spells))
            throw std::runtime_error("save spell content does not match loaded definitions");
        const auto& rules=child(root,"spell-rules");
        if (rules.attributes.count("maneuvers") && attribute_number<std::uint64_t>(rules,"maneuvers")!=maneuver_signature(spells))
            throw std::runtime_error("save maneuver content does not match loaded definitions");
        if (rules.attributes.count("buffs") && attribute_number<std::uint64_t>(rules,"buffs")!=buffs_signature(spells)) throw std::runtime_error("save buff content does not match loaded definitions");
    }
    const auto turn=attribute_number<std::uint64_t>(root,"turn");
    if (turn==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("saved turn counter exhausted");
    std::istringstream rows(child(root,"room").text);auto world=World::restore(rows,turn);auto player=read_actor(child(root,"player"),version=="6" || modern,modern);
    if (player.position.x!=world.player().x || player.position.y!=world.player().y) throw std::runtime_error("saved player/room position mismatch");
    Session session(std::move(world),player,0,attribute_number<int>(root,"experience-n"));
    if (root.attributes.count("facing")) {
        if (!modern) throw std::runtime_error("saved facing requires version seven");
        session.player_facing(attribute_number<int>(root,"facing"));
    }
    session.definitions_=std::move(definitions);session.spells_=std::move(spells);session.next_actor_=attribute_number<std::uint64_t>(root,"next-actor");
    std::set<std::uint64_t> actors={player.id},items;
    for (const auto& node:child(root,"enemies").children) {
        if (node.tag!="enemy" || session.enemies_.size()>=512) throw std::runtime_error("invalid saved enemy list");
        auto actor=read_actor(node,version=="6" || modern,modern);
        if (!actors.insert(actor.id).second || !session.world_.passable(actor.position)) throw std::runtime_error("invalid saved enemy identity/position");
        if (actor.alive()) {
            if ((actor.position.x==player.position.x && actor.position.y==player.position.y) || session.enemy_at(actor.position)) throw std::runtime_error("saved actors overlap");
        }
        session.enemies_.push_back(std::move(actor));
    }
    if (!session.next_actor_ || session.next_actor_<=*actors.rbegin()) throw std::runtime_error("invalid saved actor sequence");
    if (root.first("scheduled-spells")) {
        if ((version!="8" && version!="9" && version!="10") || !session.spells_ || !root.first("spell-rules")) throw std::runtime_error("scheduled spells require matching rules and a supported version");
        std::size_t count=0;
        for (const auto& node:child(root,"scheduled-spells").children) {
            if (node.tag!="trigger" || !node.children.empty() || ++count>4096*513) throw std::runtime_error("invalid scheduled spell list");
            const auto owner=attribute_number<std::uint64_t>(node,"owner");
            ScheduledSpell trigger;trigger.caster=attribute_number<std::uint64_t>(node,"caster");
            trigger.spell=attribute_number<std::size_t>(node,"spell");trigger.remaining=attribute_number<std::int32_t>(node,"remaining");
            trigger.percent=attribute_number<std::int32_t>(node,"percent");trigger.repeat=flag(node,"repeat");trigger.parent=attribute(node,"parent");
            if (version=="10") {trigger.requires_buff=flag(node,"requires");trigger.forbids_buff=flag(node,"forbids");trigger.required_buff=attribute(node,"required-name");trigger.forbidden_buff=attribute(node,"forbidden-name");}
            else if (node.attributes.count("requires") || node.attributes.count("forbids") || node.attributes.count("required-name") || node.attributes.count("forbidden-name")) throw std::runtime_error("trigger buff gates require version ten");
            if (!actors.count(owner) || (trigger.caster && !actors.count(trigger.caster)) || trigger.spell>=session.spells_->size()) throw std::runtime_error("invalid scheduled spell identity");
            auto& actor=session.actor_by_id(owner);
            if (actor.triggers.size()>=4096) throw std::runtime_error("scheduled spell capacity exceeded");
            actor.triggers.push_back(std::move(trigger));
        }
    }
    if (root.first("player-buffs")) {
        if ((version!="9" && version!="10") || !session.spells_ || !root.first("spell-rules") || !child(root,"spell-rules").attributes.count("buffs")) throw std::runtime_error("active buffs require matching rules and a supported version");
        for (const auto& node:child(root,"player-buffs").children) {
            if (node.tag!="buff" || !node.children.empty() || session.player_buffs_.size()>=4096) throw std::runtime_error("invalid saved buff list");
            ActiveBuff buff;buff.spell=attribute_number<std::size_t>(node,"spell");buff.buff=attribute_number<std::size_t>(node,"definition");buff.remaining=attribute_number<std::int32_t>(node,"remaining");
            const auto& definition=session.buff_definition(buff);session.validate_player_buff(definition);
            buff.upkeep_remaining=attribute_number<std::int32_t>(node,"upkeep");
            if (!definition.mana_upkeep && buff.upkeep_remaining) throw std::runtime_error("inactive buff upkeep has a countdown");
            if (!session.player_buffs_.empty() && session.spells_->at(session.player_buffs_.back().spell).name>session.spells_->at(buff.spell).name) throw std::runtime_error("saved buffs are out of order");
            session.player_buffs_.push_back(buff);
        }
    }
    const auto& progress=child(root,"progress");
    session.progression_={attribute_number<int>(progress,"level"),attribute_number<int>(progress,"xp"),attribute_number<int>(progress,"lifetime"),
                          attribute_number<int>(progress,"score"),attribute_number<int>(progress,"points")};
    if (session.progression_.level<0 || session.progression_.level>50 || session.progression_.unspent_points<0) throw std::runtime_error("invalid saved progression");
    const auto& random=child(root,"random");
    session.random_.libc.seed(attribute_number<std::uint32_t>(random,"libc"));
    session.random_.mt.restore({array<std::uint32_t,624>(random),attribute_number<std::size_t>(random,"index")});
    if (version=="5" || version=="6" || modern) {
        const auto bits=attribute_number<std::uint32_t>(random,"normal-spare");float spare;
        std::memcpy(&spare,&bits,sizeof(spare));
        if (!std::isfinite(spare)) throw std::runtime_error("invalid saved normal cache");
        session.random_.normal.restore({flag(random,"normal-available"),spare});
    }
    if (version=="4" || version=="5" || version=="6" || modern) for (const auto& node:child(root,"custom-objects").children) {
        if (node.tag!="object" || session.custom_objects_.size()>=10000) throw std::runtime_error("invalid saved custom object list");
        auto object=read_room_object(node,session.world_);
        if (object.kind==CustomRoomKind::breakable || (object.kind==CustomRoomKind::pedestal && !modern)) throw std::runtime_error("invalid saved custom object kind");
        session.custom_objects_.push_back(std::move(object));
    }
    if (root.first("breakables")) {
        if (!modern) throw std::runtime_error("breakables require save version seven");
        for (const auto& node:child(root,"breakables").children) {
            if (node.tag!="breakable" || node.children.size()!=4 || session.breakables_.size()>=10000) throw std::runtime_error("invalid saved breakable list");
            BreakableRoomObject object;object.object=read_room_object(child(node,"object"),session.world_);
            object.intact=read_room_sprite(child(node,"intact"));object.breaking=read_room_sprite(child(node,"breaking"));object.broken=read_room_sprite(child(node,"broken"));
            object.hits_needed=attribute_number<int>(node,"hits-needed");object.hits_taken=attribute_number<int>(node,"hits-taken");
            object.frames={attribute_number<std::uint16_t>(node,"intact-frames"),attribute_number<std::uint16_t>(node,"breaking-frames"),attribute_number<std::uint16_t>(node,"broken-frames")};
            object.single_breaking_delay=attribute_number<std::uint16_t>(node,"single-delay");
            const bool broken=object.hits_taken>=object.hits_needed;
            if (object.object.kind!=CustomRoomKind::breakable || !object.frames.intact || !object.frames.breaking || !object.frames.broken ||
                object.single_breaking_delay!=(object.frames.breaking==1?10:0) || object.object.playing || object.object.looping ||
                !same_sprite(object.object.sprite,broken?object.broken:object.intact) ||
                (broken && object.object.frame!=std::uint32_t(object.frames.broken-1))) throw std::runtime_error("invalid saved breakable state");
            session.breakables_.push_back(std::move(object));
        }
    }
    Inventory::Bag bag{};Inventory::Belt belt{};
    auto validate_item=[&](const XmlNode& node) {
        if (node.tag!="item") throw std::runtime_error("invalid saved item node");
        auto item=read_item_instance(node);
        if (item.artifact && !modern) throw std::runtime_error("artifact payload requires save version seven");
        if (!modern && (node.attributes.count("room-id") || node.attributes.count("push"))) throw std::runtime_error("item room metadata requires save version seven");
        if (legacy_types) {
            const auto kind=static_cast<std::uint32_t>(item.kind);
            if (kind==11) item.kind=ItemKind::trap;
            else if (kind==12) item.kind=ItemKind::toolkit;
        }
        if (!items.insert(item.instance).second) throw std::runtime_error("duplicate saved item instance");
        if (session.definitions_ && (item.definition>=session.definitions_->size() || session.definitions_->at(item.definition).kind!=item.kind))
            throw std::runtime_error("saved item definition mismatch");
        return item;
    };
    const auto& inventory=child(root,"inventory");
    for (const auto& node:child(inventory,"bag").children) {
        const auto index=attribute_number<std::size_t>(node,"slot");if (index>=48 || bag[index]) throw std::runtime_error("invalid saved bag slot");bag[index]=validate_item(node);
    }
    for (const auto& node:child(inventory,"belt").children) {
        const auto index=attribute_number<std::size_t>(node,"slot");if (index>=9 || belt[index]) throw std::runtime_error("invalid saved belt slot");belt[index]=validate_item(node);
    }
    session.inventory_=Inventory::restore(bag,belt);
    for (const auto& node:child(root,"equipment").children) {
        const auto index=attribute_number<std::size_t>(node,"slot");if (!session.definitions_ || index>=12 || session.equipment_.slots()[index]) throw std::runtime_error("invalid saved equipment slot");
        auto displaced=session.equipment_.equip(validate_item(node),static_cast<EquipmentSlot>(index),*session.definitions_);
        if (displaced) throw std::runtime_error("saved equipment unexpectedly replaced an item");
    }
    for (const auto& node:child(root,"ground").children) {
        if (session.ground_.size()>=10000) throw std::runtime_error("saved ground item limit exceeded");
        session.drop(validate_item(node),{attribute_number<int>(node,"x"),attribute_number<int>(node,"y")});
    }
    if (root.first("traps")) {
        if (!modern || !session.definitions_) throw std::runtime_error("saved traps need version seven and definitions");
        for (const auto& node:child(root,"traps").children) {
            TrapInstance trap;trap.instance=attribute_number<std::uint64_t>(node,"instance");trap.definition=attribute_number<std::uint32_t>(node,"definition");
            trap.position={attribute_number<int>(node,"x"),attribute_number<int>(node,"y")};trap.origin={attribute_number<int>(node,"origin-x"),attribute_number<int>(node,"origin-y")};
            trap.charges=attribute_number<std::int32_t>(node,"charges");trap.room_local_id=attribute(node,"id");trap.can_push=attribute_number<std::uint8_t>(node,"push");
            trap.has_origin_object=flag(node,"origin-object");trap.discovered=flag(node,"discovered");trap.spent=flag(node,"spent");trap.disarmed=flag(node,"disarmed");
            trap.pending_destruction=flag(node,"pending");trap.sprite_revealed=flag(node,"revealed");trap.disabled=flag(node,"disabled");trap.player_placed=flag(node,"placed");
            auto inside=[&](Position p){return p.x>=0 && p.y>=0 && p.x<session.world_.width() && p.y<session.world_.height();};
            if (node.tag!="trap" || !node.children.empty() || !trap.instance || !items.insert(trap.instance).second || session.traps_.size()>=10000 ||
                trap.definition>=session.definitions_->size() || session.definitions_->at(trap.definition).kind!=ItemKind::trap || !session.definitions_->at(trap.definition).trap ||
                !inside(trap.position) || (trap.has_origin_object && !inside(trap.origin))) throw std::runtime_error("invalid saved trap state");
            session.traps_.push_back(std::move(trap));
        }
    }
    session.ranks_=array<std::int32_t,3>(child(root,"ranks"));
    session.intrinsic_damage_=array<std::int32_t,16>(child(root,"intrinsic-damage"));
    session.intrinsic_resistance_=array<std::int32_t,16>(child(root,"intrinsic-resistance"));
    if (root.first("spawned-artifacts")) {
        if (!modern || !session.definitions_) throw std::runtime_error("invalid saved artifact registration");
        const auto& spawned=child(root,"spawned-artifacts");
        if (!spawned.children.empty()) throw std::runtime_error("invalid saved artifact registration");
        std::istringstream indices(spawned.text);std::string token;
        while (indices>>token) {
            const auto index=number<std::uint32_t>(token);
            if (index>=session.definitions_->size() || session.factory_state_.spawned_artifacts.size()>=10000 ||
                session.definitions_->at(index).kind!=ItemKind::weapon || !artifact_request(session.definitions_->at(index),0).fixed_name ||
                !session.factory_state_.spawned_artifacts.insert(index).second) throw std::runtime_error("invalid saved artifact registration");
        }
    }
    if (root.first("monster-counts")) {
        if (!modern) throw std::runtime_error("monster counts require save version seven");
        for (const auto& entry:child(root,"monster-counts").children) {
            const auto name=attribute(entry,"name");const auto count=attribute_number<std::int32_t>(entry,"count");
            if (entry.tag!="entry" || name.empty() || count<1 || !entry.children.empty() || session.monster_state_.counts.size()>=10000 ||
                !session.monster_state_.counts.emplace(name,count).second) throw std::runtime_error("invalid saved monster counts");
        }
    }
    if (version!="1") {
        const auto& selected=child(root,"skills");
        const auto content=attribute_number<std::uint64_t>(selected,"content");
        if (content) {
            if (!session.definitions_ || !abilities || ability_signature(abilities,version=="6" || modern)!=content)
                throw std::runtime_error("save ability content does not match loaded definitions");
            std::set<int> seen;
            for (const auto& node:selected.children) {
                if (node.tag!="skill" || session.skills_.size()>=7) throw std::runtime_error("invalid saved skill list");
                SkillRank skill{attribute_number<int>(node,"id"),attribute_number<int>(node,"rank")};
                if (skill.id<0 || skill.rank<1 || !seen.insert(skill.id).second) throw std::runtime_error("invalid saved skill");
                session.skills_.push_back(skill);
            }
            if (session.skills_.size()!=7) throw std::runtime_error("save requires seven selected skills");
            session.abilities_=std::move(abilities);
        } else if (!selected.children.empty()) throw std::runtime_error("saved skills require ability content");
    }
    return session;
}
}
