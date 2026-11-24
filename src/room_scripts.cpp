#include "dredmor/room_scripts.hpp"
#include "dredmor/legacy_number.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace dredmor {
RoomCondition read_room_condition(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room) {
    auto lower=[](std::string text) {for (auto& c:text) if (c>='A' && c<='Z') c=char(c-'A'+'a');return text;};
    if (lower(node.tag)!="condition") throw std::invalid_argument("expected room condition");
    RoomAttributes attributes;
    auto assign=[&](const std::string& key) {const auto found=node.attributes.find(key);if (found!=node.attributes.end()) attributes[lower(key)]=found->second;};
    for (const auto& key:node.attribute_order) assign(key);
    for (const auto& entry:node.attributes) if (std::find(node.attribute_order.begin(),node.attribute_order.end(),entry.first)==node.attribute_order.end()) assign(entry.first);
    auto take=[&](const char* name,bool required=false,std::string fallback="") {
        const auto found=attributes.find(name);
        if (found==attributes.end()) {if (required) throw std::invalid_argument(std::string("room condition requires ")+name);return fallback;}
        auto value=found->second;attributes.erase(found);return value;
    };
    const auto type=take("condition_type",true);RoomCondition result;result.wanted=0;
    const std::array<std::pair<const char*,RoomConditionKind>,11> kinds{{{"isdead",RoomConditionKind::is_dead},{"isalive",RoomConditionKind::is_alive},{"activated",RoomConditionKind::activated},{"activated_changed",RoomConditionKind::activation_changed},{"at",RoomConditionKind::at},{"event",RoomConditionKind::event},{"type_at",RoomConditionKind::type_at},{"entered",RoomConditionKind::entered},{"exited",RoomConditionKind::exited},{"started",RoomConditionKind::started},{"stopped",RoomConditionKind::stopped}}};
    const auto found=std::find_if(kinds.begin(),kinds.end(),[&](const auto& entry){return entry.first==type;});
    if (found==kinds.end()) throw std::invalid_argument("unknown room condition type");
    result.kind=found->second;
    result.negated=legacy_integer(take("not"))!=0;
    if (result.kind!=RoomConditionKind::entered && result.kind!=RoomConditionKind::exited && result.kind!=RoomConditionKind::type_at) {
        result.object_id=take("id",true);
        const bool missing=result.kind==RoomConditionKind::is_dead || result.kind==RoomConditionKind::is_alive || result.kind==RoomConditionKind::activation_changed;
        result.missing_value=legacy_integer(take("notfound",false,missing?"1":"0"))!=0;
    }
    auto candidate=room;
    if (result.kind==RoomConditionKind::at || result.kind==RoomConditionKind::type_at) {
        const auto point=take_room_point(attributes,"",markers,candidate);if (!point) throw std::invalid_argument("room condition requires a point");result.point=*point;
    }
    if (result.kind==RoomConditionKind::activation_changed) result.wanted=legacy_integer(take("activated",false,"-1"));
    if (result.kind==RoomConditionKind::activated || result.kind==RoomConditionKind::started || result.kind==RoomConditionKind::stopped)
        result.wanted=legacy_integer(take(result.kind==RoomConditionKind::activated?"activated":result.kind==RoomConditionKind::started?"started":"stopped",false,"1"))!=0;
    if (result.kind==RoomConditionKind::event) result.event_name=take("event_type",true);
    if (result.kind==RoomConditionKind::type_at) {result.item_type=take("type",true);result.item_subtype=take("subtype",false,"random");}
    if (!attributes.empty() || !node.children.empty()) throw std::invalid_argument("unsupported room condition field");
    room=std::move(candidate);return result;
}
RoomConditionValue room_condition_value(const RoomCondition& condition,const RoomScriptContext& context) {
    auto wrapped=[](int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t result;std::memcpy(&result,&bits,4);return result;};
    if (condition.kind==RoomConditionKind::entered) return {context.player_room==context.room && context.previous_room!=context.room,{}};
    if (condition.kind==RoomConditionKind::exited) return {context.previous_room==context.room && context.player_room!=context.room,{}};
    if (condition.kind==RoomConditionKind::type_at) {
        if (!condition.cached_item) {
            if (!context.create_item) throw std::invalid_argument("type-at condition requires prototype factory");
            const auto item=context.create_item(condition.item_type,condition.item_subtype);
            if (!item) throw std::runtime_error("type-at prototype creation failed");
            condition.cached_item=item;
        }
        if (!context.tile_items) throw std::invalid_argument("type-at condition requires tile object query");
        const auto objects=context.tile_items({wrapped(context.origin.x,condition.point.x),wrapped(context.origin.y,condition.point.y)});
        for (const auto& object:objects) if (object.item && object.type.kind==condition.cached_item->kind &&
            (condition.item_subtype=="random" || object.type.subtype==condition.cached_item->subtype)) return {true,{}};
        return {};
    }
    if (!context.find_object) throw std::invalid_argument("room condition requires object lookup");
    const auto object=context.find_object(context.room,condition.object_id);
    if (!object) return {condition.missing_value,{}};
    switch (condition.kind) {
        case RoomConditionKind::object:return {};
        case RoomConditionKind::at:return {object->position.x==wrapped(context.origin.x,condition.point.x) && object->position.y==wrapped(context.origin.y,condition.point.y),{}};
        case RoomConditionKind::is_dead:case RoomConditionKind::is_alive: {
            RoomConditionValue result;result.positive=condition.kind==RoomConditionKind::is_dead?object->dead!=0:object->dead==0;
            if (!object->monster) result.error=RoomConditionError::monster_required;
            return result;
        }
        case RoomConditionKind::activation_changed:
            if (!object->activatable) return {false,RoomConditionError::activatable_required};
            return {object->activation.changed() && (std::uint32_t(condition.wanted)>=2 || object->activation.activated()==(condition.wanted!=0)),{}};
        case RoomConditionKind::activated:
            if (!object->activatable) return {false,RoomConditionError::activatable_required};
            return {unsigned(object->activation.activated())==(std::uint32_t(condition.wanted)&255U),{}};
        case RoomConditionKind::stopped:case RoomConditionKind::started:
            if (!object->clock) return {false,RoomConditionError::clock_required};
            return {unsigned(condition.kind==RoomConditionKind::stopped?object->timer.stopped():!object->timer.stopped())==(std::uint32_t(condition.wanted)&255U),{}};
        case RoomConditionKind::event:
            if (!context.event_occurred) throw std::invalid_argument("room condition requires event lookup");
            return {context.event_occurred(context.room,object->room_local_id,condition.event_name),{}};
        default:throw std::invalid_argument("invalid room condition kind");
    }
}
RoomScriptEvaluation all_room_conditions_positive(const std::vector<RoomCondition>& conditions,const RoomScriptContext& context) {
    RoomScriptEvaluation result;
    for (std::size_t i=0;i<conditions.size();++i) {
        result.evaluated.push_back(i);const auto value=room_condition_value(conditions[i],context);
        if (value.error) result.errors.emplace_back(i,*value.error);
        if (value.positive==conditions[i].negated) {result.positive=false;break;}
    }
    return result;
}
RoomAction read_room_action(const XmlNode& node,const RoomMarkers& markers,RoomStamp& room,const RoomActionReaders& readers) {
    auto lower=[](std::string text) {for (auto& c:text) if (c>='A' && c<='Z') c=char(c-'A'+'a');return text;};
    if (lower(node.tag)!="action" || !node.children.empty()) throw std::invalid_argument("expected room action attributes");
    RoomAttributes attributes;
    auto assign=[&](const std::string& key) {const auto found=node.attributes.find(key);if (found!=node.attributes.end()) attributes[lower(key)]=found->second;};
    for (const auto& key:node.attribute_order) assign(key);
    for (const auto& entry:node.attributes) if (std::find(node.attribute_order.begin(),node.attribute_order.end(),entry.first)==node.attribute_order.end()) assign(entry.first);
    auto take=[&](const char* name,bool required=true) {
        const auto found=attributes.find(name);
        if (found==attributes.end()) {if (required) throw std::invalid_argument(std::string("room action requires ")+name);return std::string{};}
        auto value=found->second;attributes.erase(found);return value;
    };
    const auto type=take("action_type");RoomAction result;auto candidate=room;
    if (type=="create") {
        result.kind=RoomActionKind::create;const auto record_type=take("create_type");
        if (!readers.record) throw std::invalid_argument("create action requires record reader");
        XmlNode record;record.tag=record_type;record.attributes=std::move(attributes);
        result.record=readers.record(record,markers,candidate);room=std::move(candidate);return result;
    }
    if (type=="move") result.kind=RoomActionKind::move;
    else if (type=="remove") result.kind=RoomActionKind::remove;
    else if (type=="ticker") result.kind=RoomActionKind::ticker;
    else if (type=="set") result.kind=RoomActionKind::set_property;
    else if (type=="spell") result.kind=RoomActionKind::spell;
    else throw std::invalid_argument("unknown room action type");
    if (result.kind==RoomActionKind::move || result.kind==RoomActionKind::remove || result.kind==RoomActionKind::set_property) result.object_id=take("id");
    if (result.kind==RoomActionKind::move) {
        const auto point=take_room_point(attributes,"",markers,candidate);if (!point) throw std::invalid_argument("move action requires point");result.point=*point;
    }
    if (result.kind==RoomActionKind::ticker) result.text=take("text");
    if (result.kind==RoomActionKind::set_property) {result.property=take("set_type");result.value=legacy_integer(take("value"));}
    std::string spell;
    if (result.kind==RoomActionKind::spell) {
        spell=take("casts");
        if (const auto point=take_room_point(attributes,"source",markers,candidate)) result.spell_origin=*point;
        if (const auto point=take_room_point(attributes,"target",markers,candidate)) result.spell_target=*point;
    }
    if (!attributes.empty()) throw std::invalid_argument("unsupported room action field");
    if (result.kind==RoomActionKind::spell) {
        if (!readers.spell) throw std::invalid_argument("spell action requires spell resolver");
        const auto binding=readers.spell(spell);result.spell=binding.identity;result.cast_sound=binding.cast_sound;
    }
    room=std::move(candidate);return result;
}
std::vector<RoomActionRequest> perform_room_action(const RoomAction& action,const RoomActionContext& context) {
    std::vector<RoomActionRequest> result;
    auto emit=[&](RoomActionRequest request) {result.push_back(request);if (context.perform) context.perform(request);};
    auto request=[&](RoomActionRequestKind kind,std::uint64_t subject=0) {RoomActionRequest value;value.kind=kind;value.subject=subject;return value;};
    auto offset=[&](Position point) {
        auto add=[](int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;};
        return Position{add(context.origin.x,point.x),add(context.origin.y,point.y)};
    };
    if (action.kind==RoomActionKind::create) {
        auto value=request(RoomActionRequestKind::create,action.record);value.origin=context.origin;emit(value);return result;
    }
    if (action.kind==RoomActionKind::ticker) {
        auto value=request(RoomActionRequestKind::ticker);value.text=action.text;value.value=1;emit(value);return result;
    }
    if (action.kind==RoomActionKind::spell) {
        emit(request(RoomActionRequestKind::player_idle,context.player));
        auto flag=request(RoomActionRequestKind::player_action_flag,context.player);flag.value=1;emit(flag);
        emit(request(RoomActionRequestKind::player_detach_effects,context.player));
        if (!action.cast_sound.empty()) {auto sound=request(RoomActionRequestKind::sound);sound.text=action.cast_sound;emit(sound);}
        auto point=[&](Position relative) {
            if (relative.x!=-1 || relative.y!=-1) return offset(relative);
            if (!context.player_position) throw std::invalid_argument("scripted spell requires player position");
            return context.player_position();
        };
        auto cast=request(RoomActionRequestKind::cast_spell,action.spell);cast.origin=point(action.spell_origin);cast.target=point(action.spell_target);cast.caster=context.player;cast.text="by a spell";emit(cast);return result;
    }
    if (action.kind!=RoomActionKind::move && action.kind!=RoomActionKind::remove && action.kind!=RoomActionKind::set_property) throw std::invalid_argument("invalid room action kind");
    if (!context.find_object) throw std::invalid_argument("room action requires object lookup");
    const auto object=context.find_object(context.room,action.object_id);if (!object) return result;
    if (action.kind==RoomActionKind::move) {
        auto value=request(RoomActionRequestKind::teleport,object->identity);value.target=offset(action.point);value.forced=true;emit(value);
    } else if (action.kind==RoomActionKind::remove) emit(request(RoomActionRequestKind::destroy,object->identity));
    else if (action.property=="passable") {
        auto value=request(RoomActionRequestKind::set_blocking,object->identity);value.value=action.value==0;emit(value);
    } else if (action.property=="started" || action.property=="stopped") {
        if (!object->clock) {emit(request(RoomActionRequestKind::clock_required,object->identity));return result;}
        const bool start=std::int32_t(action.property=="started")==action.value;
        emit(request(start?RoomActionRequestKind::clock_start:RoomActionRequestKind::clock_stop,object->identity));
    } else {
        auto value=request(RoomActionRequestKind::unknown_property,object->identity);value.text=action.property;emit(value);
    }
    return result;
}
void perform_room_actions(const std::vector<RoomAction>& actions,const RoomActionContext& context) {
    for (const auto& action:actions) (void)perform_room_action(action,context);
}
}
