#include "dredmor/room_runtime.hpp"
#include <charconv>
#include <ostream>
#include <stdexcept>

namespace dredmor {
namespace {
template<class T> T number(const XmlNode& node,const char* key) {
    const auto found=node.attributes.find(key);if (found==node.attributes.end()) throw std::runtime_error(std::string("room runtime missing ")+key);
    T value{};const auto& text=found->second;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if (parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size()) throw std::runtime_error(std::string("invalid room runtime ")+key);
    return value;
}
const std::string& attribute(const XmlNode& node,const char* key) {
    const auto found=node.attributes.find(key);if (found==node.attributes.end()) throw std::runtime_error(std::string("room runtime missing ")+key);return found->second;
}
std::string escape(std::string_view text) {
    std::string result;
    for (char c:text) switch (c) {
        case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;case '"':result+="&quot;";break;
        case '\n':result+="&#10;";break;case '\r':result+="&#13;";break;case '\t':result+="&#9;";break;default:result+=c;
    }
    return result;
}
using Templates=std::map<std::uint64_t,const std::vector<RoomScriptDefinition>*>;
Templates templates_for(const std::vector<RoomRuntimeRoom>& rooms,const RoomRuntimeCatalogue& catalogue) {
    Templates templates;
    for (const auto& [identity,scripts]:catalogue) {
        if (!identity || !scripts) throw std::runtime_error("invalid room runtime catalogue entry");
        templates.emplace(identity,scripts.get());
    }
    for (const auto& room:rooms) if (room.trigger.scripts) {
        if (!room.template_identity) throw std::runtime_error("saved runtime has zero template identity");
        const auto [entry,inserted]=templates.emplace(room.template_identity,room.trigger.scripts.get());
        if (!inserted && entry->second!=room.trigger.scripts.get()) throw std::runtime_error("saved runtime has conflicting template identity");
        if (!catalogue.empty() && !catalogue.count(room.template_identity)) throw std::runtime_error("room template absent from runtime catalogue");
    }
    return templates;
}
std::uint64_t profile_signature(const std::vector<RoomRuntimeRoom>& rooms,const Templates& templates) {
    std::uint64_t hash=14695981039346656037ULL;
    auto integer=[&](std::uint64_t value) {for (unsigned byte=0;byte<8;++byte) {hash^=value&255U;hash*=1099511628211ULL;value>>=8;}};
    auto text=[&](const std::string& value) {integer(value.size());for (unsigned char c:value) {hash^=c;hash*=1099511628211ULL;}};
    auto point=[&](Position value) {integer(std::uint32_t(value.x));integer(std::uint32_t(value.y));};
    integer(rooms.size());
    for (const auto& room:rooms) {
        integer(std::uint32_t(room.trigger.identity));point(room.origin);integer(room.template_identity);integer(bool(room.trigger.scripts));
    }
    integer(templates.size());
    for (const auto& [identity,scripts]:templates) {
        integer(identity);integer(scripts->size());
        for (const auto& script:*scripts) {
            integer(std::uint32_t(script.success_repeat));integer(script.conditions.size());
            for (const auto& condition:script.conditions) {
                integer(unsigned(condition.kind));integer(condition.negated);integer(condition.missing_value);
                text(condition.object_id);text(condition.event_name);point(condition.point);integer(std::uint32_t(condition.wanted));text(condition.item_type);text(condition.item_subtype);
            }
            for (const auto* actions:{&script.success,&script.failure}) {
                integer(actions->size());
                for (const auto& action:*actions) {
                    integer(unsigned(action.kind));text(action.object_id);text(action.text);text(action.property);text(action.cast_sound);
                    integer(std::uint32_t(action.value));integer(action.record);integer(action.spell);point(action.point);point(action.spell_origin);point(action.spell_target);
                }
            }
        }
    }
    return hash;
}
void validate(const RoomRuntimeState& state,const std::vector<RoomRuntimeRoom>& rooms,const Templates& templates) {
    if (state.triggers.success_counts.size()>100000 || state.prototypes.size()>100000) throw std::runtime_error("room runtime state limit exceeded");
    std::size_t events=0;for (const auto& [key,names]:state.triggers.events) {(void)key;events+=names.size();if (events>100000) throw std::runtime_error("room event state limit exceeded");}
    for (const auto& [key,count]:state.triggers.success_counts) {
        (void)count;const auto [room,script]=key;
        if (room<0 || std::size_t(room)>=rooms.size() || script<0 || !rooms[room].trigger.scripts || std::size_t(script)>=rooms[room].trigger.scripts->size())
            throw std::runtime_error("saved room script counter has no definition");
    }
    for (const auto& [key,type]:state.prototypes) {
        (void)type;const auto [source,script,condition]=key;const auto found=templates.find(source);
        if (found==templates.end() || script>=found->second->size() || condition>=found->second->at(script).conditions.size() || found->second->at(script).conditions[condition].kind!=RoomConditionKind::type_at)
            throw std::runtime_error("saved room prototype has no TypeAt condition");
    }
}
}
void write_room_runtime_state(std::ostream& out,const RoomRuntimeState& state,const std::vector<RoomRuntimeRoom>& rooms,const RoomRuntimeCatalogue& catalogue) {
    const auto templates=templates_for(rooms,catalogue);
    validate(state,rooms,templates);out<<"<room-runtime previous=\""<<state.triggers.previous_room<<"\" profile=\""<<profile_signature(rooms,templates)<<"\">\n";
    for (const auto& [key,count]:state.triggers.success_counts) out<<"<counter room=\""<<key.first<<"\" script=\""<<key.second<<"\" count=\""<<count<<"\"/>\n";
    for (const auto& [key,names]:state.triggers.events) for (const auto& name:names) out<<"<event room=\""<<key.first<<"\" object=\""<<escape(key.second)<<"\" name=\""<<escape(name)<<"\"/>\n";
    for (const auto& [key,type]:state.prototypes) {
        const auto [source,script,condition]=key;out<<"<prototype template=\""<<source<<"\" script=\""<<script<<"\" condition=\""<<condition<<"\" kind=\""<<type.kind<<"\" subtype=\""<<type.subtype<<"\"/>\n";
    }
    out<<"</room-runtime>\n";if (!out) throw std::runtime_error("cannot write room runtime state");
}
RoomRuntimeState read_room_runtime_state(const XmlNode& node,const std::vector<RoomRuntimeRoom>& rooms,const RoomRuntimeCatalogue& catalogue) {
    const auto templates=templates_for(rooms,catalogue);
    if (node.tag!="room-runtime" || node.attributes.size()!=2 || node.children.size()>300000 || node.text.find_first_not_of(" \t\r\n")!=std::string::npos) throw std::runtime_error("invalid room runtime snapshot");
    if (number<std::uint64_t>(node,"profile")!=profile_signature(rooms,templates)) throw std::runtime_error("room runtime snapshot belongs to another profile");
    RoomRuntimeState state;state.triggers.previous_room=number<std::int32_t>(node,"previous");
    for (const auto& child:node.children) {
        if (!child.children.empty() || child.text.find_first_not_of(" \t\r\n")!=std::string::npos) throw std::runtime_error("invalid room runtime record body");
        if (child.tag=="counter" && child.attributes.size()==3) {
            RoomScriptKey key{number<std::int32_t>(child,"room"),number<std::int32_t>(child,"script")};
            if (!state.triggers.success_counts.emplace(key,number<std::int32_t>(child,"count")).second) throw std::runtime_error("duplicate room script counter");
        } else if (child.tag=="event" && child.attributes.size()==3) {
            RoomEventKey key{number<std::int32_t>(child,"room"),attribute(child,"object")};
            if (!state.triggers.events[key].insert(attribute(child,"name")).second) throw std::runtime_error("duplicate room event");
        } else if (child.tag=="prototype" && child.attributes.size()==5) {
            RoomPrototypeKey key{number<std::uint64_t>(child,"template"),number<std::size_t>(child,"script"),number<std::size_t>(child,"condition")};
            ScriptItemType type{number<std::uint32_t>(child,"kind"),number<std::uint32_t>(child,"subtype")};
            if (!state.prototypes.emplace(key,type).second) throw std::runtime_error("duplicate room prototype");
        } else throw std::runtime_error("invalid room runtime record");
    }
    validate(state,rooms,templates);return state;
}
}
