#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("lever assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
World map() {std::istringstream input("########\n#@..#..#\n#......#\n#......#\n########\n");return World::read(input);}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        SecretLever lever{{2,1},{4,1},false};check(can_click_secret_lever(lever,{1,1}) && !can_click_secret_lever(lever,{1,2}) && !can_click_secret_lever(lever,{1,1},true));
        const auto first=flip_secret_lever(lever);check(first.revealed && lever.activated && first.sound=="lever3" && first.graphics_bounds[0]==1 && first.graphics_bounds[1]==-2);
        check(!flip_secret_lever(lever).revealed);lever.activated=false;const auto tutorial=flip_secret_lever(lever,true);check(tutorial.tutorial_event && !tutorial.revealed && lever.activated && tutorial.sound=="lever");
        Session session(map(),make_player({3,2,2}),1);session.floor_secret_levers({{{2,1},{4,1},false}});session.floor_stairs({{{1,2},false,4}});
        check(!session.passable({2,1}) && !session.world().passable({4,1}));const auto initial=saved(session);check(initial.find("version=\"18\"")!=std::string::npos);
        check(session.use_secret_lever(0) && session.world().passable({4,1}) && session.secret_levers()[0].activated && session.world().turn()==1 && session.player().position.x==1);
        session.step(Action::east);check(session.world().turn()==2 && session.player().position.x==1 && session.events()[0].kind==EventKind::lever_used && session.events()[0].amount==0);
        const auto opened=saved(session);std::istringstream input(opened);auto restored=Session::load(input);check(saved(restored)==opened && restored.secret_levers()[0].activated && restored.world().passable({4,1}));
        std::istringstream old(initial);auto closed=Session::load(old);check(!closed.secret_levers()[0].activated && !closed.world().passable({4,1}));
        bool rejected=false;try {session.floor_stairs({{{2,1},false,0}});} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(session)==opened);
        rejected=false;try {session.floor_secret_levers({{{1,2},{4,1},false}});} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(session)==opened);
        auto bad=opened;const auto at=bad.find("activated=\"1\"");bad.replace(at,13,"activated=\"2\"");std::istringstream malformed(bad);rejected=false;try {Session::load(malformed);} catch(const std::exception&) {rejected=true;}check(rejected && saved(session)==opened);
        FloorRegistry floors;SessionFloor next(map());next.depth.depth=1;next.stairs={{{1,1},true,4}};next.secret_levers={{{2,1},{4,1},false}};floors.add_floor(session,next);
        check(floors.step(session,Action::south)==FloorTravel::travelled && session.depth_context().depth==1 && !session.secret_levers()[0].activated && !session.world().passable({4,1}));
        session.step(Action::east);check(session.secret_levers()[0].activated && session.world().passable({4,1}));
        floors.step(session,Action::south);check(floors.step(session,Action::north)==FloorTravel::travelled && session.depth_context().depth==0 && session.secret_levers()[0].activated && session.world().passable({4,1}));
        std::ostringstream run;floors.save(session,run);std::istringstream run_input(run.str());auto copy=FloorRegistry::load(run_input);std::ostringstream copy_run;copy.second.save(copy.first,copy_run);check(run.str()==copy_run.str() && copy.second.cached()[0].secret_levers[0].activated);
        std::cout<<"secret lever click/bump/repeat, collision changes, tutorial quirk, save18 and cached-floor persistence passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
