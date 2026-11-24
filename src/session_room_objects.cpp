#include "dredmor/session_room_objects.hpp"
#include "dredmor/session.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#include <ostream>
#include <set>
#include <stdexcept>

namespace dredmor {
namespace {
std::optional<ScriptObjectState> resolve(const Session& session,SessionRoomObjectRef ref) {
    ScriptObjectState result;
    auto actor=[&](const Actor& value,bool monster) {
        result.position=value.position;result.room_local_id=value.room_local_id;
        result.monster=monster;result.dead=!value.alive();
    };
    auto custom=[&](const CustomRoomObject& value) {result.position=value.position;result.room_local_id=value.room_local_id;};
    switch (ref.kind) {
        case SessionRoomObjectKind::player:
            if (ref.subject!=session.player().id) return std::nullopt;
            actor(session.player(),false);if (result.room_local_id.empty()) result.room_local_id="player";break;
        case SessionRoomObjectKind::monster: {
            const auto& values=session.enemies();const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return value.id==ref.subject;});
            if (found==values.end()) return std::nullopt;
            actor(*found,true);break;
        }
        case SessionRoomObjectKind::item: {
            const auto& values=session.ground_items();const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return value.item.instance==ref.subject;});
            if (found==values.end()) return std::nullopt;
            result.position=found->position;result.room_local_id=found->item.room_local_id;break;
        }
        case SessionRoomObjectKind::custom:
            if (!ref.subject || ref.subject>session.custom_objects().size()) return std::nullopt;
            custom(session.custom_objects()[ref.subject-1]);break;
        case SessionRoomObjectKind::breakable:
            if (!ref.subject || ref.subject>session.breakables().size()) return std::nullopt;
            custom(session.breakables()[ref.subject-1].object);break;
        case SessionRoomObjectKind::lever:
            if (!ref.subject || ref.subject>session.script_levers().size()) return std::nullopt;
            custom(session.script_levers()[ref.subject-1].object);result.activatable=true;
            result.activation=session.script_levers()[ref.subject-1].activation;break;
        case SessionRoomObjectKind::trap: {
            const auto& values=session.traps();const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return value.instance==ref.subject;});
            if (found==values.end()) return std::nullopt;
            result.position=found->position;result.room_local_id=found->room_local_id;break;
        }
        case SessionRoomObjectKind::keeper: {
            const auto& values=session.shops();const auto found=std::find_if(values.begin(),values.end(),[&](const auto& value){return value.keeper.id==ref.subject;});
            if (found==values.end()) return std::nullopt;
            actor(found->keeper,true);break;
        }
        default:throw std::invalid_argument("invalid Session room object kind");
    }
    return result;
}
template<class T> T number(const XmlNode& node,const char* name) {
    const auto found=node.attributes.find(name);if (found==node.attributes.end()) throw std::runtime_error("missing room binding number");
    T result{};const auto& text=found->second;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if (parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size()) throw std::runtime_error("invalid room binding number");
    return result;
}
std::string escape(std::string_view value) {
    std::string result;
    for (char c:value) switch(c) {
        case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;case '"':result+="&quot;";break;
        case '\n':result+="&#10;";break;case '\r':result+="&#13;";break;case '\t':result+="&#9;";break;default:result+=c;
    }
    return result;
}
void geometry(const Session& session,const DungeonLayout& layout) {
    if (layout.width()!=session.world().width() || layout.height()!=session.world().height()) throw std::invalid_argument("room binding geometry does not match Session");
}
}
SessionRoomObjects::SessionRoomObjects(std::uint64_t level):level_(level) {
    if (!level) throw std::invalid_argument("room bindings require an owning level");
}
std::vector<RoomObjectIdentity> SessionRoomObjects::registered(const Session& session) const {
    std::vector<RoomObjectIdentity> result;result.reserve(bindings_.size());
    for (const auto& binding:bindings_) if (const auto live=resolve(session,binding.reference)) {
        auto value=binding.object;value.position=live->position;result.push_back(std::move(value));
    }
    return result;
}
RoomIdAssignment SessionRoomObjects::bind(const Session& session,SessionRoomObjectRef reference,const DungeonLayout& layout,bool finalize) {
    geometry(session,layout);
    if (bindings_.size()>=100000 || next_==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("room object binding limit exceeded");
    const auto live=resolve(session,reference);if (!live) throw std::invalid_argument("room binding refers to a missing Session object");
    if (live->room_local_id.size()>4096) throw std::invalid_argument("room object label limit exceeded");
    for (const auto& binding:bindings_) if (binding.reference.kind==reference.kind && binding.reference.subject==reference.subject)
        throw std::invalid_argument("Session object already has a room binding");
    RoomObjectIdentity object{next_,level_,-1,"",reference.kind==SessionRoomObjectKind::player,live->position};
    if (object.global) object.id=live->room_local_id;
    auto entries=registered(session);entries.push_back(object);
    RoomIdAssignment result;result.object=object;
    if (finalize) result=assign_room_object_id(object,live->room_local_id,&layout,entries);
    bindings_.push_back({reference,result.object});++next_;return result;
}
RoomIdAssignment SessionRoomObjects::assign(std::uint64_t identity,std::string id,const Session& session,const DungeonLayout& layout,bool debug) {
    geometry(session,layout);
    if (id.size()>4096) throw std::invalid_argument("room object label limit exceeded");
    const auto found=std::find_if(bindings_.begin(),bindings_.end(),[&](const auto& value){return value.object.identity==identity;});
    if (found==bindings_.end()) throw std::invalid_argument("unknown room object binding");
    const auto live=resolve(session,found->reference);if (!live) throw std::invalid_argument("room binding refers to a missing Session object");
    auto object=found->object;object.position=live->position;
    auto result=assign_room_object_id(std::move(object),std::move(id),&layout,registered(session),debug);
    found->object=result.object;return result;
}
bool SessionRoomObjects::unregister(std::uint64_t identity) {
    const auto found=std::find_if(bindings_.begin(),bindings_.end(),[&](const auto& value){return value.object.identity==identity;});
    if (found==bindings_.end()) return false;
    bindings_.erase(found);return true;
}
std::optional<ScriptObjectState> SessionRoomObjects::object(const Session& session,std::uint64_t identity) const {
    const auto found=std::find_if(bindings_.begin(),bindings_.end(),[&](const auto& value){return value.object.identity==identity;});
    if (found==bindings_.end()) return std::nullopt;
    auto live=resolve(session,found->reference);if (live) {live->identity=identity;live->room_local_id=found->object.id;}return live;
}
std::optional<ScriptObjectState> SessionRoomObjects::find(const Session& session,std::int32_t room,std::string_view id) const {
    // Match registered order, not actor/item/custom vector grouping. Missing
    // typed objects cannot accidentally match a stale cached position.
    for (const auto& binding:bindings_) if (matches_room_object(binding.object,level_,room,id))
        if (auto live=object(session,binding.object.identity)) return live;
    return std::nullopt;
}
std::optional<std::uint64_t> SessionRoomObjects::player() const {
    for (const auto& binding:bindings_) if (binding.reference.kind==SessionRoomObjectKind::player) return binding.object.identity;
    return std::nullopt;
}
void SessionRoomObjects::save(std::ostream& out,const Session& session) const {
    // Do not silently drop stale registrations during save. The lifetime owner
    // must remove them at the destruction/detachment phase first.
    for (const auto& binding:bindings_) if (!resolve(session,binding.reference)) throw std::runtime_error("cannot save stale Session room binding");
    out<<"<room-bindings level=\""<<level_<<"\" next=\""<<next_<<"\">\n";
    for (const auto& binding:bindings_) {
        const auto& value=binding.object;
        out<<"<object identity=\""<<value.identity<<"\" kind=\""<<unsigned(binding.reference.kind)<<"\" subject=\""<<binding.reference.subject
           <<"\" assigned=\""<<value.assigned_room<<"\" global=\""<<value.global<<"\" id=\""<<escape(value.id)<<"\"/>\n";
    }
    out<<"</room-bindings>\n";if (!out) throw std::runtime_error("cannot write Session room bindings");
}
SessionRoomObjects SessionRoomObjects::load(const XmlNode& node,const Session& session) {
    if (node.tag!="room-bindings" || node.attributes.size()!=2 || node.children.size()>100000 || node.text.find_first_not_of(" \t\r\n")!=std::string::npos)
        throw std::runtime_error("invalid Session room bindings");
    const auto level=number<std::uint64_t>(node,"level");if (!level) throw std::runtime_error("invalid room binding level");
    SessionRoomObjects result(level);result.next_=number<std::uint64_t>(node,"next");if (!result.next_) throw std::runtime_error("invalid room binding sequence");
    std::set<std::uint64_t> identities;std::set<std::pair<unsigned,std::uint64_t>> references;
    for (const auto& child:node.children) {
        if (child.tag!="object" || child.attributes.size()!=6 || !child.children.empty() || child.text.find_first_not_of(" \t\r\n")!=std::string::npos)
            throw std::runtime_error("invalid Session room binding record");
        const auto kind=number<unsigned>(child,"kind");const auto global=number<unsigned>(child,"global");
        const auto id=child.attributes.find("id");
        SessionRoomObjectRef ref{SessionRoomObjectKind(kind),number<std::uint64_t>(child,"subject")};
        const auto identity=number<std::uint64_t>(child,"identity");const auto assigned=number<std::int32_t>(child,"assigned");
        if (kind>unsigned(SessionRoomObjectKind::keeper) || !identity || identity>=result.next_ || !identities.insert(identity).second ||
            !references.emplace(kind,ref.subject).second || global>1 || bool(global)!=(ref.kind==SessionRoomObjectKind::player) ||
            id==child.attributes.end() || id->second.size()>4096 || assigned< -1 || assigned>255)
            throw std::runtime_error("invalid Session room binding identity");
        const auto live=resolve(session,ref);if (!live) throw std::runtime_error("saved room binding has no Session object");
        result.bindings_.push_back({ref,{identity,level,assigned,id->second,bool(global),live->position}});
    }
    return result;
}
}
