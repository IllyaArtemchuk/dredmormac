#include "dredmor/content.hpp"
#include "dredmor/game_tick.hpp"
#include "dredmor/room_deferred.hpp"
#include "dredmor/room_runtime.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room puzzle assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomDefinition room;room.name="Synthetic switch puzzle";room.rows={"#####","#123#","#####"};
        room.source=parse_xml("<room width='5' height='3'>"
            "<script><condition condition_type='activated_changed' id='switch' activated='1'/>"
            "<action on='success' action_type='remove' id='gate'/>"
            "<action on='success' action_type='create' create_type='customblocker' at='3' png='synthetic.spr' id='replacement'/>"
            "<action on='success' action_type='move' id='player' at='2'/>"
            "<action on='success' action_type='ticker' text='switch fired'/></script>"
            "<script><condition condition_type='at' id='gate' at='2'/>"
            "<action on='success' action_type='ticker' text='gate remains until destruction phase'/></script>"
            "<script><condition condition_type='at' id='gate' at='2' not='1'/>"
            "<action on='success' action_type='ticker' text='gate removed'/></script></room>");
        const auto program=room.script_program();check(program.scripts.size()==3 && program.created_records.size()==1);
        auto definitions=std::make_shared<const std::vector<RoomScriptDefinition>>(program.scripts);
        std::vector<RoomRuntimeRoom> rooms={{{true,1,0,definitions},{0,0},1}};RoomRuntimeState scripts;
        RoomDeferredState deferred;deferred.current_level=1;deferred.placement.levels[1]={5,3,{3,1,2},{}};
        deferred.objects[1]={RoomObjectIdentity{1,1,0,"switch",false,{1,1}},false,RoomSquareIdentity{1,{1,1}}};
        deferred.objects[2]={RoomObjectIdentity{2,1,0,"gate",false,{2,1}},false,RoomSquareIdentity{1,{2,1}}};
        deferred.objects[3]={RoomObjectIdentity{3,1,0,"player",true,{1,1}},false,RoomSquareIdentity{1,{1,1}}};
        deferred.placement.squares[{1,{1,1}}]={3,1};deferred.placement.squares[{1,{2,1}}]={2};
        ActivationState activation;activation.activate(true);GameRandom random;random.seed(7);
        GameTickState tick;tick.level=TickLevelState{5,3,true,{3,1,2},std::vector<std::vector<std::uint64_t>>(15)};tick.player.position={1,1};
        std::vector<std::string> messages;std::vector<std::uint64_t> updates,posts;unsigned teleported=0;
        auto find=[&](std::int32_t scope,std::string_view id)->std::optional<ScriptObjectState> {
            std::vector<RoomObjectIdentity> registered;for (auto identity:deferred.placement.levels.at(1).objects) registered.push_back(deferred.objects.at(identity).room);
            const auto found=find_room_object(registered,1,scope,id);if (!found.object) return std::nullopt;
            const auto& placed=deferred.objects.at(*found.object);ScriptObjectState object;object.identity=*found.object;object.position=placed.room.position;object.room_local_id=placed.room.id;
            if (*found.object==1) {object.activatable=true;object.activation=activation;}return object;
        };
        RoomRuntimeHooks runtime;runtime.player=3;runtime.find_object=find;runtime.player_room=[] {return 0;};runtime.player_position=[&] {return deferred.objects.at(3).room.position;};
        runtime.perform=[&](const RoomActionRequest& request) {
            check(request.room==0);
            switch (request.kind) {
                case RoomActionRequestKind::destroy:queue_room_destruction(deferred,request.subject);break;
                case RoomActionRequestKind::teleport:check(request.forced);tick.teleports.push_back({request.subject,1,request.target});break;
                case RoomActionRequestKind::ticker:messages.push_back(request.text);break;
                case RoomActionRequestKind::create: {
                    check(request.subject==1 && !deferred.objects.count(4));
                    const auto object=create_custom_room_object(std::get<CustomRoomDefinition>(program.created_records.at(request.subject-1)),request.origin,random.libc);check(object);
                    RoomPlacementObject placed;placed.room={4,0,0,object->room_local_id,false,object->position};deferred.objects[4]=placed;
                    teleport_room_object(deferred.objects.at(4),deferred.placement,object->position,1);
                    tick.level->objects=deferred.placement.levels.at(1).objects;break;
                }
                default:throw std::runtime_error("unexpected synthetic puzzle action");
            }
        };
        GameTickHooks hooks;hooks.ticks=[] {return 0U;};hooks.perform=[&](const TickRequest& request) {
            switch (request.kind) {
                case TickRequestKind::fix_safe_teleports:
                    deferred.teleports.swap(tick.teleports);fix_room_teleports(deferred);deferred.teleports.swap(tick.teleports);tick.level->objects=deferred.placement.levels.at(1).objects;break;
                case TickRequestKind::object_update:
                    updates.push_back(request.object);if (request.object==1) activation.advance_history();break;
                case TickRequestKind::room_triggers:
                    check(!teleported);run_room_scripts(rooms,scripts,runtime);break;
                case TickRequestKind::clear_destruction:
                    deferred.teleports.swap(tick.teleports);clear_room_destruction(deferred);deferred.teleports.swap(tick.teleports);tick.level->objects=deferred.placement.levels.at(1).objects;break;
                case TickRequestKind::post_game_tick:posts.push_back(request.object);break;
                case TickRequestKind::teleport:
                    check(request.object==3 && deferred.objects.at(2).destroyed);teleport_room_object(deferred.objects.at(3),deferred.placement,request.position,request.level);tick.player.position=request.position;++teleported;break;
                default:break;
            }
        };
        run_game_tick(tick,random,hooks);
        check((updates==std::vector<std::uint64_t>{3,1,2}) && (posts==std::vector<std::uint64_t>{3,1,4}));
        check((messages==std::vector<std::string>{"switch fired","gate remains until destruction phase"}));
        check(deferred.objects.at(2).destroyed && deferred.objects.at(4).room.id=="replacement" && tick.player.position.x==2 && teleported==1);
        check(scripts.triggers.success_counts.at({0,0})==1 && scripts.triggers.success_counts.at({0,1})==1 && scripts.triggers.success_counts.at({0,2})==0);
        // The next trigger pass observes completed removal; already successful
        // scripts stay exhausted. New objects now receive Update as well.
        updates.clear();posts.clear();teleported=0;run_game_tick(tick,random,hooks);
        check(messages.size()==3 && messages.back()=="gate removed" && scripts.triggers.success_counts.at({0,2})==1 && !teleported);
        check((updates==std::vector<std::uint64_t>{3,1,4}) && (posts==std::vector<std::uint64_t>{3,1,4}));
        std::cout<<"linked XML switch puzzle creates a replacement, retains pending-removal lookup within the pass, destroys before post-tick, teleports last and observes removal next tick\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
