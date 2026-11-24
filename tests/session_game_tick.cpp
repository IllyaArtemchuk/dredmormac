#include "dredmor/session_game_tick.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("Session tick assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
Session make_session(std::uint32_t seed) {std::istringstream in("######\n#@...#\n#....#\n######\n");return Session(World::read(in),make_player({3,2,2}),seed);}
GameTickState make_tick() {
    GameTickState tick;tick.level=TickLevelState{6,4,true,{1},std::vector<std::vector<std::uint64_t>>(24)};
    tick.level->tile_effects[8]={17};tick.effect_durations[17]=1;
    tick.player.position={1,1};tick.player.respawn_counter=100;tick.player.action_delay=2;
    tick.spell_cooldowns={0,1,4,-1};tick.colour_strings={"first","second"};
    tick.announced_full_inventory=true;tick.game_message="before";return tick;
}
std::string state_bytes(const GameTickState& tick) {
    std::ostringstream out;out<<tick.announced_full_inventory<<' '<<tick.slow_time_counter<<' '<<tick.tutorial<<' '<<tick.game_message<<' '<<tick.last_autosave;
    out<<' '<<tick.player.position.x<<' '<<tick.player.position.y<<' '<<tick.player.dead<<' '<<tick.player.respawn_counter<<' '<<tick.player.action_delay;
    out<<' '<<bool(tick.level);if (tick.level) {
        out<<' '<<tick.level->width<<' '<<tick.level->height<<' '<<tick.level->updates_player;
        for (auto object:tick.level->objects) out<<" o"<<object;
        for (const auto& tile:tick.level->tile_effects) {out<<" t";for (auto effect:tile) out<<' '<<effect;}
    }
    for (const auto& effect:tick.effect_durations) out<<" e"<<effect.first<<' '<<effect.second;
    for (const auto& text:tick.colour_strings) out<<" c"<<text;
    for (auto cooldown:tick.spell_cooldowns) out<<" s"<<cooldown;
    for (const auto& move:tick.teleports) out<<" m"<<move.object<<' '<<move.level<<' '<<move.position.x<<' '<<move.position.y;
    return out.str();
}
Actor construct(GameRandom& random) {
    auto actor=make_player({1,1,1},"constructed");actor.id=0;
    actor.zorkmids=std::int32_t(random.libc.bounded(100));
    actor.monster_flags=random.mt.u32();actor.sprite_tint=int(random.libc.bounded(360));
    actor.health=actor.secondary[0]=20+int(random.normal.sample(5,1,random.mt));return actor;
}
RoomMonsterRecord record(Position point) {RoomMonsterRecord value;value.name="fixture";value.placement.relative=point;return value;}
void check_random(const Session& session,const GameRandom& expected) {
    const auto root=parse_xml(saved(session));const auto* rng=root.first("random");check(rng);
    check(rng->attribute("libc")==std::to_string(expected.libc.state()) && rng->attribute("index")==std::to_string(expected.mt.state().index));
    const auto normal=expected.normal.state();std::uint32_t spare;std::memcpy(&spare,&normal.spare,4);
    check(rng->attribute("normal-available")==std::to_string(normal.available) && rng->attribute("normal-spare")==std::to_string(spare));
    std::istringstream words(rng->text);for (auto word:expected.mt.state().words) {std::uint32_t actual;check(bool(words>>actual) && word==actual);}
}
}
int main() {
    try {
        for (std::uint32_t seed=1;seed<=64;++seed) {
            auto session=make_session(seed);auto tick=make_tick();auto reference=tick;GameRandom expected;expected.seed(seed);
            std::vector<Actor> actors;std::vector<TickRequestKind> actual_trace,reference_trace;
            auto create_reference=[&](Position point) {
                const auto plan=plan_room_population(RoomPopulationRecord{record(point)},{0,0},expected.libc);
                auto actor=construct(expected);actor.position=plan.position;actors.push_back(actor);reference.level->objects.push_back(actors.size()+1);
            };
            GameTickHooks oracle;oracle.ticks=[] {return 2U;};oracle.perform=[&](const TickRequest& request) {
                reference_trace.push_back(request.kind);
                if (request.kind==TickRequestKind::object_update && request.object==1) create_reference({2,1});
                if (request.kind==TickRequestKind::spawn_monster) create_reference({3,1});
            };
            run_game_tick(reference,expected,oracle);
            SessionGameTickHooks hooks;hooks.ticks=[](auto&,auto&) {return 2U;};
            hooks.perform=[&](Session& candidate,GameTickState& pending,const TickRequest& request) {
                actual_trace.push_back(request.kind);
                if ((request.kind==TickRequestKind::object_update && request.object==1) || request.kind==TickRequestKind::spawn_monster) {
                    const auto point=request.kind==TickRequestKind::spawn_monster?Position{3,1}:Position{2,1};
                    auto id=candidate.create_room_monster(record(point),[](const auto&,auto& random,auto&) {return construct(random);});
                    check(id);pending.level->objects.push_back(*id);
                }
            };
            SessionGameTickExecutor::run(session,tick,hooks);
            check(actual_trace==reference_trace && state_bytes(tick)==state_bytes(reference) && session.enemies().size()==2);
            for (std::size_t i=0;i<actors.size();++i) {
                const auto& actor=session.enemies()[i];check(actor.health==actors[i].health && actor.zorkmids==actors[i].zorkmids && actor.monster_flags==actors[i].monster_flags && actor.sprite_tint==actors[i].sprite_tint);
            }
            check_random(session,expected);
            auto restored_bytes=saved(session);std::istringstream in(restored_bytes);auto restored=Session::load(in);check(saved(restored)==restored_bytes);
            // Fail after both constructors and the driver's incidental MT draw.
            for (bool clock_failure:{false,true}) {
                auto failed=make_session(seed);auto state=make_tick();const auto original=saved(failed),original_tick=state_bytes(state);
                auto failing=hooks;failing.perform=[&](auto& candidate,auto& pending,const auto& request) {
                    hooks.perform(candidate,pending,request);
                    if (!clock_failure && request.kind==TickRequestKind::post_game_tick) throw std::runtime_error("late object failure");
                };
                failing.ticks=[&](auto&,auto&)->std::uint32_t {throw std::runtime_error("clock failure");};
                bool rejected=false;try {SessionGameTickExecutor::run(failed,state,failing);} catch(const std::runtime_error&) {rejected=true;}
                check(rejected && saved(failed)==original && state_bytes(state)==original_tick);
            }
        }
        auto session=make_session(7);auto tick=make_tick();const auto before=saved(session),state_before=state_bytes(tick);
        SessionGameTickHooks recursive;recursive.ticks=[](auto&,auto&) {return 0U;};recursive.perform=[&](auto& candidate,auto& pending,const auto&) {SessionGameTickExecutor::run(candidate,pending,recursive);};
        bool rejected=false;try {SessionGameTickExecutor::run(session,tick,recursive);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && saved(session)==before && state_bytes(tick)==state_before);
        // A null level still commits the initial fix/clear callback and slow counter.
        GameTickState absent;absent.slow_time_counter=1;unsigned calls=0;
        SessionGameTickHooks empty;empty.ticks=[](auto&,auto&)->std::uint32_t {throw std::runtime_error("unexpected clock");};
        empty.perform=[&](auto& candidate,auto&,const auto& request) {
            check(request.kind==TickRequestKind::fix_safe_teleports);CustomRoomDefinition object;object.placement.relative={2,1};object.sprite.image="fixture.spr";
            candidate.create_custom_object(object);++calls;
        };
        SessionGameTickExecutor::run(session,absent,empty);check(calls==1 && absent.slow_time_counter==0 && session.custom_objects().size()==1);
        std::cout<<"Session tick shares libc/MT/normal-cache state with constructors, preserves live ordering and atomically rolls back late/recursive failures\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
