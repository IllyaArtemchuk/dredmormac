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
void write_actor(std::ostream& out,std::string_view tag,const Actor& actor) {
    out<<'<'<<tag<<" id=\""<<actor.id<<"\" name=\""<<escape(actor.name)<<"\" sprite=\""<<escape(actor.sprite)
       <<"\" x=\""<<actor.position.x<<"\" y=\""<<actor.position.y<<"\" hp=\""<<actor.health
       <<"\" mana=\""<<actor.mana<<"\" reward=\""<<actor.experience_reward<<"\" zorkmids=\""<<actor.zorkmids<<"\">\n";
    write_array(out,"primary",actor.primary);write_array(out,"secondary",actor.secondary);
    write_array(out,"damage",actor.damage);write_array(out,"resistance",actor.resistance);
    out<<"</"<<tag<<">\n";
}
Actor read_actor(const XmlNode& node,bool currency_fields) {
    Actor actor;actor.id=attribute_number<std::uint64_t>(node,"id");actor.name=attribute(node,"name");actor.sprite=attribute(node,"sprite");
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
    out<<' '<<extra<<"/>\n";
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
    item.stackable=stack!=0;return item;
}
}
void Session::save(std::ostream& out) const {
    out<<"<dredmor-session version=\"6\" turn=\""<<world_.turn()<<"\" next-actor=\""<<next_actor_<<"\" experience-n=\""<<experience_n_
       <<"\" content=\""<<signature(definitions_)<<"\">\n<room>"<<escape(world_.room_text())<<"</room>\n";
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
    out<<"</ground>\n";write_array(out,"ranks",ranks_);write_array(out,"intrinsic-damage",intrinsic_damage_);
    write_array(out,"intrinsic-resistance",intrinsic_resistance_);
    out<<"<skills content=\""<<ability_signature(abilities_)<<"\">\n";
    for (auto skill:skills_) out<<"<skill id=\""<<skill.id<<"\" rank=\""<<skill.rank<<"\"/>\n";
    out<<"</skills>\n<custom-objects>\n";
    for (const auto& object:custom_objects_) {
        const auto& sprite=object.sprite;
        out<<"<object kind=\""<<unsigned(object.kind)<<"\" name=\""<<escape(object.name)<<"\" description=\""<<escape(object.description)
            <<"\" id=\""<<escape(object.room_local_id)<<"\" x=\""<<object.position.x<<"\" y=\""<<object.position.y<<"\" push=\""<<unsigned(object.can_push)
            <<"\" movement=\""<<object.blocks_movement<<"\" vision=\""<<object.blocks_vision<<"\" frame=\""<<object.frame<<"\" playing=\""<<object.playing
            <<"\" looping=\""<<object.looping<<"\" image=\""<<escape(sprite.image)<<"\" series=\""<<escape(sprite.series)<<"\" first=\""<<sprite.first
            <<"\" count=\""<<sprite.count<<"\" rate=\""<<sprite.rate<<"\"/>\n";
    }
    out<<"</custom-objects>\n</dredmor-session>\n";
    if (!out) throw std::runtime_error("cannot write session save");
}
Session Session::load(std::istream& input,std::shared_ptr<const std::vector<ItemDefinition>> definitions,
                       std::shared_ptr<const std::vector<AbilityDefinition>> abilities) {
    std::string content;std::array<char,4096> buffer;
    while (input) {input.read(buffer.data(),buffer.size());content.append(buffer.data(),static_cast<std::size_t>(input.gcount()));if (content.size()>64*1024*1024) throw std::runtime_error("save exceeds size limit");}
    if (input.bad()) throw std::runtime_error("cannot read session save");
    const auto root=parse_xml(content,"session save");
    const auto version=attribute(root,"version");
    if (root.tag!="dredmor-session" || (version!="1" && version!="2" && version!="3" && version!="4" && version!="5" && version!="6")) throw std::runtime_error("unsupported session save version");
    const bool legacy_types=version=="1" || version=="2";
    if (attribute_number<std::uint64_t>(root,"content")!=signature(definitions,legacy_types)) throw std::runtime_error("save item content does not match loaded definitions");
    const auto turn=attribute_number<std::uint64_t>(root,"turn");
    if (turn==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("saved turn counter exhausted");
    std::istringstream rows(child(root,"room").text);auto world=World::restore(rows,turn);auto player=read_actor(child(root,"player"),version=="6");
    if (player.position.x!=world.player().x || player.position.y!=world.player().y) throw std::runtime_error("saved player/room position mismatch");
    Session session(std::move(world),player,0,attribute_number<int>(root,"experience-n"));
    session.definitions_=std::move(definitions);session.next_actor_=attribute_number<std::uint64_t>(root,"next-actor");
    std::set<std::uint64_t> actors={player.id},items;
    for (const auto& node:child(root,"enemies").children) {
        if (node.tag!="enemy" || session.enemies_.size()>=512) throw std::runtime_error("invalid saved enemy list");
        auto actor=read_actor(node,version=="6");
        if (!actors.insert(actor.id).second || !session.world_.passable(actor.position)) throw std::runtime_error("invalid saved enemy identity/position");
        if (actor.alive()) {
            if ((actor.position.x==player.position.x && actor.position.y==player.position.y) || session.enemy_at(actor.position)) throw std::runtime_error("saved actors overlap");
        }
        session.enemies_.push_back(std::move(actor));
    }
    if (!session.next_actor_ || session.next_actor_<=*actors.rbegin()) throw std::runtime_error("invalid saved actor sequence");
    const auto& progress=child(root,"progress");
    session.progression_={attribute_number<int>(progress,"level"),attribute_number<int>(progress,"xp"),attribute_number<int>(progress,"lifetime"),
                          attribute_number<int>(progress,"score"),attribute_number<int>(progress,"points")};
    if (session.progression_.level<0 || session.progression_.level>50 || session.progression_.unspent_points<0) throw std::runtime_error("invalid saved progression");
    const auto& random=child(root,"random");
    session.random_.libc.seed(attribute_number<std::uint32_t>(random,"libc"));
    session.random_.mt.restore({array<std::uint32_t,624>(random),attribute_number<std::size_t>(random,"index")});
    if (version=="5" || version=="6") {
        const auto bits=attribute_number<std::uint32_t>(random,"normal-spare");float spare;
        std::memcpy(&spare,&bits,sizeof(spare));
        if (!std::isfinite(spare)) throw std::runtime_error("invalid saved normal cache");
        session.random_.normal.restore({flag(random,"normal-available"),spare});
    }
    if (version=="4" || version=="5" || version=="6") for (const auto& node:child(root,"custom-objects").children) {
        if (node.tag!="object" || session.custom_objects_.size()>=10000) throw std::runtime_error("invalid saved custom object list");
        const auto kind=attribute_number<unsigned>(node,"kind"),push=attribute_number<unsigned>(node,"push");
        if (kind>1 || push>255) throw std::runtime_error("invalid saved custom object type/push");
        CustomRoomObject object;object.kind=static_cast<CustomRoomKind>(kind);object.can_push=push;
        object.position={attribute_number<int>(node,"x"),attribute_number<int>(node,"y")};
        if (object.position.x<0 || object.position.y<0 || object.position.x>=session.world_.width() || object.position.y>=session.world_.height())
            throw std::runtime_error("saved custom object outside room");
        object.name=attribute(node,"name");object.description=attribute(node,"description");object.room_local_id=attribute(node,"id");
        object.blocks_movement=flag(node,"movement");object.blocks_vision=flag(node,"vision");object.frame=attribute_number<std::uint32_t>(node,"frame");
        object.playing=flag(node,"playing");object.looping=flag(node,"looping");
        object.sprite.image=attribute(node,"image");object.sprite.series=attribute(node,"series");
        object.sprite.first=attribute_number<int>(node,"first");object.sprite.count=attribute_number<int>(node,"count");object.sprite.rate=attribute_number<int>(node,"rate");
        session.custom_objects_.push_back(std::move(object));
    }
    Inventory::Bag bag{};Inventory::Belt belt{};
    auto validate_item=[&](const XmlNode& node) {
        if (node.tag!="item") throw std::runtime_error("invalid saved item node");
        auto item=read_item_instance(node);
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
    session.ranks_=array<std::int32_t,3>(child(root,"ranks"));
    session.intrinsic_damage_=array<std::int32_t,16>(child(root,"intrinsic-damage"));
    session.intrinsic_resistance_=array<std::int32_t,16>(child(root,"intrinsic-resistance"));
    if (version!="1") {
        const auto& selected=child(root,"skills");
        const auto content=attribute_number<std::uint64_t>(selected,"content");
        if (content) {
            if (!session.definitions_ || !abilities || ability_signature(abilities,version=="6")!=content)
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
