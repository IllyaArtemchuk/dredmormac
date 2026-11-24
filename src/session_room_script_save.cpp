#include "dredmor/session_room_scripts.hpp"
#include <charconv>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace dredmor {
namespace {
std::uint64_t signature(const SessionRoomScriptProfile& profile) {
    if (profile.sources.size()!=profile.programs.size() || profile.rooms.size()!=profile.programs.size())
        throw std::invalid_argument("saved room script profile requires authoritative sources");
    std::uint64_t hash=14695981039346656037ULL;
    auto integer=[&](std::uint64_t value) {for (unsigned byte=0;byte<8;++byte) {hash^=value&255U;hash*=1099511628211ULL;value>>=8;}};
    auto text=[&](std::string_view value) {integer(value.size());for (unsigned char c:value) {hash^=c;hash*=1099511628211ULL;}};
    std::function<void(const XmlNode&)> source=[&](const XmlNode& node) {
        text(node.tag);text(node.text);integer(node.attributes.size());
        for (const auto& [key,value]:node.attributes) {text(key);text(value);}
        integer(node.attribute_order.size());for (const auto& key:node.attribute_order) text(key);
        integer(node.children.size());for (const auto& child:node.children) source(child);
    };
    integer(profile.layout.width());integer(profile.layout.height());
    for (const auto& cell:profile.layout.cells()) {integer(cell.kind);integer(cell.room_id);integer(cell.door_flags);integer(cell.flags);integer(cell.stair_link);}
    for (const auto& node:profile.sources) source(node);
    // Includes origins, template/room identities and all linked conditions and
    // actions. Empty state yields the same signature after any progress.
    std::ostringstream linked;write_room_runtime_state(linked,RoomRuntimeState{},profile.rooms);text(linked.str());
    return hash;
}
}
void SessionRoomScriptExecutor::save(std::ostream& out,const Session& session,const SessionRoomScriptProfile& profile,
    const SessionRoomObjects& objects,const RoomRuntimeState& runtime) {
    // Prepare all children before writing, so a failed validator cannot expose
    // an apparently complete wrapper to an atomic file-write caller.
    std::ostringstream payload;session.save(payload);objects.save(payload,session);write_room_runtime_state(payload,runtime,profile.rooms);
    const auto key=signature(profile);
    out<<"<dredmor-scripted-room version=\"1\" profile=\""<<key<<"\">\n"<<payload.str()<<"</dredmor-scripted-room>\n";
    if (!out) throw std::runtime_error("cannot write scripted room snapshot");
}
SessionRoomScriptSnapshot SessionRoomScriptExecutor::load(std::istream& input,const SessionRoomScriptProfile& profile,
    std::shared_ptr<const std::vector<ItemDefinition>> definitions,std::shared_ptr<const std::vector<AbilityDefinition>> abilities,
    std::shared_ptr<const std::vector<SpellDefinition>> spells) {
    std::string bytes;char buffer[8192];
    while (input) {input.read(buffer,sizeof(buffer));bytes.append(buffer,std::size_t(input.gcount()));if (bytes.size()>64*1024*1024) throw std::runtime_error("scripted room save exceeds 64 MiB");}
    if (input.bad()) throw std::runtime_error("cannot read scripted room snapshot");
    const auto node=parse_xml(bytes);
    if (node.tag!="dredmor-scripted-room" || node.attributes.size()!=2 || node.attribute("version")!="1" || node.children.size()!=3 ||
        node.children[0].tag!="dredmor-session" || node.children[1].tag!="room-bindings" || node.children[2].tag!="room-runtime" ||
        node.text.find_first_not_of(" \t\r\n")!=std::string::npos) throw std::runtime_error("invalid scripted room save structure");
    const auto key=node.attribute("profile");std::uint64_t saved{};const auto parsed=std::from_chars(key.data(),key.data()+key.size(),saved);
    if (parsed.ec!=std::errc{} || parsed.ptr!=key.data()+key.size() || saved!=signature(profile)) throw std::runtime_error("scripted room save belongs to another profile");
    auto session=Session::load_node(node.children[0],std::move(definitions),std::move(abilities),std::move(spells));
    if (session.world().width()!=profile.layout.width() || session.world().height()!=profile.layout.height()) throw std::runtime_error("scripted room save geometry mismatch");
    auto objects=SessionRoomObjects::load(node.children[1],session);
    if (!objects.player()) throw std::runtime_error("scripted room save is missing its player binding");
    auto runtime=read_room_runtime_state(node.children[2],profile.rooms);
    return {std::move(session),std::move(objects),std::move(runtime)};
}
}
