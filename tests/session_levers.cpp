#include "dredmor/floors.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("session lever assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
World room() {std::istringstream input("#######\n#.....#\n#.@...#\n#.....#\n#######\n");return World::read(input);}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        Session session(room(),make_player({3,2,2}),7);const auto baseline=saved(session);
        RoomElementDefinition definition;definition.type="lever";definition.placement.relative={3,2};definition.placement.id="switch";definition.placement.can_push=0;
        check(session.create_script_lever(definition)==0 && session.script_levers().size()==1 && !session.passable({3,2}));
        check(session.script_levers()[0].object.room_local_id=="switch" && session.script_levers()[0].object.sprite.image=="dungeon/lever1.spr");
        const auto initial=saved(session);check(parse_xml(initial).attribute("version")=="22");check(parse_xml(initial).first("random")->attribute("libc")==parse_xml(baseline).first("random")->attribute("libc"));
        check(!session.use_script_lever(0,true) && saved(session)==initial);
        session.step(Action::east);check(session.world().turn()==1 && session.player().position.x==2 && session.script_levers()[0].activation.activated());
        check(session.script_levers()[0].activation.previous==1 && session.script_levers()[0].activation.older==0 && session.script_levers()[0].activation.changed());
        check(session.script_levers()[0].object.frame==1 && session.events().front().kind==EventKind::lever_used);
        std::istringstream in(saved(session));auto restored=Session::load(in);check(saved(restored)==saved(session));
        session.step(Action::wait);restored.step(Action::wait);check(!session.script_levers()[0].activation.changed() && saved(restored)==saved(session));
        check(session.use_script_lever(0) && !session.script_levers()[0].activation.activated() && session.script_levers()[0].object.frame==0 && session.script_levers()[0].activation.changed());
        auto cached=session.floor_state();check(cached.script_levers.size()==1);session.swap_floor(SessionFloor(room()),{2,2});check(session.script_levers().empty());session.swap_floor(std::move(cached),{2,2});check(session.script_levers().size()==1);
        auto invalid=saved(session);const auto at=invalid.find("current=\"0\"");check(at!=std::string::npos);invalid.replace(at,11,"current=\"2\"");bool rejected=false;
        try {std::istringstream bad(invalid);Session::load(bad);} catch(const std::runtime_error&) {rejected=true;}check(rejected);
        definition.placement.relative={99,99};const auto before=saved(session);bool outside=false;try {session.create_script_lever(definition);} catch(const std::invalid_argument&) {outside=true;}check(outside && saved(session)==before);
        ActivationState activation;const auto tutorial=flip_script_lever(activation,true);check(tutorial.tutorial_event && !activation.activated());
        std::istringstream old(baseline);check(Session::load(old).script_levers().empty());
        std::cout<<"ordinary lever creation, bump/click/held gates, two-update history, canonical save/load, cached floors and failed creation rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
