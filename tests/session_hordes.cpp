#include "dredmor/content.hpp"
#include "dredmor/floors.hpp"
#include "dredmor/session_room_scripts.hpp"
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
        // Script Create shares the same placement phase as Move actions. Newly
        // constructed groups are absent from registration/occupancy until that
        // phase; two Create calls can choose the same follower square.
        Session scripted(make_world(),make_player({3,2,2}),1);
        CustomRoomDefinition gate;gate.placement.relative={2,1};gate.placement.id="gate";gate.sprite.image="gate.spr";
        scripted.create_custom_object(gate);
        RoomDefinition script_room;script_room.rows={"#####","#..##","##1##","#####"};
        script_room.source=parse_xml(R"(<room width="5" height="4"><script>
            <action on="success" action_type="move" id="gate" x="1" y="2"/>
            <action on="success" action_type="create" create_type="horde" name="Fixture variant" min="0" additional="0" at="1" id="ignored-script-horde"/>
            <action on="success" action_type="create" create_type="customengraving" x="2" y="1" png="later.spr" id="later"/>
            <action on="success" action_type="move" id="player" at="1"/>
            <action on="success" action_type="create" create_type="horde" name="Fixture variant" min="0" additional="0" at="1" id="ignored-script-horde"/>
            <action on="success" action_type="remove" id="gate"/></script></room>)");
        auto script_program=std::make_shared<const RoomScriptProgram>(script_room.script_program());
        SessionRoomScriptProfile script_profile(layout);script_profile.programs={script_program};script_profile.sources={script_room.source};
        script_profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(script_program,&script_program->scripts)},{0,0},1}};
        SessionRoomObjects script_objects;script_objects.bind(scripted,{SessionRoomObjectKind::player,scripted.player().id},layout);
        const auto gate_object=script_objects.bind(scripted,{SessionRoomObjectKind::custom,1},layout).object.identity;
        check(gate_object==2); // same number as the next actor, different domain
        RoomRuntimeState script_runtime;SessionRoomScriptHooks script_hooks;unsigned groups=0;
        script_hooks.queue_horde=[&](auto& owner,const auto& definition,auto origin) {
            if (groups) for (const auto& actor:owner.enemies()) check(actor.position.x==-1 && actor.position.y==-1);
            ++groups;check(content.queue_script_horde(owner,definition,layout,rooms,origin));
        };
        const auto scripted_before=save(scripted);auto failed_objects=script_objects;auto failed_runtime=script_runtime;
        auto failure=script_hooks;failure.queue_horde=[&](auto& owner,const auto& definition,auto origin) {
            content.queue_script_horde(owner,definition,layout,rooms,origin);throw std::runtime_error("script horde resource failure");
        };
        rejected=false;try {SessionRoomScriptExecutor::run(scripted,script_profile,failed_objects,failed_runtime,failure);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && save(scripted)==scripted_before && failed_objects.bindings().size()==2 && failed_runtime.triggers.success_counts.empty());
        auto script_result=SessionRoomScriptExecutor::run(scripted,script_profile,script_objects,script_runtime,script_hooks);
        check(groups==2 && script_result.player_teleported && scripted.enemies().size()==4 && scripted.custom_objects().size()==1 && scripted.custom_objects()[0].room_local_id=="later" && !scripted.queued_horde_members());
        check(scripted.player().position.x==2 && scripted.player().position.y==2 && script_objects.bindings().size()==6 && script_objects.unplaced().empty());
        for (const auto& actor:scripted.enemies()) check(actor.position.x==2 && actor.position.y==2 && actor.horde && actor.room_local_id.empty());
        check(!script_objects.object(scripted,gate_object));
        check(script_objects.bindings()[1].object.id=="later" && script_objects.bindings()[1].object.identity==5 && script_objects.bindings()[2].object.identity==3);
        for (const auto& binding:script_objects.bindings()) if (binding.reference.kind==SessionRoomObjectKind::monster) check(binding.object.assigned_room==-1 && binding.object.id.empty());
        Session direct(make_world(),make_player({3,2,2}),1);
        for (const auto& created:script_program->created_records) {
            if (const auto* horde=std::get_if<RoomHordeRecord>(&created)) check(content.queue_script_horde(direct,*horde,layout,rooms));
            else check(direct.create_custom_object(std::get<CustomRoomDefinition>(created)));
        }
        direct.place_queued_hordes();direct.swap_floor(direct.floor_state(),{2,2});check(save(direct)==save(scripted));
        std::ostringstream script_save;SessionRoomScriptExecutor::save(script_save,scripted,script_profile,script_objects,script_runtime);
        std::istringstream script_input(script_save.str());auto script_restored=SessionRoomScriptExecutor::load(script_input,script_profile);
        check(save(script_restored.session)==save(scripted) && script_restored.objects.bindings().size()==6);
        // Script removal runs destructor link cleanup, not OnDeath's panic or
        // quest callbacks. Orphan followers retain their own stats/names/quest.
        auto removal_program=std::make_shared<RoomScriptProgram>();RoomScriptDefinition remove_script;
        RoomAction remove;remove.kind=RoomActionKind::remove;remove.object_id="victim";remove_script.success={remove};removal_program->scripts={remove_script};
        SessionRoomScriptProfile removal_profile(layout);removal_profile.programs={removal_program};
        removal_profile.sources={parse_xml("<room width='5' height='4'><script><action on='success' action_type='remove' id='victim'/></script></room>")};
        removal_profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(removal_program,&removal_program->scripts)},{0,0},1}};
        const auto orphan_before=scripted.enemies()[1],other_leader=scripted.enemies()[2];
        const auto random_before=*parse_xml(save(scripted)).first("random");
        const auto leader_object=script_objects.bindings()[2].object.identity;check(script_objects.reference(leader_object)->subject==2);
        script_objects.assign(leader_object,"victim",scripted,layout);
        RoomRuntimeState removal_state;SessionRoomScriptExecutor::run(scripted,removal_profile,script_objects,removal_state);
        check(scripted.enemies().size()==3 && scripted.enemies()[0].id==orphan_before.id && !scripted.enemies()[0].horde->leader);
        const auto& orphan=scripted.enemies()[0];
        check(orphan.health==orphan_before.health && orphan.mana==orphan_before.mana && orphan.primary==orphan_before.primary &&
            orphan.horde->moniker==orphan_before.horde->moniker && orphan.horde->platoon==orphan_before.horde->platoon && orphan.horde->quest==orphan_before.horde->quest);
        check(scripted.enemies()[1].horde->followers==other_leader.horde->followers);
        const auto random_after=*parse_xml(save(scripted)).first("random");check(random_after.attributes==random_before.attributes && random_after.text==random_before.text);
        script_objects.assign(script_objects.bindings().back().object.identity,"victim",scripted,layout);
        removal_state=RoomRuntimeState{};SessionRoomScriptExecutor::run(scripted,removal_profile,script_objects,removal_state);
        check(scripted.enemies().size()==2 && scripted.enemies()[1].id==other_leader.id && scripted.enemies()[1].horde->followers.empty());
        check(scripted.enemies()[1].primary==other_leader.primary && scripted.enemies()[1].horde->moniker==other_leader.horde->moniker);
        std::ostringstream removed_save;SessionRoomScriptExecutor::save(removed_save,scripted,removal_profile,script_objects,removal_state);
        std::istringstream removed_input(removed_save.str());auto removed=SessionRoomScriptExecutor::load(removed_input,removal_profile);
        check(save(removed.session)==save(scripted) && removed.objects.bindings().size()==4);
        std::cout<<"horde placement batches preserve pending occupancy, duplicate sites, group links/colour/stats through save24/floor cache and rollback failed construction\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
