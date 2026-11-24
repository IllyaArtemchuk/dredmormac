#include "dredmor/session_room_objects.hpp"
#include "dredmor/session.hpp"
#include "dredmor/session_room_scripts.hpp"
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
        const auto before_pickup=session;session.pickup();check(!objects.object(session,ground.object.identity));
        check(rejects([&]{saved(objects,session);}));
        objects.synchronize_items(before_pickup,session);
        check(!objects.find(session,3,"loot") && objects.carried().size()==1);
        check(objects.carried()[0].object.identity==ground.object.identity && objects.carried()[0].object.level==0 && objects.carried()[0].object.position.y==3);
        const auto carried_bindings=saved(objects,session);std::ostringstream carried_session;session.save(carried_session);
        std::istringstream carried_input(carried_session.str());auto carried_restored=Session::load(carried_input);
        auto carried_objects=SessionRoomObjects::load(parse_xml(carried_bindings),carried_restored);
        check(saved(carried_objects,carried_restored)==carried_bindings && carried_objects.carried().size()==1);
        SessionRoomScriptProfile carried_profile(layout);carried_profile.rooms.resize(9);carried_profile.programs.resize(9);carried_profile.sources.resize(9,parse_xml("<room/>"));
        for (std::size_t i=0;i<carried_profile.rooms.size();++i) carried_profile.rooms[i].trigger.identity=std::int32_t(i);
        std::ostringstream combined_carried;SessionRoomScriptExecutor::save(combined_carried,carried_restored,carried_profile,carried_objects,RoomRuntimeState{});
        std::istringstream combined_input(combined_carried.str());auto combined_snapshot=SessionRoomScriptExecutor::load(combined_input,carried_profile);
        check(combined_snapshot.objects.carried().size()==1 && combined_snapshot.objects.carried()[0].object.identity==ground.object.identity);
        auto unsupported_legacy=combined_carried.str();const auto legacy_version=unsupported_legacy.find("version=\"2\"");check(legacy_version!=std::string::npos);
        unsupported_legacy.replace(legacy_version,11,"version=\"1\"");
        check(rejects([&]{std::istringstream legacy(unsupported_legacy);SessionRoomScriptExecutor::load(legacy,carried_profile);}));
        auto invalid_carried=parse_xml(carried_bindings);invalid_carried.children.back().attributes["subject"]="78";
        check(rejects([&]{SessionRoomObjects::load(invalid_carried,carried_restored);}));
        const auto before_drop=carried_restored;auto dropped=carried_restored.inventory().remove(77);check(dropped);
        carried_restored.drop(*dropped,{7,3});carried_objects.synchronize_items(before_drop,carried_restored);
        check(carried_objects.carried().empty() && carried_objects.bindings().back().object.identity==ground.object.identity);
        check(carried_objects.find(carried_restored,3,"loot")->position.x==7 && !carried_objects.find(carried_restored,8,"loot"));
        // Consuming/removing an owned instance retires its binding rather than
        // changing it to the identity of a stack or a future factory item.
        auto after_consumed=session;check(after_consumed.inventory().remove(77));
        objects.synchronize_items(session,after_consumed);check(objects.carried().empty());
        check(!objects.unregister(ground.object.identity));
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
