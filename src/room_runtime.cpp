#include "dredmor/room_runtime.hpp"
#include <stdexcept>

namespace dredmor {
RoomTriggerPass run_room_scripts(const std::vector<RoomRuntimeRoom>& rooms,RoomRuntimeState& state,const RoomRuntimeHooks& hooks,bool debug) {
    if (!hooks.player_room || !hooks.player_position || !hooks.find_object || !hooks.perform)
        throw std::invalid_argument("room runtime requires live object, player and action hooks");
    std::vector<RoomTriggerRoom> triggers;triggers.reserve(rooms.size());
    std::map<std::uint64_t,const std::vector<RoomScriptDefinition>*> template_sources;
    for (const auto& room:rooms) {
        if (room.trigger.scripts) {
            if (!room.template_identity) throw std::invalid_argument("room runtime requires stable template identity");
            const auto [entry,inserted]=template_sources.emplace(room.template_identity,room.trigger.scripts.get());
            if (!inserted && entry->second!=room.trigger.scripts.get()) throw std::invalid_argument("conflicting room template identity");
        }
        triggers.push_back(room.trigger);
    }
    RoomTriggerCallbacks callbacks;
    callbacks.player_room=hooks.player_room;
    callbacks.conditions_positive=[&](std::size_t room_index,std::size_t script_index) {
        const auto& room=rooms.at(room_index);auto conditions=room.trigger.scripts->at(script_index).conditions;
        for (std::size_t i=0;i<conditions.size();++i) {
            auto& condition=conditions[i];condition.cached_item.reset();
            const auto cached=state.prototypes.find({room.template_identity,script_index,i});
            if (condition.kind==RoomConditionKind::type_at && cached!=state.prototypes.end()) condition.cached_item=cached->second;
        }
        RoomScriptContext context;context.room=std::int32_t(room_index);context.player_room=hooks.player_room();context.previous_room=state.triggers.previous_room;context.origin=room.origin;
        context.find_object=hooks.find_object;context.create_item=hooks.create_item;context.tile_items=hooks.tile_items;
        context.event_occurred=[&](std::int32_t scope,std::string_view object,std::string_view event) {return room_event_is_set(state.triggers.events,scope,object,event);};
        const auto evaluation=all_room_conditions_positive(conditions,context);
        // Prototype construction can happen before a condition fails. Commit
        // every actually populated cache, including false/negated conditions.
        for (std::size_t i=0;i<conditions.size();++i) if (conditions[i].kind==RoomConditionKind::type_at && conditions[i].cached_item)
            state.prototypes[{room.template_identity,script_index,i}]=*conditions[i].cached_item;
        if (!evaluation.errors.empty()) throw std::runtime_error("room condition requires another object type");
        return evaluation.positive;
    };
    callbacks.success_actions=[&](std::size_t room_index,std::size_t script_index) {
        const auto& room=rooms.at(room_index);RoomActionContext context;
        context.room=std::int32_t(room_index);context.origin=room.origin;context.player=hooks.player;context.player_position=hooks.player_position;context.find_object=hooks.find_object;context.perform=hooks.perform;
        perform_room_actions(room.trigger.scripts->at(script_index).success,context);
    };
    return run_room_trigger_logic(triggers,state.triggers,callbacks,debug);
}
}
