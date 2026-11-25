#include "dredmor/session_room_game_tick.hpp"
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace dredmor {
void SessionRoomGameTickExecutor::validate_snapshot(const Session& session,const SessionRoomObjects& objects,const GameTickState& state) {
    if (!objects.player() || !state.level || state.level->width!=session.world_.width() || state.level->height!=session.world_.height() ||
        state.player.position.x!=session.player_.position.x || state.player.position.y!=session.player_.position.y ||
        bool(state.player.dead)==session.player_.alive() || state.level->objects.size()!=objects.bindings().size() ||
        state.spell_cooldowns.size()!=(session.spells_?session.spells_->size():0))
        throw std::runtime_error("scripted tick snapshot does not match Session/registries");
    for (std::size_t i=0;i<state.level->objects.size();++i) if (state.level->objects[i]!=objects.bindings()[i].object.identity)
        throw std::runtime_error("scripted tick snapshot has a different object registration order");
    for (std::size_t i=0;i<state.spell_cooldowns.size();++i) if (state.spell_cooldowns[i]!=session.spell_cooldown(i))
        throw std::runtime_error("scripted tick snapshot has different spell cooldowns");
}
void SessionRoomGameTickExecutor::save(std::ostream& output,const Session& session,const SessionRoomScriptProfile& profile,
    const SessionRoomObjects& objects,const RoomRuntimeState& runtime,const GameTickState& state) {
    validate_snapshot(session,objects,state);
    if (profile.layout.width()!=session.world_.width() || profile.layout.height()!=session.world_.height())
        throw std::runtime_error("scripted tick snapshot has a different prepared profile geometry");
    std::ostringstream payload;
    SessionRoomScriptExecutor::save(payload,session,profile,objects,runtime);write_game_tick_state(payload,state);
    const auto bytes="<dredmor-scripted-tick version=\"1\">\n"+payload.str()+"</dredmor-scripted-tick>\n";
    if (bytes.size()>64*1024*1024) throw std::runtime_error("scripted tick snapshot exceeds64 MiB");
    output<<bytes;if (!output) throw std::runtime_error("cannot write scripted tick snapshot");
}
SessionRoomGameTickSnapshot SessionRoomGameTickExecutor::load(std::istream& input,const SessionRoomScriptProfile& profile,
    std::shared_ptr<const std::vector<ItemDefinition>> definitions,std::shared_ptr<const std::vector<AbilityDefinition>> abilities,
    std::shared_ptr<const std::vector<SpellDefinition>> spells) {
    std::string bytes;char buffer[8192];
    while (input) {input.read(buffer,sizeof(buffer));bytes.append(buffer,std::size_t(input.gcount()));if (bytes.size()>64*1024*1024) throw std::runtime_error("scripted tick snapshot exceeds64 MiB");}
    if (input.bad()) throw std::runtime_error("cannot read scripted tick snapshot");
    const auto root=parse_xml(bytes);
    if (root.tag!="dredmor-scripted-tick" || root.attributes.size()!=1 || root.attribute("version")!="1" || root.children.size()!=2 ||
        root.children[0].tag!="dredmor-scripted-room" || root.children[1].tag!="game-tick" || root.text.find_first_not_of(" \t\r\n")!=std::string::npos)
        throw std::runtime_error("invalid scripted tick snapshot structure");
    auto room=SessionRoomScriptExecutor::load_node(root.children[0],profile,std::move(definitions),std::move(abilities),std::move(spells));
    auto tick=read_game_tick_state(root.children[1]);validate_snapshot(room.session,room.objects,tick);
    return {std::move(room.session),std::move(room.objects),std::move(room.runtime),std::move(tick)};
}
}
