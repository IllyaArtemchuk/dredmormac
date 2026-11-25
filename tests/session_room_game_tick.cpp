#include "dredmor/content.hpp"
#include "dredmor/session_room_game_tick.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("scripted tick assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
std::string bundle(const Session& session,const SessionRoomScriptProfile& profile,const SessionRoomObjects& objects,const RoomRuntimeState& runtime,const GameTickState& tick) {
    std::ostringstream out;SessionRoomScriptExecutor::save(out,session,profile,objects,runtime);write_game_tick_state(out,tick);return out.str();
}
}
int main() {
    try {
        RoomDefinition room;room.rows={"#####","#123#","#...#","#####"};room.source=parse_xml(R"(<room width="5" height="4"><script>
            <condition condition_type="entered"/><action on="success" action_type="remove" id="gate"/>
            <action on="success" action_type="create" create_type="customengraving" at="3" png="new.spr" id="new"/>
            <action on="success" action_type="move" id="player" at="2"/><action on="success" action_type="move" id="player" at="3"/>
            <action on="success" action_type="ticker" text="opened"/></script></room>)");
        auto program=std::make_shared<const RoomScriptProgram>(room.script_program());
        std::vector<DungeonCell> cells(20);for (int y=0;y<4;++y) for (int x=0;x<5;++x) cells[y*5+x].kind=x>0 && x<4 && y>0 && y<3?1:2;
        SessionRoomScriptProfile profile(DungeonLayout(5,4,cells));profile.programs={program};profile.sources={room.source};
        profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(program,&program->scripts)},{0,0},1}};
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session initial(World::read(map),make_player({3,2,2}),7);
        auto definitions=std::make_shared<std::vector<SpellDefinition>>(3);for (auto& spell:*definitions) {spell.name="fixture";spell.type="self";}
        (*definitions)[0].casting.cooldown=(*definitions)[1].casting.cooldown=true;initial.spell_profile(definitions);
        // Seed valid positive and negative persisted cooldowns without running
        // the provisional player-action scheduler in this driver fixture.
        std::ostringstream seed;initial.save(seed);auto bytes=seed.str();const auto version=bytes.find("version=\"7\"");check(version!=std::string::npos);
        bytes.replace(version,11,"version=\"15\"");bytes.insert(bytes.find("</dredmor-session>"),"<spell-cooldowns><cooldown spell='0' remaining='3'/><cooldown spell='1' remaining='-7'/></spell-cooldowns>");
        std::istringstream seed_input(bytes);initial=Session::load(seed_input,{},{},definitions);
        CustomRoomDefinition gate;gate.placement.relative={2,1};gate.placement.id="gate";gate.sprite.image="gate.spr";initial.create_custom_object(gate);
        SessionRoomObjects initial_objects;const auto gate_id=initial_objects.bind(initial,{SessionRoomObjectKind::custom,1},profile.layout).object.identity;
        const auto player_id=initial_objects.bind(initial,{SessionRoomObjectKind::player,initial.player().id},profile.layout).object.identity;
        RoomRuntimeState initial_runtime;GameTickState initial_tick;initial_tick.level=TickLevelState{5,4,true,{},std::vector<std::vector<std::uint64_t>>(20)};
        initial_tick.player.position=initial.player().position;initial_tick.player.action_delay=1;initial_tick.player.respawn_counter=100;
        initial_tick.teleports={{gate_id,1,{1,1}},{900,1,{4,3}}}; // controlled external placement boundary
        std::vector<std::uint64_t> updates,posts;unsigned external_moves=0,script_moves=0,saves=0;
        SessionRoomGameTickHooks hooks;hooks.ticks=[](auto&,auto&) {return 300001U;};
        hooks.perform=[&](Session& candidate,SessionRoomObjects& objects,RoomRuntimeState& runtime,GameTickState& tick,const TickRequest& request,bool managed) {
            auto construct=[](const auto&,auto& random,auto&)->Actor {
                auto actor=make_player({1,1,1},"fixture");actor.id=0;actor.zorkmids=int(random.libc.bounded(100));actor.monster_flags=random.mt.u32();return actor;
            };
            switch (request.kind) {
                case TickRequestKind::object_update:
                    updates.push_back(request.object);
                    if (request.object==gate_id) {
                        RoomMonsterRecord early;early.name="fixture";early.placement.relative={2,2};
                        const auto actor=candidate.create_room_monster(early,construct);check(actor);
                        objects.bind(candidate,{SessionRoomObjectKind::monster,*actor},profile.layout);
                    }
                    break;
                case TickRequestKind::room_triggers:
                    check(managed && objects.find(candidate,0,"gate") && objects.find(candidate,0,"new") && candidate.spell_cooldown(0)==3);break;
                case TickRequestKind::clear_destruction:
                    check(managed && !objects.find(candidate,0,"gate") && tick.teleports.size()==3 && tick.teleports[0].object==900);break;
                case TickRequestKind::spawn_monster: {
                    const auto actor=candidate.create_respawn_monster(profile.layout,{},[&](const auto&,auto& random,auto& counts)->std::optional<Actor> {
                        auto value=construct(RoomPopulationPlan{},random,counts);value.position={1,2};++counts.counts["fixture"];return value;
                    });check(actor);objects.bind(candidate,{SessionRoomObjectKind::monster,*actor},profile.layout,false);break;
                }
                case TickRequestKind::post_game_tick:
                    check(candidate.spell_cooldown(0)==3 && candidate.spell_cooldown(1)==-7 && candidate.player().position.x==1 && !objects.find(candidate,0,"gate"));
                    posts.push_back(request.object);break;
                case TickRequestKind::teleport:
                    check(candidate.spell_cooldown(0)==2 && candidate.spell_cooldown(1)==-7 && tick.player.action_delay==0);
                    if (managed) {check(request.object==player_id && candidate.player().position.x==int(script_moves)+2);++script_moves;}
                    else {check(request.object==900);++external_moves;}
                    break;
                case TickRequestKind::save_game: {
                    check(candidate.player().position.x==3 && candidate.spell_cooldown(0)==2);
                    std::ostringstream payload;SessionRoomScriptExecutor::save(payload,candidate,profile,objects,runtime);write_game_tick_state(payload,tick);++saves;break;
                }
                default:break;
            }
        };
        auto session=initial;auto objects=initial_objects;auto runtime=initial_runtime;auto tick=initial_tick;
        auto pass=SessionRoomGameTickExecutor::run(session,profile,objects,runtime,tick,hooks);
        check(updates==std::vector<std::uint64_t>({1,2,3}) && posts==std::vector<std::uint64_t>({2,3,4,5}));
        check(external_moves==1 && script_moves==2 && saves==1 && pass.player_teleported && pass.presentation.size()==1);
        check(tick.teleports.empty() && tick.player.respawn_counter==1 && tick.spell_cooldowns==std::vector<std::int32_t>({2,-7,0}) && session.spell_cooldown(0)==2);
        check(objects.bindings().size()==4 && session.enemies().size()==2 && tick.level->objects==posts && session.monster_spawn_state().counts.at("fixture")==1);
        const auto before=bundle(initial,profile,initial_objects,initial_runtime,initial_tick);
        for (auto failure:{TickRequestKind::room_triggers,TickRequestKind::clear_destruction,TickRequestKind::director,TickRequestKind::spawn_monster,TickRequestKind::post_game_tick,TickRequestKind::teleport,TickRequestKind::save_game,TickRequestKind::platform_callbacks}) {
            auto failed=initial;auto failed_objects=initial_objects;auto failed_runtime=initial_runtime;auto failed_tick=initial_tick;
            updates.clear();posts.clear();external_moves=script_moves=saves=0;auto failing=hooks;
            failing.perform=[&](auto& owner,auto& registry,auto& scripts,auto& driver,const auto& request,bool managed) {
                hooks.perform(owner,registry,scripts,driver,request,managed);
                if (request.kind==failure && (failure!=TickRequestKind::teleport || managed)) throw std::runtime_error("injected phase failure");
            };
            bool rejected=false;try {SessionRoomGameTickExecutor::run(failed,profile,failed_objects,failed_runtime,failed_tick,failing);} catch(const std::runtime_error&) {rejected=true;}
            check(rejected && bundle(failed,profile,failed_objects,failed_runtime,failed_tick)==before);
        }
        {
            auto failed=initial;auto registry=initial_objects;auto scripts=initial_runtime;auto driver=initial_tick;
            updates.clear();posts.clear();external_moves=script_moves=saves=0;auto failing=hooks;
            failing.ticks=[](auto&,auto&)->std::uint32_t {throw std::runtime_error("injected clock failure");};
            bool rejected=false;try {SessionRoomGameTickExecutor::run(failed,profile,registry,scripts,driver,failing);} catch(const std::runtime_error&) {rejected=true;}
            check(rejected && bundle(failed,profile,registry,scripts,driver)==before);
        }
        // The following pass has no script moves. Clock-time autosave access
        // must still see a finished phase and decremented Session cooldowns.
        SessionRoomGameTickHooks quiet;quiet.ticks=[](auto&,auto&) {return 600002U;};quiet.perform=[&](auto& owner,auto& registry,auto& scripts,auto& driver,const auto& request,bool) {
            if (request.kind==TickRequestKind::save_game) {check(owner.spell_cooldown(0)==1);std::ostringstream out;SessionRoomScriptExecutor::save(out,owner,profile,registry,scripts);write_game_tick_state(out,driver);}
        };
        auto second=SessionRoomGameTickExecutor::run(session,profile,objects,runtime,tick,quiet);check(!second.player_teleported && second.presentation.empty() && session.spell_cooldown(0)==1);
        {
            auto gated=initial;auto registry=initial_objects;auto scripts=initial_runtime;auto driver=initial_tick;driver.level->updates_player=false;
            unsigned player_passes=0,respawns=0;SessionRoomGameTickHooks inactive;inactive.ticks=[](auto&,auto&) {return 1U;};
            inactive.perform=[&](auto&,auto&,auto&,auto&,const auto& request,bool) {
                if (request.kind==TickRequestKind::player_buffs || request.kind==TickRequestKind::player_triggers) ++player_passes;
                if (request.kind==TickRequestKind::spawn_monster) ++respawns;
            };
            SessionRoomGameTickExecutor::run(gated,profile,registry,scripts,driver,inactive);
            check(player_passes==0 && respawns==0 && gated.spell_cooldown(0)==3 && gated.spell_cooldown(1)==-7 && driver.player.respawn_counter==100);
        }
        std::cout<<"scripted outer tick owns all candidates, live registrations, shared RNG, cooldowns, deferred moves and late-failure rollback\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
