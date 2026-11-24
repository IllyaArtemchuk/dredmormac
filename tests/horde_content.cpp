#include "dredmor/content.hpp"
#include "dredmor/floors.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("horde content assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
struct TemporaryRoot {
    std::filesystem::path path;
    ~TemporaryRoot() {std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
bool same_random(const GameRandom& a,const GameRandom& b) {
    return a.libc.state()==b.libc.state() && a.mt.state().index==b.mt.state().index && a.mt.state().words==b.mt.state().words;
}
}
int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::invalid_argument("synthetic content fixture root required");
        TemporaryRoot root{std::filesystem::temp_directory_path()/("dredmor-horde-content-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        std::filesystem::copy(argv[1],root.path,std::filesystem::copy_options::recursive);
        std::ofstream(root.path/"game/monDB.xml")<<R"(<monDB><monster name="Fixture monster"><stats numFig="2" numRog="1"/>
            <idleSprite down="fixture.spr"/><monster name="Fixture variant" level="1"><palette tint="120"/></monster>
            </monster><monster name="Floor monster" level="2"><stats numFig="2"/><idleSprite down="fixture.spr"/></monster></monDB>)";
        std::ofstream(root.path/"game/tweakDB.xml")<<R"(<tweakdb><tweak name="monster HP global scale" fval="1.1"/>
            <tweak name="monster additional HP percent per level" fval="0.1"/><tweak name="monster additional HP per level" ival="2"/>
            <tweak name="monster level adder" ival="3"/><tweak name="horde is overpowered change" ival="2"/></tweakdb>)";
        const auto without_words=GameContent::load(root.path);
        std::ofstream(root.path/"game/text.xml")<<R"(<text><startphoneme text="Test"/><phoneme text="x"/>
            <title text="Strong"/><noun text="Noun" plural="People"/><adjective text="Old"/></text>)";
        const auto content=GameContent::load(root.path);
        std::vector<DungeonCell> cells(25);for (auto& cell:cells) cell.kind=2;cells[12].kind=1;
        DungeonLayout argument(5,5,cells),current(5,5,cells);std::vector<bool> occupied(25,false);
        std::vector<HordeRoomBounds> rooms={{{2,2},{3,3}}};HordeWorld world{argument,argument,occupied,occupied,rooms};
        HordeOptions options;options.position={2,2};options.depth=0;options.minimum=0;options.additional=0;
        options.named="Fixture variant";options.quest=HordeQuest{13,"Quest group"};
        MonsterSpawnState counts;counts.counts["Fixture variant"]=7;GameRandom random;random.seed(0);const auto before=random;
        const auto created=content.create_horde(options,world,Difficulty::medium,9,counts,random,42);
        check(created.result.accepted && created.result.depth==-1 && created.actors.size()==2 && created.result.quest_count==2);
        check(counts.counts.at("Fixture variant")==7 && created.result.members[0].identity==42 && created.result.members[1].identity==43);
        const auto& leader=created.actors[0];const auto& follower=created.actors[1];
        check(leader.id==42 && leader.position.x==-1 && leader.position.y==-1 && follower.position.x==-1 && follower.position.y==-1);
        check(leader.stat_profile->dungeon_level==-1 && follower.stat_profile->dungeon_level==-1);
        auto base=*leader.stat_profile;base.primary_percent=default_primary_percentages();const auto initial_stats=derive_monster(base);
        check(leader.health==initial_stats.secondary[0] && leader.mana==initial_stats.secondary[1] && leader.secondary[0]>leader.health);
        check(leader.stat_profile->primary_percent[0]==120 && leader.stat_profile->primary_percent[1]==110 && follower.stat_profile->primary_percent[0]==100);
        check(leader.sprite_tint>=0 && leader.sprite_tint<360 && leader.sprite_palette.empty() && follower.sprite_tint==120);
        check(created.result.members[0].moniker=="Testxx the Strong People" && created.result.members[0].platoon=="Quest group");
        check(created.result.members[0].followers==std::vector<std::uint64_t>{43} && created.result.members[1].leader==42 && created.result.members[1].quest==13);
        check(created.result.teleports.size()==2 && created.result.teleports[0].position.x==2 && created.result.teleports[1].position.x==2);
        check(!created.result.searches[0].found && created.result.searches[0].accepted);
        check(!leader.monster_flags && !follower.monster_flags);
        // A distinct current level controls constructor scaling independently.
        HordeWorld other{argument,current,occupied,occupied,rooms};auto alternate_random=before;
        const auto separate=content.create_horde(options,other,Difficulty::medium,9,counts,alternate_random,42);
        check(separate.result.depth==-1 && separate.actors[0].stat_profile->dungeon_level==9 && separate.actors[0].health>leader.health);
        check(same_random(random,alternate_random));
        auto failed=before;bool rejected=false;
        try {without_words.create_horde(options,world,Difficulty::medium,9,counts,failed,42);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && same_random(failed,before));
        auto missing=options;missing.named="unregistered";rejected=false;
        try {content.create_horde(missing,world,Difficulty::medium,9,counts,failed,42);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && same_random(failed,before));
        auto invalid=options;invalid.position={-1,2};const auto skipped=without_words.create_horde(invalid,world,Difficulty::medium,9,counts,failed,42);
        check(!skipped.result.accepted && skipped.actors.empty() && same_random(failed,before));
        auto make_world=[] {std::istringstream rows("#####\n#@..#\n#...#\n#####\n");return World::read(rows);};
        Session session(make_world(),make_player({3,2,2}),19);
        RoomMonsterRecord ordinary;ordinary.placement.relative={3,2};
        auto spawn=[&] {
            return session.create_room_monster(ordinary,[&](const auto& plan,auto& rng,auto& state) {
                return content.create_room_monster(plan,{session.depth_context()},Difficulty::medium,rng,state);
            });
        };
        check(spawn() && session.monster_spawn_state().counts.at("Fixture variant")==1);
        SessionFloor second(make_world());second.depth.depth=1;
        auto first=session.swap_floor(std::move(second),{1,1});
        check(session.monster_spawn_state().counts.empty() && first.monster_state.counts.at("Fixture variant")==1);
        check(spawn() && session.monster_spawn_state().counts.at("Floor monster")==1 && session.monster_spawn_state().counts.size()==1);
        auto cached=session.swap_floor(std::move(first),{1,1});
        check(session.monster_spawn_state().counts.at("Fixture variant")==1 && session.monster_spawn_state().counts.size()==1);
        FloorRegistry floors;floors.add_floor(session,std::move(cached));std::ostringstream encoded;floors.save(session,encoded);
        std::istringstream saved(encoded.str());auto restored=FloorRegistry::load(saved);
        check(restored.first.monster_spawn_state().counts.at("Fixture variant")==1);
        check(restored.second.cached()[0].monster_state.counts.at("Floor monster")==1 && restored.second.cached()[0].monster_state.counts.size()==1);
        std::cout<<"constructor-backed hordes preserve HP/MP across leader promotion, resolve colour/names/group links, keep counts unchanged, separate current depth, queue duplicate placements and roll back failures\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
