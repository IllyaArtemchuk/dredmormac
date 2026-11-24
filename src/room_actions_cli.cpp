#include "dredmor/room_scripts.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--metadata") {
            std::string xml;
            while (std::cin>>std::quoted(xml)) {
                RoomStamp room;room.width=room.height=7;room.cells.resize(49);std::string spell_name,record_type;RoomAttributes record_attributes;
                RoomActionReaders readers;
                readers.spell=[&](std::string_view name){spell_name=name;return ScriptSpellBinding{17,"cast"};};
                readers.record=[&](const XmlNode& record,const RoomMarkers&,RoomStamp&) {record_type=record.tag;record_attributes=record.attributes;return std::uint64_t(23);};
                const auto action=read_room_action(parse_xml(xml),{{'3',{2,4}}},room,readers);
                auto emit=[](auto value){std::cout<<value<<' ';};
                emit(int(action.kind));emit(std::quoted(action.object_id));emit(std::quoted(action.text));emit(std::quoted(action.property));emit(action.value);emit(action.point.x);emit(action.point.y);
                emit(action.spell_origin.x);emit(action.spell_origin.y);emit(action.spell_target.x);emit(action.spell_target.y);emit(action.spell);emit(action.record);emit(std::quoted(action.cast_sound));emit(std::quoted(spell_name));emit(std::quoted(record_type));
                emit(record_attributes.size());for (const auto& attribute:record_attributes) {emit(std::quoted(attribute.first));emit(std::quoted(attribute.second));}
                for (auto cell:room.cells) emit(unsigned(cell.kind));std::cout<<'\n';
            }
            return 0;
        }
        RoomActionContext context;Position player;ScriptObjectState object;int blocking,player_blocking=0;bool failure;
        while (std::cin>>context.room>>context.origin.x>>context.origin.y>>player.x>>player.y>>object.position.x>>object.position.y
            >>object.timer.duration>>object.timer.remaining>>object.timer.step>>blocking>>failure) {
            std::size_t count;std::cin>>count;if (count>4096) throw std::invalid_argument("too many script actions");
            std::vector<RoomAction> actions(count);std::vector<RoomActionRequest> requests;std::vector<ClockEvent> events;std::vector<std::size_t> lookups;
            object.identity=2;object.clock=true;bool player_flag=false;player_blocking=0;
            for (std::size_t i=0;i<count;++i) {
                int kind,found,property;bool sound;auto& action=actions[i];std::cin>>kind>>found>>property>>action.value>>action.point.x>>action.point.y
                    >>action.spell_origin.x>>action.spell_origin.y>>action.spell_target.x>>action.spell_target.y>>sound>>action.record>>action.spell;
                if (property<0 || property>4) throw std::invalid_argument("invalid property fixture");
                action.kind=RoomActionKind(kind);action.object_id=std::string(found==2?"player":found==1?"found":"missing")+std::to_string(i);
                action.property=std::array<const char*,5>{"passable","started","stopped","unknown","Started"}[property];action.text="script text";action.cast_sound=sound?"cast":"";
            }
            if (!std::cin) throw std::runtime_error("truncated script action fixture");
            context.player=1;context.player_position=[&] {return player;};
            context.find_object=[&](std::int32_t room,std::string_view name)->std::optional<ScriptObjectState> {
                if (room!=context.room) throw std::logic_error("wrong action room");
                const auto player_target=name.substr(0,6)=="player",found=name.substr(0,5)=="found";lookups.push_back(std::stoull(std::string(name.substr(player_target?6:found?5:7))));
                if (player_target) {ScriptObjectState result;result.identity=1;result.position=player;return result;}
                return found?std::optional<ScriptObjectState>(object):std::nullopt;
            };
            context.perform=[&](const RoomActionRequest& request) {
                requests.push_back(request);
                if (request.kind==RoomActionRequestKind::teleport) {if (request.subject==1) player=request.target;else object.position=request.target;}
                if (request.kind==RoomActionRequestKind::set_blocking) {if (request.subject==1) player_blocking=request.value;else blocking=request.value;}
                if (request.kind==RoomActionRequestKind::player_action_flag) player_flag=true;
                if (request.kind==RoomActionRequestKind::clock_start || request.kind==RoomActionRequestKind::clock_stop) {
                    const auto result=advance_clock(object.timer,request.kind==RoomActionRequestKind::clock_start?ClockAction::start:ClockAction::stop);object.timer=result.state;events.insert(events.end(),result.events.begin(),result.events.end());
                }
            };
            perform_room_actions(actions,context);
            auto emit=[](auto value){std::cout<<value<<' ';};emit(object.position.x);emit(object.position.y);emit(player.x);emit(player.y);emit(blocking);emit(player_blocking);emit(object.timer.duration);emit(object.timer.remaining);emit(object.timer.step);emit(player_flag);emit(requests.size());
            for (const auto& request:requests) {emit(int(request.kind));emit(request.subject);emit(request.caster);emit(request.origin.x);emit(request.origin.y);emit(request.target.x);emit(request.target.y);emit(request.value);emit(request.forced);emit(std::quoted(request.text));}
            emit(events.size());for (const auto& event:events) {emit(std::quoted(event.name));emit(event.state.duration);emit(event.state.remaining);emit(event.state.step);}
            emit(lookups.size());for (auto index:lookups) emit(index);std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
