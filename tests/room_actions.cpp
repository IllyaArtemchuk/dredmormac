#include "dredmor/room_scripts.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("script action assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomStamp room;room.width=room.height=7;room.cells.resize(49);RoomMarkers markers{{'3',{2,4}}};
        const auto moved=read_room_action(parse_xml("<action action_type='move' id='player' at='3'/>"),markers,room);
        check(moved.kind==RoomActionKind::move && moved.point.x==2 && moved.point.y==4 && room.cells[30].kind==1);
        const auto set=read_room_action(parse_xml("<action action_type='set' id='clock' set_type='started' value=' -2tail'/>"),markers,room);check(set.value==-2 && set.property=="started");
        RoomActionReaders readers;readers.spell=[](std::string_view name) {check(name=="Spell");return ScriptSpellBinding{4,"sound"};};
        const auto cast=read_room_action(parse_xml("<action action_type='spell' casts='Spell' targetat='3'/>"),markers,room,readers);check(cast.spell==4 && cast.spell_origin.x==-1 && cast.spell_target.x==2 && cast.spell_target.y==4 && cast.cast_sound=="sound");
        readers.record=[](const XmlNode& node,const RoomMarkers&,RoomStamp&) {check(node.tag=="monster" && node.attribute("name")=="Monster" && !node.attributes.count("create_type"));return std::uint64_t(7);};
        check(read_room_action(parse_xml("<action action_type='create' create_type='monster' name='Monster' at='3'/>"),markers,room,readers).record==7);
        bool rejected=false;try {read_room_action(parse_xml("<action action_type='move' id='player' x='1'/>"),markers,room);} catch(const std::invalid_argument&) {rejected=true;}check(rejected);
        RoomActionContext context;context.room=3;context.origin={10,20};Position player{1,2};std::vector<RoomActionRequest> seen;
        context.player_position=[&]{return player;};context.find_object=[](auto,std::string_view name)->std::optional<ScriptObjectState>{if (name=="missing") return std::nullopt;ScriptObjectState object;object.identity=1;object.clock=true;return object;};
        context.perform=[&](const RoomActionRequest& request){seen.push_back(request);if (request.kind==RoomActionRequestKind::teleport) player=request.target;};
        RoomAction move;move.kind=RoomActionKind::move;move.point={2,-1};move.object_id="player";
        RoomAction spell;spell.kind=RoomActionKind::spell;spell.spell=42;spell.cast_sound="cast";spell.spell_target={-1,2};
        perform_room_actions({move,spell},context);
        check(seen.size()==6 && player.x==12 && player.y==19 && seen[0].forced && seen[1].kind==RoomActionRequestKind::player_idle);
        check(seen[2].kind==RoomActionRequestKind::player_action_flag && seen[2].value==1 && seen[3].kind==RoomActionRequestKind::player_detach_effects && seen[4].kind==RoomActionRequestKind::sound);
        check(seen[5].kind==RoomActionRequestKind::cast_spell && seen[5].origin.x==12 && seen[5].origin.y==19 && seen[5].target.x==9 && seen[5].target.y==22 && seen[5].text=="by a spell");
        RoomAction property;property.kind=RoomActionKind::set_property;property.property="started";property.value=1;
        check(perform_room_action(property,context)[0].kind==RoomActionRequestKind::clock_start);property.value=2;check(perform_room_action(property,context)[0].kind==RoomActionRequestKind::clock_stop);
        property.property="stopped";property.value=0;check(perform_room_action(property,context)[0].kind==RoomActionRequestKind::clock_start);
        property.property="Started";check(perform_room_action(property,context)[0].kind==RoomActionRequestKind::unknown_property);
        property.object_id="missing";check(perform_room_action(property,context).empty());
        property.object_id="player";property.property="passable";property.value=-1;check(perform_room_action(property,context)[0].value==0);
        RoomAction remove;remove.kind=RoomActionKind::remove;remove.object_id="player";check(perform_room_action(remove,context)[0].kind==RoomActionRequestKind::destroy);
        RoomAction ticker;ticker.kind=RoomActionKind::ticker;ticker.text="message";const auto message=perform_room_action(ticker,context);check(message[0].text=="message" && message[0].value==1);
        context.perform={};RoomAction create;create.kind=RoomActionKind::create;create.record=8;const auto plan=perform_room_action(create,context);check(plan[0].subject==8 && plan[0].origin.x==10 && plan[0].origin.y==20);
        std::cout<<"ordered script move/create/remove/property/spell requests, live player coordinates, clock gates and missing-object behavior passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
