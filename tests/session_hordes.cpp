#include "dredmor/content.hpp"
#include "dredmor/floors.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("session horde assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
struct TemporaryRoot {std::filesystem::path path;~TemporaryRoot() {std::error_code ignored;std::filesystem::remove_all(path,ignored);}};
std::string save(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
World make_world(std::string text="#####\n#@.##\n##.##\n#####\n") {std::istringstream rows(text);return World::read(rows);}
}
int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::invalid_argument("synthetic content fixture root required");
        TemporaryRoot root{std::filesystem::temp_directory_path()/("dredmor-session-hordes-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        std::filesystem::copy(argv[1],root.path,std::filesystem::copy_options::recursive);
        std::ofstream(root.path/"game/monDB.xml")<<R"(<monDB><monster name="Fixture monster"><stats numFig="2" numWiz="1" numRog="1"/>
            <idleSprite down="fixture.spr"/><monster name="Fixture variant" level="1"><palette tint="120"/></monster>
            </monster></monDB>)";
        std::ofstream(root.path/"game/tweakDB.xml")<<R"(<tweakdb><tweak name="monster HP global scale" fval="1"/>
            <tweak name="monster level adder" ival="1"/></tweakdb>)";
        std::ofstream(root.path/"game/text.xml")<<R"(<text><startphoneme text="S"/><phoneme text="x"/>
            <title text="Strong"/><noun text="Noun" plural="People"/><adjective text="Old"/></text>)";
        const auto content=GameContent::load(root.path);
        std::vector<DungeonCell> cells(20);for (auto& cell:cells) cell.kind=2;cells[6].kind=cells[7].kind=cells[12].kind=1;
        DungeonLayout layout(5,4,cells);std::vector<HordeRoomBounds> rooms={{{2,2},{3,3}}};
        RoomHordeRecord record;record.placement.relative={2,2};record.name="Fixture variant";record.minimum=record.additional=0;
        record.placement.id="ignored-horde-id";record.placement.can_push=0;
        auto build=[&](const Session& owner,const RoomPopulationPlan& plan,const HordeWorld& world,const MonsterSpawnState& counts,GameRandom& random,std::uint64_t first) {
            HordeOptions options;options.position=plan.position;options.minimum=plan.minimum;options.additional=plan.additional;options.depth=owner.depth_context().depth;
            if (!plan.name.empty()) options.named=plan.name;
            return content.create_horde(options,world,Difficulty::medium,owner.depth_context().depth,counts,random,first);
        };
        auto queue=[&](Session& owner,const RoomHordeRecord& request) {
            return owner.queue_room_horde(request,layout,rooms,[&](const auto& plan,const auto& world,const auto& counts,auto& rng,auto first) {return build(owner,plan,world,counts,rng,first);});
        };
        Session session(make_world(),make_player({3,2,2}),1);const auto initial=save(session);
        bool rejected=false;
        try {session.queue_room_horde(record,layout,rooms,[](const auto&,const auto&,const auto&,auto& rng,auto)->HordePopulation {rng.libc.next();rng.mt.u32();throw std::runtime_error("constructor failure");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && save(session)==initial);
        const auto first=queue(session,record);check(first && first->members.size()==2 && first->members[0].identity==2 && session.queued_horde_members()==2 && session.enemies().empty());
        rejected=false;try {save(session);} catch(const std::runtime_error&) {rejected=true;}check(rejected);
        rejected=false;try {session.floor_state();} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        auto reference=session;
        auto intruder=content.make_monster("Fixture variant",{2,1},Difficulty::medium);intruder.id=2;
        rejected=false;try {session.spawn_enemy(intruder);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && session.enemies().empty() && session.queued_horde_members()==2);
        rejected=false;
        try {session.queue_room_horde(record,layout,rooms,[&](const auto& plan,const auto& world,const auto& counts,auto& rng,auto id) {
            auto population=build(session,plan,world,counts,rng,id);population.result.members[1].leader=999;return population;
        });} catch(const std::invalid_argument&) {rejected=true;}check(rejected && session.queued_horde_members()==2);
        const auto second=queue(session,record);queue(reference,record);
        check(second && second->members.size()==2 && second->members[0].identity==4 && session.queued_horde_members()==4 && session.enemies().empty());
        check(session.place_queued_hordes()==std::vector<std::uint64_t>({2,3,4,5}));reference.place_queued_hordes();check(save(session)==save(reference));
        check(!session.queued_horde_members() && session.enemies().size()==4 && session.monster_spawn_state().counts.empty());
        for (const auto& actor:session.enemies()) check(actor.position.x==2 && actor.position.y==2 && actor.horde &&
            actor.room_local_id.empty() && actor.can_push==1 && !actor.room_spawned);
        check(session.enemies()[0].horde->followers==std::vector<std::uint64_t>{3} && session.enemies()[1].horde->leader==2 && session.enemies()[2].horde->followers==std::vector<std::uint64_t>{5});
        const auto encoded=save(session);check(encoded.find("version=\"24\"")!=std::string::npos);
        std::istringstream input(encoded);auto loaded=Session::load(input);check(save(loaded)==encoded);
        session.step(Action::wait);loaded.step(Action::wait);check(save(session)==save(loaded));
        auto rejects=[](const std::string& data) {try {std::istringstream in(data);Session::load(in);return false;} catch(const std::runtime_error&) {return true;}};
        auto invalid=encoded;auto at=invalid.find("leader=\"2\"");check(at!=std::string::npos);invalid.replace(at,10,"leader=\"999\"");check(rejects(invalid));
        invalid=encoded;at=invalid.find("<followers>3 ");check(at!=std::string::npos);invalid.replace(at,13,"<followers>3 3 ");check(rejects(invalid));
        invalid=encoded;at=invalid.find("version=\"24\"");invalid.replace(at,12,"version=\"23\"");check(rejects(invalid));
        // A fixed leader can share the player's square; that existing square
        // occupancy rejects the follower even though queued leaders do not.
        Session occupied(make_world("#####\n#..##\n##@##\n#####\n"),make_player({3,2,2}),1);
        const auto blocked=queue(occupied,record);check(blocked && blocked->members.size()==1);occupied.place_queued_hordes();
        check(occupied.enemies()[0].position.x==occupied.player().position.x && occupied.enemies()[0].position.y==occupied.player().position.y);
        std::istringstream overlap(save(occupied));check(save(Session::load(overlap))==save(occupied));
        // Fixed origins need not be floor, unlike follower sites.
        Session wall(make_world("#####\n#@.##\n#####\n#####\n"),make_player({3,2,2}),1);layout.at(2,2).kind=2;
        auto no_followers=record;no_followers.minimum=-3;check(queue(wall,no_followers)->members.size()==1);wall.place_queued_hordes();
        std::istringstream wall_save(save(wall));check(save(Session::load(wall_save))==save(wall));layout.at(2,2).kind=1;
        // Cached floors retain complete group links and colours.
        std::istringstream original(encoded);auto travelling=Session::load(original);SessionFloor empty(make_world());empty.depth.depth=1;
        auto previous=travelling.swap_floor(std::move(empty),{1,1});FloorRegistry registry;registry.add_floor(travelling,std::move(previous));
        std::ostringstream run;registry.save(travelling,run);std::istringstream run_input(run.str());auto restored=FloorRegistry::load(run_input);
        check(restored.second.cached().size()==1 && restored.second.cached()[0].enemies.size()==4 && restored.second.cached()[0].enemies[1].horde->leader==2);
        std::ostringstream again;restored.second.save(restored.first,again);check(again.str()==run.str());
        std::istringstream spacious_rows("#############\n#@..........#\n#...........#\n#...........#\n#...........#\n#...........#\n#############\n");
        Session spacious(World::read(spacious_rows),make_player({3,2,2}),1);
        std::vector<DungeonCell> spacious_cells(91);for (int y=0;y<7;++y) for (int x=0;x<13;++x) spacious_cells[y*13+x].kind=(x && y && x<12 && y<6)?1:2;
        DungeonLayout spacious_layout(13,7,std::move(spacious_cells));std::vector<HordeRoomBounds> spacious_rooms={{{0,0},{13,7}}};
        auto distant=record;distant.placement.relative={11,5};
        for (int i=0;i<2;++i) spacious.queue_room_horde(distant,spacious_layout,spacious_rooms,[&](const auto& plan,const auto& world,const auto& counts,auto& rng,auto id) {return build(spacious,plan,world,counts,rng,id);});
        spacious.place_queued_hordes();check(spacious.enemies().size()==4);
        for (const auto& actor:spacious.enemies()) check(actor.position.x!=2 || actor.position.y!=1);
        spacious.step(Action::east);spacious.step(Action::wait);check(spacious.player().position.x==2 && spacious.player().position.y==1);
        std::cout<<"horde placement batches preserve pending occupancy, duplicate sites, group links/colour/stats through save24/floor cache and rollback failed construction\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
