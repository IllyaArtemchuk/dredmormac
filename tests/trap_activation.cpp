#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("trap activation assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
struct Fixture {
    std::shared_ptr<const std::vector<ItemDefinition>> items=std::make_shared<const std::vector<ItemDefinition>>(std::vector<ItemDefinition>{
        read_item(parse_xml("<item name='Fixture trap'><trap trigger='charges' charges='2' casts='Fixture spell'/></item>"))});
    std::shared_ptr<const std::vector<SpellDefinition>> spells=std::make_shared<const std::vector<SpellDefinition>>(std::vector<SpellDefinition>{
        read_spell(parse_xml("<spell name='Fixture spell' type='target'><effect type='damage' slashing='8' affectsCaster='1' unblockable='1'/></spell>"))});
    ItemFactoryTables tables;ItemFactoryContext context;TrapMap map{7,4,{0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0}};
    Fixture() {tables.traps[0]={0};}
    Session session(bool profile=true) const {
        std::istringstream rows("#######\n#@....#\n#.....#\n#######\n");Session result(World::read(rows),make_player({3,2,2}),1);
        result.equipment_profile({3,2,2},items);if (profile) result.spell_profile(spells);return result;
    }
    void trap(Session& session,Position position={2,1}) const {
        RoomTrapRecord record;record.name="Fixture trap";record.placement.relative=position;record.placement.id="activation-trap";
        check(session.create_room_trap(record,tables,context,map).instance);
    }
};
int main() {
    try {
        Fixture fixture;auto session=fixture.session();fixture.trap(session);const auto health=session.player().health;
        session.step(Action::east);check(session.world().turn()==1 && session.player().health==health-8 && session.traps()[0].charges==1 && session.traps()[0].discovered);
        check(session.events().size()==4 && session.events()[1].kind==EventKind::trap_click && session.events()[2].kind==EventKind::trap_triggered && session.events()[3].kind==EventKind::spell_damage);
        std::istringstream input(saved(session));auto resumed=Session::load(input,fixture.items,{},fixture.spells);
        for (auto* current:{&session,&resumed}) {current->step(Action::west);current->step(Action::east);}
        check(saved(session)==saved(resumed) && session.player().health==health-16 && session.traps()[0].pending_destruction && session.traps()[0].charges==0);
        session.step(Action::west);session.step(Action::east);check(session.player().health==health-16 && session.detect_traps().empty());
        auto wrong=*fixture.spells;wrong[0].effects[0].damage[1]++;bool failed=false;
        try {std::istringstream invalid(saved(session));Session::load(invalid,fixture.items,{},std::make_shared<const std::vector<SpellDefinition>>(wrong));}
        catch(const std::runtime_error&) {failed=true;}
        check(failed);failed=false;
        try {std::istringstream missing(saved(session));Session::load(missing,fixture.items);} catch(const std::runtime_error&) {failed=true;}
        check(failed);
        auto absent=fixture.session(false);fixture.trap(absent);const auto no_profile=saved(absent);failed=false;
        try {absent.step(Action::east);} catch(const std::runtime_error&) {failed=true;}
        check(failed && saved(absent)==no_profile);
        std::istringstream old_profile(no_profile);auto migrated=Session::load(old_profile,fixture.items,{},fixture.spells);migrated.step(Action::east);
        check(migrated.player().health==health-8 && saved(migrated).find("<spell-rules")!=std::string::npos);
        auto missing=fixture.session();fixture.trap(missing);missing.spell_profile(std::make_shared<const std::vector<SpellDefinition>>());missing.step(Action::east);
        check(missing.player().health==health && missing.traps()[0].charges==2 && missing.events().back().kind==EventKind::trap_spell_missing);
        auto unsupported=fixture.session();fixture.trap(unsupported);auto partial=*fixture.spells;
        partial[0].effects.push_back(read_spell_effect(parse_xml("<effect type='teleport' amount='1'/>")));
        unsupported.spell_profile(std::make_shared<const std::vector<SpellDefinition>>(partial));const auto before=saved(unsupported);failed=false;
        try {unsupported.step(Action::east);} catch(const std::invalid_argument&) {failed=true;}
        check(failed && saved(unsupported)==before);
        for (int terrain:{0,1}) {
            auto monsters=fixture.session();fixture.trap(monsters,{3,1});Actor enemy;enemy.name="Fixture enemy";enemy.position={4,1};enemy.health=enemy.secondary[0]=100;enemy.monster_terrain=terrain;
            const auto id=monsters.spawn_enemy(enemy);monsters.step(Action::wait);
            check(monsters.enemies()[0].position.x==3 && monsters.enemies()[0].health==(terrain?100:92) && monsters.traps()[0].charges==(terrain?2:1));
            if (!terrain) check(monsters.events().back().actor==monsters.player().id && monsters.events().back().target==id);
            std::istringstream restored(saved(monsters));auto copy=Session::load(restored,fixture.items,{},fixture.spells);check(saved(copy)==saved(monsters));
        }
        std::cout<<"automatic player/monster traps, charges, terrain, spell identity, resumed turns and effect rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
