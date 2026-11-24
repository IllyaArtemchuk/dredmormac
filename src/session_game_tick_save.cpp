#include "dredmor/session_game_tick.hpp"
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace dredmor {
namespace {
void validate_owner(const Session& session,const GameTickState& tick) {
    if (!tick.level) return;
    if (tick.level->width!=session.world().width() || tick.level->height!=session.world().height() ||
        tick.player.position.x!=session.player().position.x || tick.player.position.y!=session.player().position.y ||
        bool(tick.player.dead)==session.player().alive())
        throw std::runtime_error("game tick snapshot does not match Session geometry/player");
}
}
void SessionGameTickExecutor::save(std::ostream& output,const Session& session,const GameTickState& state) {
    validate_owner(session,state);std::ostringstream payload;
    session.save(payload);write_game_tick_state(payload,state);
    const auto bytes="<dredmor-session-tick version=\"1\">\n"+payload.str()+"</dredmor-session-tick>\n";
    if (bytes.size()>64*1024*1024) throw std::runtime_error("Session tick snapshot exceeds64 MiB");
    output<<bytes;
    if (!output) throw std::runtime_error("cannot write Session game tick snapshot");
}
SessionGameTickSnapshot SessionGameTickExecutor::load(std::istream& input,
    std::shared_ptr<const std::vector<ItemDefinition>> definitions,std::shared_ptr<const std::vector<AbilityDefinition>> abilities,
    std::shared_ptr<const std::vector<SpellDefinition>> spells) {
    std::string bytes;char buffer[8192];
    while (input) {input.read(buffer,sizeof(buffer));bytes.append(buffer,std::size_t(input.gcount()));if (bytes.size()>64*1024*1024) throw std::runtime_error("Session tick snapshot exceeds64 MiB");}
    if (input.bad()) throw std::runtime_error("cannot read Session game tick snapshot");
    const auto root=parse_xml(bytes);
    if (root.tag!="dredmor-session-tick" || root.attributes.size()!=1 || root.attribute("version")!="1" || root.children.size()!=2 ||
        root.children[0].tag!="dredmor-session" || root.children[1].tag!="game-tick" || root.text.find_first_not_of(" \t\r\n")!=std::string::npos)
        throw std::runtime_error("invalid Session game tick snapshot structure");
    auto session=Session::load_node(root.children[0],std::move(definitions),std::move(abilities),std::move(spells));
    auto tick=read_game_tick_state(root.children[1]);validate_owner(session,tick);return {std::move(session),std::move(tick)};
}
}
