#include "dredmor/session_room_objects.hpp"
#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("Session room objects assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
template<class F> bool rejects(F body) {try {body();} catch(const std::exception&) {return true;}return false;}
std::string saved(const SessionRoomObjects& objects,const Session& session) {std::ostringstream out;objects.save(out,session);return out.str();}
int main() {
    try {
        std::istringstream rows("#########\n#.......#\n#.@.....#\n#.......#\n#########\n");
        Session session(World::read(rows),make_player({3,2,2}),7);
        std::vector<DungeonCell> cells(45);for (std::size_t i=0;i<cells.size();++i) {cells[i].kind=1;cells[i].room_id=i%9<4?3:8;}
        DungeonLayout layout(9,5,cells);
        CustomRoomDefinition gate;gate.placement.relative={4,2};gate.placement.id="duplicate";gate.sprite.image="gate.spr";
        check(session.create_custom_object(gate)==0);
        RoomElementDefinition lever;lever.type="lever";lever.placement.relative={3,2};lever.placement.id="duplicate";
        check(session.create_script_lever(lever)==0);
        SessionRoomObjects objects(17);
        auto switched=objects.bind(session,{SessionRoomObjectKind::lever,1},layout);
        auto gated=objects.bind(session,{SessionRoomObjectKind::custom,1},layout);
        check(!switched.duplicate && !gated.duplicate); // distinct assigned rooms
        check(switched.object.assigned_room==3 && gated.object.assigned_room==8);
        auto player=objects.bind(session,{SessionRoomObjectKind::player,session.player().id},layout);
        check(objects.player()==player.object.identity && player.object.global);
        check(objects.find(session,999,"player")->position.x==2);
        check(objects.find(session,3,"duplicate")->identity==switched.object.identity);
        check(objects.find(session,8,"duplicate")->identity==gated.object.identity);
        check(!objects.find(session,3,"Duplicate") && !objects.find(session,4,"duplicate"));
        // SetRoomID captures a tile byte, not a runtime room identity. Changing
        // scope by assigning a different level grid preserves insertion order.
        auto same_cells=cells;for (auto& cell:same_cells) cell.room_id=3;
        DungeonLayout same_room(9,5,same_cells);
        auto conflict=objects.assign(gated.object.identity,"duplicate",session,same_room);
        check(conflict.duplicate==switched.object.identity && objects.find(session,3,"duplicate")->activatable);
        check(rejects([&]{objects.bind(session,{SessionRoomObjectKind::lever,1},layout);}));
        check(rejects([&]{objects.bind(session,{SessionRoomObjectKind::custom,99},layout);}));
        const auto before=saved(objects,session);
        auto candidate=session;candidate.step(Action::east);
        check(!objects.find(session,3,"duplicate")->activation.activated());
        auto live=objects.find(candidate,3,"duplicate");check(live->activatable && live->activation.activated() && live->activation.changed());
        // The full runtime sees live Session state, never the registration-time
        // activation or the immutable typed collection's iteration order.
        RoomCondition condition;condition.kind=RoomConditionKind::activation_changed;condition.object_id="duplicate";condition.wanted=1;
        RoomAction ticker;ticker.kind=RoomActionKind::ticker;ticker.text="live switch";
        RoomScriptDefinition script;script.conditions={condition};script.success={ticker};
        auto scripts=std::make_shared<const std::vector<RoomScriptDefinition>>(std::vector<RoomScriptDefinition>{script});
        RoomRuntimeHooks hooks;hooks.player=*objects.player();hooks.player_room=[] {return 3;};hooks.player_position=[&] {return candidate.player().position;};
        hooks.find_object=[&](auto room,auto id) {return objects.find(candidate,room,id);};unsigned messages=0;
        hooks.perform=[&](const auto& request) {check(request.kind==RoomActionRequestKind::ticker && request.text=="live switch");++messages;};
        std::vector<RoomRuntimeRoom> rooms(4);rooms[3]={RoomTriggerRoom{true,1,3,scripts},{0,0},1};RoomRuntimeState state;
        run_room_scripts(rooms,state,hooks);check(messages==1);
        check(saved(objects,session)==before);session=std::move(candidate);
        // Position changes are resolved through stable subjects, while assigned
        // scope remains the captured room even after crossing another tile.
        session.step(Action::south);check(objects.find(session,99,"player")->position.y==3);
        check(objects.bindings()[2].object.assigned_room==3);
        auto snapshot=objects;check(snapshot.unregister(switched.object.identity));
        check(!snapshot.unregister(switched.object.identity));
        check(snapshot.find(session,3,"duplicate")->identity==gated.object.identity);
        check(objects.find(session,3,"duplicate")->identity==switched.object.identity);
        objects.assign(gated.object.identity,"label&\"\n\t<",session,layout);
        std::ostringstream session_save;session.save(session_save);std::istringstream session_input(session_save.str());auto restored_session=Session::load(session_input);
        auto encoded=saved(objects,session);auto restored=SessionRoomObjects::load(parse_xml(encoded),restored_session);
        check(saved(restored,restored_session)==encoded && restored.find(restored_session,8,"label&\"\n\t<"));
        Actor enemy=make_player({1,0,0},"Monster");enemy.id=0;enemy.room_local_id="not finalized";enemy.position={7,1};
        const auto actor=session.spawn_enemy(enemy);auto unfinalized=objects.bind(session,{SessionRoomObjectKind::monster,actor},layout,false);
        check(unfinalized.object.assigned_room==-1 && unfinalized.object.id.empty());
        check(objects.find(session,-1,"")->monster && !objects.find(session,8,"not finalized"));
        InventoryItem item;item.instance=77;item.kind=ItemKind::misc;item.room_local_id="loot";session.drop(item,session.player().position);
        auto ground=objects.bind(session,{SessionRoomObjectKind::item,77},layout);
        session.pickup();check(!objects.object(session,ground.object.identity));
        check(rejects([&]{saved(objects,session);}));check(objects.unregister(ground.object.identity));
        auto malformed=parse_xml(encoded);malformed.children.push_back(malformed.children.front());
        check(rejects([&]{SessionRoomObjects::load(malformed,restored_session);}));
        malformed=parse_xml(encoded);malformed.children[0].attributes["subject"]="99";
        check(rejects([&]{SessionRoomObjects::load(malformed,restored_session);}));
        malformed=parse_xml(encoded);malformed.children[0].attributes["global"]="1";
        check(rejects([&]{SessionRoomObjects::load(malformed,restored_session);}));
        malformed=parse_xml(encoded);malformed.children[0].attributes["assigned"]="256";
        check(rejects([&]{SessionRoomObjects::load(malformed,restored_session);}));
        malformed=parse_xml(encoded);malformed.attributes["next"]="1";
        check(rejects([&]{SessionRoomObjects::load(malformed,restored_session);}));
        check(rejects([&]{objects.assign(player.object.identity,"player",session,DungeonLayout(1,1,{DungeonCell{}}));}));
        std::cout<<"ordered live Session bindings preserve scope, identity, activation, snapshots and validated save references\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
