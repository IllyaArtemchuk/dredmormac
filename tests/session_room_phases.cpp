#include "dredmor/content.hpp"
#include "dredmor/game_tick.hpp"
#include "dredmor/session_game_tick.hpp"
#include "dredmor/session_room_scripts.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("room phase assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
template<class F> bool rejects(F body) {try {body();} catch(const std::exception&) {return true;}return false;}
std::string saved(const Session& value) {std::ostringstream out;value.save(out);return out.str();}
std::string without_random(const Session& value) {
    auto bytes=saved(value);const auto begin=bytes.find("<random "),end=bytes.find("</random>",begin);
    check(begin!=std::string::npos && end!=std::string::npos);bytes.erase(begin,end+9-begin);return bytes;
}
int main() {
    try {
        RoomDefinition room;room.rows={"#####","#123#","#####"};
        room.source=parse_xml(R"(<room width="5" height="3"><script>
            <condition condition_type="entered"/>
            <action on="success" action_type="remove" id="gate"/>
            <action on="success" action_type="create" create_type="customblocker" at="3" png="new.spr" id="new"/>
            <action on="success" action_type="move" id="player" at="2"/>
            <action on="success" action_type="move" id="player" at="3"/></script>
            <script><condition condition_type="at" id="gate" at="2"/>
            <action on="success" action_type="ticker" text="still visible"/></script></room>)");
        auto program=std::make_shared<const RoomScriptProgram>(room.script_program());
        std::vector<DungeonCell> cells(15);for (std::size_t i=0;i<cells.size();++i) cells[i].kind=i>=5 && i<10?1:2;
        SessionRoomScriptProfile profile(DungeonLayout(5,3,cells));profile.programs={program};profile.sources={room.source};
        profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(program,&program->scripts)},{0,0},1}};
        std::istringstream rows("#####\n#@..#\n#####\n");Session committed(World::read(rows),make_player({3,2,2}),7);
        CustomRoomDefinition gate;gate.placement.relative={2,1};gate.placement.id="gate";gate.sprite.image="gate.spr";committed.create_custom_object(gate);
        SessionRoomObjects committed_objects;const auto gate_id=committed_objects.bind(committed,{SessionRoomObjectKind::custom,1},profile.layout).object.identity;
        const auto player_id=committed_objects.bind(committed,{SessionRoomObjectKind::player,committed.player().id},profile.layout).object.identity;
        RoomRuntimeState committed_runtime;const auto before=saved(committed);
        auto direct=committed;auto direct_objects=committed_objects;auto direct_runtime=committed_runtime;
        const auto direct_pass=SessionRoomScriptExecutor::run(direct,profile,direct_objects,direct_runtime);
        auto session=committed;auto objects=committed_objects;auto runtime=committed_runtime;
        auto pending=SessionRoomScriptExecutor::evaluate(session,profile,objects,runtime);
        check(pending.stage==SessionRoomScriptStage::evaluated && pending.destruction==std::vector<std::uint64_t>{gate_id} && pending.moves.size()==2);
        check(objects.find(session,0,"gate") && objects.find(session,0,"new") && session.player().position.x==1);
        check(pending.pass.presentation.size()==1 && pending.pass.presentation[0].text=="still visible");
        std::ostringstream rejected_output;check(rejects([&]{session.save(rejected_output);}) && rejected_output.str().empty());
        check(rejects([&]{session.floor_state();}) && rejects([&]{SessionRoomScriptExecutor::place_next(session,objects,pending);}));
        check(rejects([&]{SessionRoomScriptExecutor::evaluate(session,profile,objects,runtime);}));
        SessionRoomScriptExecutor::clear_destruction(session,objects,pending);
        check(!objects.find(session,0,"gate") && objects.find(session,0,"new") && session.player().position.x==1);
        check(rejects([&]{SessionRoomScriptExecutor::finish(session,objects,pending);}) && rejects([&]{saved(session);}));
        SessionRoomScriptExecutor::place_next(session,objects,pending);check(session.player().position.x==2);
        SessionRoomScriptExecutor::place_next(session,objects,pending);check(session.player().position.x==3 && rejects([&]{saved(session);}));
        const auto pass=SessionRoomScriptExecutor::finish(session,objects,pending);
        check(pass.player_teleported && direct_pass.player_teleported && saved(session)==saved(direct));
        check(pending.stage==SessionRoomScriptStage::finished && rejects([&]{SessionRoomScriptExecutor::place_next(session,objects,pending);}));
        check(saved(committed)==before); // only candidate snapshots were mutated
        // Compose these phases with the recovered outer GameTick and its
        // candidate-owned RNG. Virtual object/player/director bodies remain
        // controlled boundaries, rather than the provisional enemy scheduler.
        session=committed;objects=committed_objects;runtime=committed_runtime;
        GameTickState tick;tick.level=TickLevelState{5,3,true,{gate_id,player_id},std::vector<std::vector<std::uint64_t>>(15)};
        tick.player.position=session.player().position;tick.player.action_delay=1;tick.spell_cooldowns={3,0,-1};
        std::optional<SessionRoomScriptQueue> queue;
        std::vector<std::uint64_t> updates,posts;unsigned moves=0,saves=0;bool finished=false;
        SessionGameTickHooks hooks;
        hooks.perform=[&](Session& candidate,GameTickState& pending,const TickRequest& request) {
            switch (request.kind) {
                case TickRequestKind::object_update:updates.push_back(request.object);break;
                case TickRequestKind::room_triggers:
                    queue=SessionRoomScriptExecutor::evaluate(candidate,profile,objects,runtime);break;
                case TickRequestKind::clear_destruction:
                    SessionRoomScriptExecutor::clear_destruction(candidate,objects,*queue);
                    for (const auto& move:queue->moves) pending.teleports.push_back({move.subject,queue->level,move.point});
                    break;
                case TickRequestKind::post_game_tick:
                    check(!objects.find(candidate,0,"gate") && candidate.player().position.x==1 && pending.spell_cooldowns[0]==3);
                    posts.push_back(request.object);break;
                case TickRequestKind::teleport:
                    check(request.object==player_id && pending.spell_cooldowns[0]==2 && pending.player.action_delay==0);
                    check(request.position.x==int(moves)+2);SessionRoomScriptExecutor::place_next(candidate,objects,*queue);
                    pending.player.position=candidate.player().position;++moves;break;
                case TickRequestKind::save_game:
                    check(finished && without_random(candidate)==without_random(direct));++saves;break;
                default:break;
            }
        };
        hooks.ticks=[&](Session& candidate,GameTickState&) {
            if (!finished) {check(moves==2);SessionRoomScriptExecutor::finish(candidate,objects,*queue);finished=true;}
            return 300001U;
        };
        // The binding/runtime copies belong to this outer fixture transaction.
        // Refresh the driver candidate, never its committed owner.
        const auto perform=hooks.perform;
        hooks.perform=[&](Session& candidate,GameTickState& pending,const TickRequest& request) {
            perform(candidate,pending,request);
            if (request.kind==TickRequestKind::room_triggers || request.kind==TickRequestKind::clear_destruction) {
                pending.level->objects.clear();for (const auto& binding:objects.bindings()) pending.level->objects.push_back(binding.object.identity);
            }
        };
        SessionGameTickExecutor::run(session,tick,hooks);
        check(updates==std::vector<std::uint64_t>({gate_id,player_id}) && posts==std::vector<std::uint64_t>({player_id,3}));
        check(finished && saves==1 && moves==2 && without_random(session)==without_random(direct) && saved(session)!=saved(direct) && tick.teleports.empty());
        std::cout<<"staged live scripts preserve pending lookup, destruction/Post/cooldown/teleport ordering and save guards\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
