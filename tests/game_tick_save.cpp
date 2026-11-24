#include "dredmor/session_game_tick.hpp"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("tick save assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
std::string saved(const GameTickState& tick) {std::ostringstream out;write_game_tick_state(out,tick);return out.str();}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
template<class F> bool rejects(F body) {try {body();} catch(const std::exception&) {return true;}return false;}
GameTickState state() {
    GameTickState tick;tick.announced_full_inventory=true;tick.slow_time_counter=INT32_MIN;tick.tutorial=-1;
    tick.game_message="line\r\n<&>\t\"quotes\" 🐉";tick.last_autosave=UINT32_MAX;
    tick.player={{-1,INT32_MAX},INT32_MIN,INT32_MAX,INT32_MIN};
    tick.level=TickLevelState{2,2,true,{7,UINT64_MAX,7},{{1,2,1},{},{3},{2}}};
    tick.effect_durations={{1,-1},{2,INT32_MIN},{3,0},{UINT64_MAX,INT32_MAX}};
    tick.colour_strings={"",tick.game_message,"one\rtwo\nthree"};tick.spell_cooldowns={INT32_MIN,-1,0,1,INT32_MAX};
    tick.teleports={{UINT64_MAX,7,{-1,INT32_MIN}},{1,UINT64_MAX,{INT32_MAX,0}}};return tick;
}
}
int main() {
    try {
        const auto initial=state();const auto bytes=saved(initial);check(saved(read_game_tick_state(parse_xml(bytes)))==bytes);
        auto absent=initial;absent.level.reset();check(saved(read_game_tick_state(parse_xml(saved(absent))))==saved(absent));
        auto empty=initial;empty.level=TickLevelState{0,3,false,{}, {}};check(saved(read_game_tick_state(parse_xml(saved(empty))))==saved(empty));
        const auto root=parse_xml(bytes);
        auto bad=root;bad.attributes["version"]="2";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.attributes["announced"]="2";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[1].attributes["delay"]="2147483648";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[2].attributes["height"]="-2";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[2].children[0].text="7 1 garbage";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[2].children[1].children.pop_back();check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[3].children.push_back(bad.children[3].children[0]);check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[3].children.erase(bad.children[3].children.begin());check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[6].children[0].attributes["unexpected"]="1";check(rejects([&]{read_game_tick_state(bad);}));
        bad=root;bad.children[5].children.push_back(XmlNode{});check(rejects([&]{read_game_tick_state(bad);}));
        auto broken=initial;broken.level->width=1000001;std::ostringstream output;
        check(rejects([&]{write_game_tick_state(output,broken);}) && output.str().empty());
        broken=initial;broken.game_message=std::string(1,'\1');check(rejects([&]{write_game_tick_state(output,broken);}) && output.str().empty());
        broken=initial;broken.game_message=std::string(1,char(0xff));check(rejects([&]{write_game_tick_state(output,broken);}) && output.str().empty());
        // Continue a shared-RNG Session tick sequence after a mid-run snapshot.
        // Only explicit controlled constructor/subsystem bodies are supplied.
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session uninterrupted(World::read(map),make_player({3,2,2}),7),resumed=uninterrupted;
        GameTickState direct;direct.level=TickLevelState{5,4,true,{1},std::vector<std::vector<std::uint64_t>>(20)};direct.player.position={1,1};
        direct.effect_durations={{42,45},{43,-1}};direct.level->tile_effects[6]={42,43};direct.spell_cooldowns={0,2,100};direct.player.action_delay=2;
        auto restored=direct;unsigned now=0;std::vector<std::string> direct_trace,resumed_trace;
        auto hooks=[&](std::vector<std::string>& trace) {
            SessionGameTickHooks result;result.ticks=[&](auto&,auto&) {return now;};
            result.perform=[&trace](Session& candidate,GameTickState& tick,const TickRequest& request) {
                trace.push_back(std::to_string(int(request.kind))+":"+std::to_string(request.object)+":"+request.text);
                if (request.kind==TickRequestKind::spawn_monster) {
                    RoomMonsterRecord record;record.name="fixture";record.placement.relative={2,1};
                    auto actor=candidate.create_room_monster(record,[](const auto&,auto& random,auto&) {
                        auto value=make_player({1,1,1},"spawned");value.id=0;value.zorkmids=int(random.libc.bounded(100));value.monster_flags=random.mt.u32();return value;
                    });check(actor);tick.level->objects.push_back(*actor);
                }
            };return result;
        };
        const auto first=hooks(direct_trace),second=hooks(resumed_trace);
        for (unsigned turn=0;turn<110;++turn) {
            now=turn*10000;direct.colour_strings.push_back("colour"+std::to_string(turn));restored.colour_strings=direct.colour_strings;
            SessionGameTickExecutor::run(uninterrupted,direct,first);SessionGameTickExecutor::run(resumed,restored,second);
            if (turn==39) {
                std::ostringstream snapshot;SessionGameTickExecutor::save(snapshot,resumed,restored);
                std::istringstream input(snapshot.str());auto loaded=SessionGameTickExecutor::load(input);
                resumed=std::move(loaded.session);restored=std::move(loaded.tick);
                std::ostringstream reloaded;SessionGameTickExecutor::save(reloaded,resumed,restored);check(reloaded.str()==snapshot.str());
            }
            check(saved(uninterrupted)==saved(resumed) && saved(direct)==saved(restored) && direct_trace==resumed_trace);
        }
        check(resumed.enemies().size()==1 && restored.player.respawn_counter==10 && restored.level->tile_effects[6]==std::vector<std::uint64_t>{43});
        auto wrong=restored;wrong.player.position.x=3;std::ostringstream rejected_output;
        check(rejects([&]{SessionGameTickExecutor::save(rejected_output,resumed,wrong);}) && rejected_output.str().empty());
        wrong=restored;wrong.player.dead=1;check(rejects([&]{SessionGameTickExecutor::save(rejected_output,resumed,wrong);}) && rejected_output.str().empty());
        std::ostringstream valid;SessionGameTickExecutor::save(valid,resumed,restored);auto invalid=valid.str();
        const auto geometry=invalid.find("<level width=\"5\"");check(geometry!=std::string::npos);invalid.replace(geometry+14,1,"4");
        std::istringstream invalid_input(invalid);check(rejects([&]{SessionGameTickExecutor::load(invalid_input);}));
        std::cout<<"driver snapshots retain all fields and ordered memberships, reject invalid data before output, and preserve110 shared-RNG ticks across reload\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
