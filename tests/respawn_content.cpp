#include "dredmor/content.hpp"
#include "dredmor/spawn_locations.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("respawn adapter assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
namespace {
struct TemporaryRoot {std::filesystem::path path;~TemporaryRoot() {std::error_code ignored;std::filesystem::remove_all(path,ignored);}};
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
World world(std::string text="#####\n#@.##\n##.##\n#####\n") {std::istringstream input(text);return World::read(input);}
bool same_random(const GameRandom& a,const GameRandom& b) {return a.libc.state()==b.libc.state() && a.mt.state().words==b.mt.state().words && a.mt.state().index==b.mt.state().index;}
}
int main(int argc,char** argv) {
    try {
        check(argc==2);TemporaryRoot root{std::filesystem::temp_directory_path()/("dredmor-respawn-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        std::filesystem::copy(argv[1],root.path,std::filesystem::copy_options::recursive);
        std::ofstream(root.path/"game/monDB.xml")<<R"(<monDB><monster name="Fixture monster"><stats numFig="2" numWiz="1" numRog="1"/>
            <idleSprite down="fixture.spr"/><monster name="Fixture variant" level="1"><palette tint="-1"/></monster></monster></monDB>)";
        // Keep the existing source fixtures intact; this private test adds a
        // floor-mounted trap whose separate origin occupies the only horde site.
        std::ifstream original_items(root.path/"game/itemDB.xml");std::ostringstream item_bytes;item_bytes<<original_items.rdbuf();
        auto added=item_bytes.str();const auto close=added.rfind("</itemDB>");check(close!=std::string::npos);
        added.insert(close,"<item name='Origin trap'><trap originmount='floor' originfacing='north' origin='origin.spr'/></item>");
        std::ofstream(root.path/"game/itemDB.xml")<<added;
        std::ofstream(root.path/"game/text.xml")<<"<text><startphoneme text='S'/><phoneme text='x'/><title text='Strong'/><noun text='Noun' plural='People'/><adjective text='Old'/></text>";
        auto content=GameContent::load(root.path);
        std::vector<DungeonCell> cells(20);for (auto& cell:cells) cell.kind=2;cells[6].kind=cells[7].kind=cells[12].kind=1;
        DungeonLayout layout(5,4,cells);std::vector<bool> occupied(20,false);occupied[6]=true;std::vector<Position> sites;
        const MonsterSpawnWorld map{layout,occupied,sites};
        for (std::uint32_t seed=1;seed<=64;++seed) {
            GameRandom random;random.seed(seed);auto expected=random;MonsterSpawnState counts,expected_counts;
            const auto selected=select_room_monster(content.monster_tables(),content.monsters(),{},content.tweaks().integer(Difficulty::medium,"monster boost chance"),expected_counts,expected);check(selected);
            auto actor=content.create_monster(content.monsters()[*selected].name,{-1,-1},Difficulty::medium,0,expected.libc);
            const auto point=select_monster_spawn_position(layout,occupied,sites,{-1,-1},content.monsters()[*selected].special,expected.libc);check(point);
            const auto flags=finish_random_monster_spawn(content.monsters()[*selected],expected_counts,expected.mt);
            auto result=content.create_respawn_monster(map,{},Difficulty::medium,random,counts);check(result);
            check(result->name==actor.name && result->primary==actor.primary && result->secondary==actor.secondary && result->health==actor.health && result->mana==actor.mana);
            check(result->position.x==point->x && result->position.y==point->y && result->sprite_tint==actor.sprite_tint && result->monster_flags==flags);
            check(same_random(random,expected) && counts.counts==expected_counts.counts && !result->room_spawned && result->room_local_id.empty());
        }
        Session session(world(),make_player({3,2,2}),7);auto spawn=[&](Session& owner) {
            return owner.create_respawn_monster(layout,sites,[&](const auto& grid,auto& random,auto& counts) {
                return content.create_respawn_monster(grid,{owner.depth_context()},Difficulty::medium,random,counts);
            });
        };
        auto id=spawn(session);check(id==2 && session.enemies().size()==1 && session.enemies()[0].sprite_tint>=0 && session.monster_spawn_state().counts.at("Fixture variant")==1);
        const auto saved_session=saved(session);std::istringstream input(saved_session);auto restored=Session::load(input);check(saved(restored)==saved_session);
        const auto before_failure=saved(session);bool rejected=false;
        try {session.create_respawn_monster(layout,sites,[](const auto&,auto& random,auto&)->std::optional<Actor> {random.libc.next();random.mt.u32();throw std::runtime_error("constructor failure");});} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && saved(session)==before_failure);
        Session secret(world("#####\n#@.##\n#####\n#####\n"),make_player({3,2,2}),7);
        auto closed=cells;closed[12].kind=2;DungeonLayout secret_layout(5,4,closed);
        secret.floor_secret_levers({{{2,1},{2,2},false}});check(secret.use_secret_lever(0));check(secret.room_spawn_layout(secret_layout).at(2,2).kind==1);
        check(secret.create_respawn_monster(secret_layout,sites,[&](const auto& grid,auto& random,auto& counts) {return content.create_respawn_monster(grid,{},Difficulty::medium,random,counts);}));
        check(secret.enemies()[0].position.x==2 && secret.enemies()[0].position.y==2);
        Session origins(world(),make_player({3,2,2}),7);origins.equipment_profile({3,2,2},std::make_shared<const std::vector<ItemDefinition>>(content.items()));
        RoomTrapRecord trap;trap.name="Origin trap";trap.placement.relative={2,1};TrapMap trap_map{5,4,{}};for (const auto& cell:cells) trap_map.tiles.push_back(cell.kind);
        check(origins.create_room_trap(trap,content.item_factory_tables(),content.loot_context(),trap_map).instance);
        check(origins.traps()[0].has_origin_object && origins.traps()[0].origin.x==2 && origins.traps()[0].origin.y==2);
        RoomHordeRecord horde;horde.name="Fixture variant";horde.minimum=horde.additional=0;horde.placement.relative={2,2};
        const auto group=content.queue_script_horde(origins,horde,layout,{{{2,2},{3,3}}});check(group && group->members.size()==1);origins.place_queued_hordes();
        Session shop(world(),make_player({3,2,2}),7);shop.equipment_profile({3,2,2},std::make_shared<const std::vector<ItemDefinition>>(content.items()));
        ShopRoomMarkers markers;markers.home=markers.guard={2,1};
        shop.create_shop(markers,{{2,2}},content.item_factory_tables(),content.loot_context(),[&](auto& random) {return content.create_monster("Fixture monster",{2,1},Difficulty::medium,0,random.libc);});
        check(shop.room_spawn_layout(layout).at(2,2).kind==13 && layout.at(2,2).kind==1);
        std::cout<<"respawn construction order/RNG/counts, saved actors, failure rollback and live trap/shop/secret geometry agree\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
