#include "dredmor/content.hpp"
#include "dredmor/session_room_scripts.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("Session scripts assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
template<class F> bool rejects(F body) {try {body();} catch(const std::exception&) {return true;}return false;}
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
std::string saved(const SessionRoomObjects& objects,const Session& session) {std::ostringstream out;objects.save(out,session);return out.str();}
std::string saved(const RoomRuntimeState& state,const std::vector<RoomRuntimeRoom>& rooms) {std::ostringstream out;write_room_runtime_state(out,state,rooms);return out.str();}
int main() {
    try {
        RoomDefinition definition;definition.name="Synthetic live Session puzzle";
        definition.rows={"#########","#.......#","#..1234.#","#.......#","#########"};
        definition.source=parse_xml(R"(<room width="9" height="5">
            <script><condition condition_type="activated_changed" id="switch" activated="1"/>
              <action on="success" action_type="move" id="gate" at="1"/>
              <action on="success" action_type="remove" id="gate"/>
              <action on="success" action_type="remove" id="gate"/>
              <action on="success" action_type="create" create_type="customblocker" at="3" png="replacement.spr" id="replacement"/>
              <action on="success" action_type="move" id="player" at="2"/>
              <action on="success" action_type="move" id="player" at="3"/>
              <action on="success" action_type="move" id="player" at="4"/>
              <action on="success" action_type="ticker" text="switch fired"/></script>
            <script><condition condition_type="at" id="gate" at="2"/>
              <action on="success" action_type="ticker" text="gate visible until clear"/></script>
            <script><condition condition_type="at" id="replacement" at="3"/>
              <action on="success" action_type="set" id="replacement" set_type="passable" value="1"/>
              <action on="success" action_type="ticker" text="created object visible immediately"/></script>
            <script><condition condition_type="at" id="gate" at="2" not="1"/>
              <action on="success" action_type="ticker" text="gate gone"/></script>
            </room>)");
        auto program=std::make_shared<const RoomScriptProgram>(definition.script_program());
        auto entry=std::make_shared<RoomScriptProgram>();RoomCondition entered;entered.kind=RoomConditionKind::entered;
        RoomAction message;message.kind=RoomActionKind::ticker;message.text="entered second room";
        RoomScriptDefinition on_entry;on_entry.conditions={entered};on_entry.success={message};entry->scripts={on_entry};
        std::vector<DungeonCell> cells(45);for (std::size_t i=0;i<cells.size();++i) {cells[i].kind=1;cells[i].room_id=i%9>=6?1:0;}
        SessionRoomScriptProfile profile(DungeonLayout(9,5,cells));
        profile.programs={program,entry};
        profile.sources={definition.source,parse_xml("<room><script><condition condition_type='entered'/><action on='success' action_type='ticker' text='entered second room'/></script></room>")};
        profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(program,&program->scripts)},{0,0},1},
                       {{true,1,1,std::shared_ptr<const std::vector<RoomScriptDefinition>>(entry,&entry->scripts)},{0,0},2}};
        std::istringstream rows("#########\n#.......#\n#.@.....#\n#.......#\n#########\n");
        Session session(World::read(rows),make_player({3,2,2}),7);SessionRoomObjects objects;
        CustomRoomDefinition gate;gate.placement.relative={4,2};gate.placement.id="gate";gate.sprite.image="gate.spr";
        session.create_custom_object(gate);auto gate_binding=objects.bind(session,{SessionRoomObjectKind::custom,1},profile.layout);
        RoomElementDefinition lever;lever.type="lever";lever.placement.relative={3,2};lever.placement.id="switch";
        session.create_script_lever(lever);objects.bind(session,{SessionRoomObjectKind::lever,1},profile.layout);
        objects.bind(session,{SessionRoomObjectKind::player,session.player().id},profile.layout);
        session.step(Action::east);check(session.script_levers()[0].activation.changed());
        RoomRuntimeState runtime;const auto before_session=saved(session),before_bindings=saved(objects,session),before_runtime=saved(runtime,profile.rooms);
        // Fail after Create, removal and earlier queued teleports have mutated
        // the candidate. Every owned payload and both RNG streams roll back.
        auto bad_program=std::make_shared<RoomScriptProgram>(*program);
        auto invalid_move=program->scripts[0].success[5];invalid_move.point={99,99};bad_program->scripts[0].success.push_back(invalid_move);
        auto bad_profile=profile;bad_profile.programs[0]=bad_program;
        bad_profile.rooms[0].trigger.scripts=std::shared_ptr<const std::vector<RoomScriptDefinition>>(bad_program,&bad_program->scripts);
        check(rejects([&]{SessionRoomScriptExecutor::run(session,bad_profile,objects,runtime);}));
        check(saved(session)==before_session && saved(objects,session)==before_bindings && saved(runtime,profile.rooms)==before_runtime);
        auto result=SessionRoomScriptExecutor::run(session,profile,objects,runtime);
        check(result.triggers.successes.size()==3 && result.presentation.size()==3 && result.player_teleported);
        check(result.presentation[0].text=="switch fired" && result.presentation[1].text=="gate visible until clear" && result.presentation[2].text=="created object visible immediately");
        check(session.custom_objects().size()==1 && session.custom_objects()[0].room_local_id=="replacement");
        check(!session.custom_objects()[0].blocks_movement && session.custom_objects()[0].position.x==5);
        check(session.player().position.x==6 && session.player().position.y==2 && session.world().turn()==1);
        check(!objects.object(session,gate_binding.object.identity) && objects.find(session,0,"replacement")->position.x==5);
        check(runtime.triggers.previous_room==0); // captured before queued moves
        // The remaining object's shifted typed index and the opaque ID both
        // survive Session and script-component snapshots.
        const auto encoded_session=saved(session),encoded_objects=saved(objects,session),encoded_runtime=saved(runtime,profile.rooms);
        std::istringstream input(encoded_session);auto restored=Session::load(input);
        auto restored_objects=SessionRoomObjects::load(parse_xml(encoded_objects),restored);
        auto restored_runtime=read_room_runtime_state(parse_xml(encoded_runtime),profile.rooms);
        std::ostringstream combined;SessionRoomScriptExecutor::save(combined,session,profile,objects,runtime);
        std::istringstream combined_input(combined.str());auto snapshot=SessionRoomScriptExecutor::load(combined_input,profile);
        check(saved(snapshot.session)==encoded_session && saved(snapshot.objects,snapshot.session)==encoded_objects && saved(snapshot.runtime,profile.rooms)==encoded_runtime);
        // A changed Create body previously escaped runtime action hashing:
        // its local record handle stays the same. Authoritative source protects
        // the combined snapshot even for an unused Create or another room.
        auto changed_profile=profile;changed_profile.sources[0].children[0].children[4].attributes["png"]="different.spr";
        check(rejects([&]{std::istringstream wrong(combined.str());SessionRoomScriptExecutor::load(wrong,changed_profile);}));
        auto changed_grid=profile;changed_grid.layout.at(1,1).room_id=1;
        check(rejects([&]{std::istringstream wrong(combined.str());SessionRoomScriptExecutor::load(wrong,changed_grid);}));
        auto malformed=combined.str();const auto sequence=malformed.find("<room-bindings");malformed.replace(sequence,14,"<bad-bindings");
        check(rejects([&]{std::istringstream wrong(malformed);SessionRoomScriptExecutor::load(wrong,profile);}));
        auto next=SessionRoomScriptExecutor::run(session,profile,objects,runtime);
        auto next_restored=SessionRoomScriptExecutor::run(restored,profile,restored_objects,restored_runtime);
        check(next.presentation.size()==2 && next.presentation[0].text=="gate gone" && next.presentation[1].text=="entered second room");
        check(next_restored.presentation.size()==next.presentation.size() && saved(session)==saved(restored));
        check(saved(objects,session)==saved(restored_objects,restored) && saved(runtime,profile.rooms)==saved(restored_runtime,profile.rooms));
        check(SessionRoomScriptExecutor::run(session,profile,objects,runtime).presentation.empty());
        // Constructor/resource failure cannot leak its created object or RNG.
        auto resource_program=std::make_shared<RoomScriptProgram>();BreakableRoomDefinition breakable;
        breakable.placement.relative={5,3};breakable.placement.id="new breakable";resource_program->created_records={breakable};
        RoomScriptDefinition create;RoomAction action;action.kind=RoomActionKind::create;action.record=1;create.success={action};resource_program->scripts={create};
        auto resource_profile=profile;resource_profile.programs[0]=resource_program;
        resource_profile.rooms[0].trigger.scripts=std::shared_ptr<const std::vector<RoomScriptDefinition>>(resource_program,&resource_program->scripts);
        RoomRuntimeState fresh;SessionRoomScriptHooks resources;resources.breakable_frames=[](const auto&)->BreakableFrames {throw std::runtime_error("resource failure");};
        const auto unchanged=saved(session);check(rejects([&]{SessionRoomScriptExecutor::run(session,resource_profile,objects,fresh,resources);}));
        check(saved(session)==unchanged && fresh.triggers.success_counts.empty());
        auto mismatch=profile;mismatch.rooms[0].trigger.scripts=profile.rooms[1].trigger.scripts;
        check(rejects([&]{SessionRoomScriptExecutor::run(session,mismatch,objects,runtime);}));
        // Breakable SetEvent uses its captured room/name, and hit+broken can
        // both reach this pass. Removal cancels the destroyed object's pending
        // integration animation indices instead of targeting a shifted object.
        std::istringstream event_rows("#########\n#@......#\n#.......#\n#.......#\n#########\n");
        Session event_session(World::read(event_rows),make_player({3,2,2}),7);
        BreakableRoomDefinition fragile;fragile.placement.relative={2,1};fragile.placement.id="fragile";
        fragile.intact.image="intact.spr";fragile.broken.image="broken.spr";fragile.hits_needed=2;
        event_session.create_breakable(fragile,{0,0},BreakableFrames{3,1,4});
        auto event_cells=cells;for (auto& cell:event_cells) cell.room_id=0;
        SessionRoomScriptProfile event_profile(DungeonLayout(9,5,event_cells));
        auto event_program=std::make_shared<RoomScriptProgram>();
        RoomCondition hit;hit.kind=RoomConditionKind::event;hit.object_id="fragile";hit.event_name="hit";
        RoomCondition broken_event=hit;broken_event.event_name="broken";
        RoomAction hit_message=message;hit_message.text="hit";RoomAction broken_message=message;broken_message.text="broken";
        RoomAction remove;remove.kind=RoomActionKind::remove;remove.object_id="fragile";
        RoomScriptDefinition hit_script;hit_script.success_repeat=-1;hit_script.conditions={hit};hit_script.success={hit_message};
        RoomScriptDefinition broken_script;broken_script.conditions={broken_event};broken_script.success={remove,broken_message};
        event_program->scripts={hit_script,broken_script};event_profile.programs={event_program};
        event_profile.sources={parse_xml("<room><custombreakable x='2' y='1' id='fragile' png='intact.spr' broken='broken.spr' hitsNeededToBreak='2'/></room>")};
        event_profile.rooms={{{true,1,0,std::shared_ptr<const std::vector<RoomScriptDefinition>>(event_program,&event_program->scripts)},{0,0},1}};
        SessionRoomObjects event_objects;event_objects.bind(event_session,{SessionRoomObjectKind::breakable,1},event_profile.layout);
        event_objects.bind(event_session,{SessionRoomObjectKind::player,event_session.player().id},event_profile.layout);
        RoomRuntimeState event_runtime;
        event_session.step(Action::east);
        check(event_objects.set_event(event_session,event_runtime.triggers.events,{SessionRoomObjectKind::breakable,1},"hit"));
        check(event_objects.set_event(event_session,event_runtime.triggers.events,{SessionRoomObjectKind::breakable,1},"hit"));
        check(event_runtime.triggers.events.size()==1 && event_runtime.triggers.events.begin()->second.size()==1);
        check(!event_objects.set_event(event_session,event_runtime.triggers.events,{SessionRoomObjectKind::breakable,2},"hit"));
        std::ostringstream pending_events;SessionRoomScriptExecutor::save(pending_events,event_session,event_profile,event_objects,event_runtime);
        std::istringstream pending_input(pending_events.str());auto pending=SessionRoomScriptExecutor::load(pending_input,event_profile);
        auto first_hit=SessionRoomScriptExecutor::run(pending.session,event_profile,pending.objects,pending.runtime);
        check(first_hit.presentation.size()==1 && first_hit.presentation[0].text=="hit" && pending.session.breakables().size()==1);
        pending.session.step(Action::east);
        for (const auto& event:pending.session.events())
            pending.objects.set_event(pending.session,pending.runtime.triggers.events,{SessionRoomObjectKind::breakable,event.target},event.kind==EventKind::object_hit?"hit":"broken");
        auto last_hit=SessionRoomScriptExecutor::run(pending.session,event_profile,pending.objects,pending.runtime);
        check(last_hit.presentation.size()==2 && last_hit.presentation[1].text=="broken" && pending.session.breakables().empty());
        check(!pending.objects.find(pending.session,0,"fragile") && pending.session.events().empty() && pending.runtime.triggers.events.empty());
        check(SessionRoomScriptExecutor::run(pending.session,event_profile,pending.objects,pending.runtime).presentation.empty());
        std::cout<<"live Session room puzzle preserves same-pass visibility, deferred deletion/movement, stable references, rollback and saved continuation\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
