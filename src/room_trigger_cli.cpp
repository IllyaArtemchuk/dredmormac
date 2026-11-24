#include "dredmor/room_scripts.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
struct Profile {int condition=0,arrival=-1;bool event=false;};
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--events") {
            std::int32_t assigned_room;std::string assigned_id;std::size_t count;
            while (std::cin>>assigned_room>>std::quoted(assigned_id)>>count) {
                RoomEventQueue events;
                for (std::size_t i=0;i<count;++i) {
                    int action;std::int32_t room;std::string id,event;std::cin>>action>>room>>std::quoted(id)>>std::quoted(event);
                    if (!std::cin) throw std::runtime_error("truncated event fixture");int result=-1;
                    if (action==0) set_room_event(events,room,id,event);
                    else if (action==1) set_room_event(events,assigned_room,assigned_id,event);
                    else if (action==2) result=room_event_is_set(events,room,id,event);
                    else if (action==3) events.clear();
                    else throw std::invalid_argument("invalid event operation");
                    std::cout<<result<<' '<<events.size()<<' ';
                    for (const auto& entry:events) {std::cout<<entry.first.first<<' '<<std::quoted(entry.first.second)<<' '<<entry.second.size()<<' ';for (const auto& value:entry.second) std::cout<<std::quoted(value)<<' ';}
                }
                std::cout<<'\n';
            }
            return 0;
        }
        std::size_t count,passes;bool debug,queued;RoomTriggerState state;std::size_t player;
        while (std::cin>>count>>passes>>debug>>state.previous_room>>player>>queued) {
            if (!count || count>256 || passes>16 || player>=count) throw std::invalid_argument("invalid trigger fixture dimensions");
            std::vector<RoomTriggerRoom> rooms(count);std::vector<std::vector<Profile>> profiles(count);
            for (std::size_t r=0;r<count;++r) {
                auto& room=rooms[r];bool has_template;std::size_t scripts;std::cin>>room.present>>room.content_filled>>room.identity>>has_template>>scripts;
                if (scripts>4096) throw std::invalid_argument("too many trigger scripts");auto definitions=std::make_shared<std::vector<RoomScriptDefinition>>(scripts);profiles[r].resize(scripts);
                for (std::size_t s=0;s<scripts;++s) std::cin>>definitions->at(s).success_repeat>>profiles[r][s].condition>>profiles[r][s].event>>profiles[r][s].arrival;
                if (has_template) room.scripts=definitions;
            }
            std::size_t entries;std::cin>>entries;state.success_counts.clear();state.events.clear();if (queued) set_room_event(state.events,0,"queued","event");
            for (std::size_t i=0;i<entries;++i) {std::int32_t room,script,value;std::cin>>room>>script>>value;state.success_counts[{room,script}]=value;}
            if (!std::cin) throw std::runtime_error("truncated trigger fixture");
            std::vector<std::array<std::int64_t,8>> trace;
            auto record=[&](int kind,std::size_t room,std::size_t script) {
                const auto key=RoomScriptKey{std::int32_t(room),std::int32_t(script)};const auto found=state.success_counts.find(key);
                trace.push_back({kind,std::int64_t(room),std::int64_t(script),state.previous_room,found!=state.success_counts.end(),found==state.success_counts.end()?0:found->second,!state.events.empty(),rooms[player].identity});
            };
            RoomTriggerCallbacks callbacks;
            callbacks.player_room=[&] {return rooms[player].identity;};
            callbacks.conditions_positive=[&](std::size_t room,std::size_t script) {record(0,room,script);const auto mode=profiles[room][script].condition;return mode==1 || (mode==2 && !state.events.empty());};
            callbacks.success_actions=[&](std::size_t room,std::size_t script) {
                record(1,room,script);const auto& profile=profiles[room][script];if (profile.event) set_room_event(state.events,0,"queued","event");
                if (profile.arrival>=0) {if (std::size_t(profile.arrival)>=rooms.size()) throw std::invalid_argument("invalid script arrival");player=std::size_t(profile.arrival);}
            };
            auto emit=[](auto value) {std::cout<<value<<' ';};
            for (std::size_t pass=0;pass<passes;++pass) {
                const auto result=run_room_trigger_logic(rooms,state,callbacks,debug);
                emit(state.previous_room);emit(state.events.size());emit(result.invalid_room_ids.size());for (auto room:result.invalid_room_ids) emit(room);
                emit(result.checked.size());for (auto key:result.checked) {emit(key.first);emit(key.second);}
                emit(result.conditions.size());for (auto key:result.conditions) {emit(key.first);emit(key.second);}
                emit(result.successes.size());for (auto key:result.successes) {emit(key.first);emit(key.second);}
            }
            emit(state.success_counts.size());for (auto counter:state.success_counts) {emit(counter.first.first);emit(counter.first.second);emit(counter.second);}
            emit(trace.size());for (auto entry:trace) for (auto value:entry) emit(value);std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
