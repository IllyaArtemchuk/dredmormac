#include "dredmor/room_runtime.hpp"
#include "dredmor/random.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("room runtime assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        RoomCondition condition;condition.kind=RoomConditionKind::type_at;condition.item_type="weapon";condition.item_subtype="specific";condition.point={1,2};
        RoomAction ticker;ticker.kind=RoomActionKind::ticker;ticker.text="matched";
        RoomScriptDefinition script;script.success_repeat=-1;script.conditions={condition};script.success={ticker};
        const auto shared=std::make_shared<const std::vector<RoomScriptDefinition>>(std::vector<RoomScriptDefinition>{script});
        std::vector<RoomRuntimeRoom> rooms={{{true,1,0,shared},{10,20},7},{{true,1,1,shared},{30,40},7}};
        RoomRuntimeState original;RoomRuntimeHooks hooks;unsigned creates=0,tickers=0;bool occupied=false;
        hooks.player_room=[] {return 0;};hooks.player_position=[] {return Position{0,0};};hooks.find_object=[](auto,auto) {return std::optional<ScriptObjectState>{};};
        hooks.create_item=[&](auto type,auto subtype) {check(type=="weapon" && subtype=="specific");++creates;return ScriptItemType{3,9};};
        hooks.tile_items=[&](Position p) {check((p.x==11 && p.y==22) || (p.x==31 && p.y==42));return occupied?std::vector<ScriptTileItem>{{true,{3,9}}}:std::vector<ScriptTileItem>{};};
        hooks.perform=[&](const RoomActionRequest& request) {check(request.kind==RoomActionRequestKind::ticker && request.text=="matched");++tickers;};
        auto candidate=original;run_room_scripts(rooms,candidate,hooks);
        check(creates==1 && !tickers && candidate.prototypes.size()==1 && original.prototypes.empty() && !shared->at(0).conditions[0].cached_item);
        occupied=true;run_room_scripts(rooms,candidate,hooks);check(creates==1 && tickers==2 && candidate.triggers.success_counts.at({0,0})==1 && candidate.triggers.success_counts.at({1,0})==1);
        // Discarded snapshots do not populate another snapshot's shared template.
        run_room_scripts(rooms,original,hooks);check(creates==2 && original.prototypes.size()==1 && !shared->at(0).conditions[0].cached_item);
        set_room_event(candidate.triggers.events,-1,"id&\"\n", "name<\t");candidate.triggers.previous_room=-1;
        std::ostringstream encoded;write_room_runtime_state(encoded,candidate,rooms);auto restored=read_room_runtime_state(parse_xml(encoded.str()),rooms);
        std::ostringstream canonical;write_room_runtime_state(canonical,restored,rooms);check(encoded.str()==canonical.str());
        check(room_event_is_set(restored.triggers.events,-1,"id&\"\n","name<\t"));
        run_room_scripts(rooms,restored,hooks);check(creates==2 && restored.prototypes.size()==1 && restored.triggers.events.empty());
        auto wrong=parse_xml(encoded.str());wrong.children={parse_xml("<prototype template='7' script='0' condition='9' kind='3' subtype='9'/>")};
        bool invalid_reference=false;try {read_room_runtime_state(wrong,rooms);} catch(const std::runtime_error&) {invalid_reference=true;}check(invalid_reference);
        auto duplicates=parse_xml(encoded.str());const auto counter=parse_xml("<counter room='0' script='0' count='0'/>");duplicates.children={counter,counter};
        bool duplicate=false;try {read_room_runtime_state(duplicates,rooms);} catch(const std::runtime_error&) {duplicate=true;}check(duplicate);
        auto changed=std::make_shared<std::vector<RoomScriptDefinition>>(*shared);changed->at(0).success[0].text="different content";
        auto another_profile=rooms;for (auto& room:another_profile) room.trigger.scripts=changed;
        bool wrong_profile=false;try {read_room_runtime_state(parse_xml(encoded.str()),another_profile);} catch(const std::runtime_error&) {wrong_profile=true;}check(wrong_profile);
        struct Snapshot {RoomRuntimeState runtime;DarwinRandom random{7};};Snapshot committed;auto failed=committed;std::int32_t first_draw=0;
        hooks.create_item=[&](auto,auto) {first_draw=failed.random.next();return ScriptItemType{3,9};};
        hooks.perform=[](const RoomActionRequest&) {throw std::runtime_error("injected resource failure");};
        bool failed_action=false;try {run_room_scripts(rooms,failed.runtime,hooks);} catch(const std::runtime_error&) {failed_action=true;}
        check(failed_action && failed.runtime.prototypes.size()==1 && committed.runtime.prototypes.empty() && committed.random.state()==7 && !shared->at(0).conditions[0].cached_item);
        auto retry=committed;std::int32_t retry_draw=0;hooks.create_item=[&](auto,auto) {retry_draw=retry.random.next();return ScriptItemType{3,9};};hooks.perform=[](const RoomActionRequest&) {};
        run_room_scripts(rooms,retry.runtime,hooks);check(first_draw==retry_draw && retry.runtime.prototypes.size()==1 && retry.runtime.triggers.success_counts.size()==2);
        RoomCondition activated;activated.kind=RoomConditionKind::activated;activated.object_id="switch";activated.wanted=1;
        RoomCondition event;event.kind=RoomConditionKind::event;event.object_id="switch";event.event_name="fired";
        RoomAction emit=ticker;emit.text="emit";RoomAction observe=ticker;observe.text="observe";
        RoomScriptDefinition first;first.conditions={activated};first.success={emit};RoomScriptDefinition second;second.conditions={event};second.success={observe};
        auto live=std::make_shared<const std::vector<RoomScriptDefinition>>(std::vector<RoomScriptDefinition>{first,second});
        RoomRuntimeState state;std::vector<std::string> messages;hooks.find_object=[](auto scope,auto id) {check(scope==0 && id=="switch");ScriptObjectState object;object.activatable=true;object.activation.current=1;object.room_local_id="actual-switch";return std::optional<ScriptObjectState>{object};};
        hooks.perform=[&](const RoomActionRequest& request) {messages.push_back(request.text);if (request.text=="emit") set_room_event(state.triggers.events,0,"actual-switch","fired");};
        auto pass=run_room_scripts({{{true,1,0,live},{0,0},8}},state,hooks);
        check(pass.successes.size()==2 && (messages==std::vector<std::string>{"emit","observe"}) && state.triggers.events.empty());
        run_room_scripts({{{true,1,0,live},{0,0},8}},state,hooks);check(messages.size()==2);
        bool collision=false;auto conflicting=rooms;conflicting[1].trigger.scripts=live;try {run_room_scripts(conflicting,state,hooks);} catch(const std::invalid_argument&) {collision=true;}check(collision);
        // A new original Level starts with previous room -1. Room zero's
        // entered script must run on its first pass, then again on re-entry.
        RoomRuntimeState new_level;check(new_level.triggers.previous_room==-1);
        RoomCondition entered;entered.kind=RoomConditionKind::entered;
        RoomScriptDefinition on_entry;on_entry.success_repeat=-1;on_entry.conditions={entered};on_entry.success={ticker};
        auto entry_scripts=std::make_shared<const std::vector<RoomScriptDefinition>>(std::vector<RoomScriptDefinition>{on_entry});
        std::vector<RoomRuntimeRoom> entry_rooms={{{true,1,0,entry_scripts},{0,0},9},{{true,1,1,{}},{4,0},0}};
        std::int32_t current_room=0;unsigned entries=0;RoomRuntimeHooks entry_hooks;
        entry_hooks.player_room=[&] {return current_room;};entry_hooks.player_position=[] {return Position{1,1};};
        entry_hooks.find_object=[](auto,auto)->std::optional<ScriptObjectState> {return std::nullopt;};entry_hooks.perform=[&](const auto&) {++entries;};
        run_room_scripts(entry_rooms,new_level,entry_hooks);check(entries==1 && new_level.triggers.previous_room==0);
        run_room_scripts(entry_rooms,new_level,entry_hooks);check(entries==1);
        current_room=1;run_room_scripts(entry_rooms,new_level,entry_hooks);check(entries==1);
        current_room=0;run_room_scripts(entry_rooms,new_level,entry_hooks);check(entries==2);
        // TypeAt caches belong to template condition objects, including when
        // their template is absent from the current floor. Counters/history
        // remain local to the floor; travelling must not reconstruct items.
        RoomRuntimeCatalogue catalogue{{7,shared},{9,entry_scripts}};
        RoomRuntimeState upstairs,downstairs;unsigned factory_calls=0;
        RoomRuntimeHooks travel_hooks=entry_hooks;
        travel_hooks.create_item=[&](auto,auto) {++factory_calls;return ScriptItemType{3,9};};
        travel_hooks.tile_items=[](Position) {return std::vector<ScriptTileItem>{{true,{3,9}}};};
        travel_hooks.perform=[](const auto&) {};
        run_room_scripts(rooms,upstairs,travel_hooks);check(factory_calls==1 && upstairs.prototypes.size()==1);
        downstairs.prototypes=upstairs.prototypes;
        std::vector<RoomRuntimeRoom> downstairs_rooms={{{true,1,0,entry_scripts},{0,0},9}};
        std::ostringstream downstairs_save;write_room_runtime_state(downstairs_save,downstairs,downstairs_rooms,catalogue);
        downstairs=read_room_runtime_state(parse_xml(downstairs_save.str()),downstairs_rooms,catalogue);
        check(downstairs.triggers.previous_room==-1 && downstairs.triggers.success_counts.empty() && downstairs.prototypes.size()==1);
        run_room_scripts(downstairs_rooms,downstairs,travel_hooks);
        upstairs.prototypes=downstairs.prototypes;
        std::ostringstream upstairs_save;write_room_runtime_state(upstairs_save,upstairs,rooms,catalogue);
        upstairs=read_room_runtime_state(parse_xml(upstairs_save.str()),rooms,catalogue);
        run_room_scripts(rooms,upstairs,travel_hooks);
        check(factory_calls==1 && upstairs.triggers.success_counts.at({0,0})==2 && downstairs.triggers.success_counts.at({0,0})==1);
        auto changed_catalogue=catalogue;changed_catalogue[7]=changed;
        bool absent_changed=false;try {read_room_runtime_state(parse_xml(downstairs_save.str()),downstairs_rooms,changed_catalogue);} catch(const std::runtime_error&) {absent_changed=true;}check(absent_changed);
        bool absent_undefined=false;try {write_room_runtime_state(canonical,downstairs,downstairs_rooms);} catch(const std::runtime_error&) {absent_undefined=true;}check(absent_undefined);
        auto conflicting_catalogue=catalogue;conflicting_catalogue[9]=shared;
        bool catalogue_collision=false;try {write_room_runtime_state(canonical,downstairs,downstairs_rooms,conflicting_catalogue);} catch(const std::runtime_error&) {catalogue_collision=true;}check(catalogue_collision);
        auto missing_catalogue=catalogue;missing_catalogue.erase(9);
        bool missing_template=false;try {write_room_runtime_state(canonical,downstairs,downstairs_rooms,missing_catalogue);} catch(const std::runtime_error&) {missing_template=true;}check(missing_template);
        auto invalid_catalogue=catalogue;invalid_catalogue[0]=shared;
        bool zero_template=false;try {write_room_runtime_state(canonical,downstairs,downstairs_rooms,invalid_catalogue);} catch(const std::runtime_error&) {zero_template=true;}check(zero_template);
        invalid_catalogue=catalogue;invalid_catalogue[10]={};
        bool null_template=false;try {write_room_runtime_state(canonical,downstairs,downstairs_rooms,invalid_catalogue);} catch(const std::runtime_error&) {null_template=true;}check(null_template);
        std::cout<<"room runtime composes live activation/events, per-room repeats and source-order actions with template-shared prototypes isolated between snapshots\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
