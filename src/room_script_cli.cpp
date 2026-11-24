#include "dredmor/room_scripts.hpp"
#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if ((argc==3 || argc==4) && std::string(argv[1])=="--programs") {
            const auto content=GameContent::load(argv[2],argc==4);
            for (std::size_t i=0;i<content.rooms().size();++i) {
                const auto program=content.room_script_program(i);std::size_t conditions=0,success=0,failure=0,spells=0;
                for (const auto& script:program.scripts) {conditions+=script.conditions.size();success+=script.success.size();failure+=script.failure.size();
                    for (const auto* actions:{&script.success,&script.failure}) for (const auto& action:*actions) {
                        if (action.kind==RoomActionKind::spell) {if (!action.spell || action.spell>content.spells().size()) throw std::runtime_error("invalid linked spell");++spells;}
                        if (action.kind==RoomActionKind::create && (!action.record || action.record>program.created_records.size())) throw std::runtime_error("invalid linked record");
                    }
                }
                std::cout<<i<<' '<<program.scripts.size()<<' '<<conditions<<' '<<success<<' '<<failure<<' '<<spells<<' '<<program.created_records.size()<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--spell-sound") {
            std::string xml;while (std::cin>>std::quoted(xml)) {SpellDefinition spell;spell.source=parse_xml(xml);std::cout<<std::quoted(spell_cast_sound(spell))<<'\n';}return 0;
        }
        if (argc==2 && std::string(argv[1])=="--script-metadata") {
            std::string xml;
            while (std::cin>>std::quoted(xml)) {
                RoomStamp room;room.width=room.height=7;room.cells.resize(49);std::vector<std::string> lookups;std::vector<XmlNode> records;RoomActionReaders readers;
                readers.spell=[&](std::string_view name) {lookups.emplace_back(name);return ScriptSpellBinding{17,"cast"};};
                readers.record=[&](const XmlNode& record,const RoomMarkers&,RoomStamp&) {records.push_back(record);return std::uint64_t(records.size());};
                const auto script=read_room_script(parse_xml(xml),{{'3',{2,4}}},room,readers);auto emit=[](auto value){std::cout<<value<<' ';};
                emit(script.success_repeat);emit(script.conditions.size());
                for (const auto& c:script.conditions) {emit(int(c.kind));emit(c.negated);emit(c.missing_value);emit(c.wanted);emit(c.point.x);emit(c.point.y);emit(std::quoted(c.object_id));emit(std::quoted(c.event_name));emit(std::quoted(c.item_type));emit(std::quoted(c.item_subtype));emit(bool(c.cached_item));}
                for (const auto* actions:{&script.success,&script.failure}) {
                    emit(actions->size());for (const auto& action:*actions) {emit(int(action.kind));emit(std::quoted(action.object_id));emit(std::quoted(action.text));emit(std::quoted(action.property));emit(action.value);emit(action.point.x);emit(action.point.y);emit(action.spell_origin.x);emit(action.spell_origin.y);emit(action.spell_target.x);emit(action.spell_target.y);emit(action.spell);emit(action.record);emit(std::quoted(action.cast_sound));}
                }
                emit(lookups.size());for (const auto& name:lookups) emit(std::quoted(name));
                emit(records.size());for (const auto& record:records) {emit(std::quoted(record.tag));emit(record.attributes.size());for (const auto& attribute:record.attributes) {emit(std::quoted(attribute.first));emit(std::quoted(attribute.second));}}
                for (auto cell:room.cells) emit(unsigned(cell.kind));std::cout<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--metadata") {
            std::string xml;
            while (std::cin>>std::quoted(xml)) {
                RoomStamp room;room.width=room.height=7;room.cells.resize(49);
                const auto condition=read_room_condition(parse_xml(xml),{{'3',{2,4}}},room);
                std::cout<<int(condition.kind)<<' '<<condition.negated<<' '<<condition.missing_value<<' '<<condition.wanted<<' '<<condition.point.x<<' '<<condition.point.y<<' '
                         <<std::quoted(condition.object_id)<<' '<<std::quoted(condition.event_name)<<' '<<std::quoted(condition.item_type)<<' '<<std::quoted(condition.item_subtype)<<' '<<bool(condition.cached_item);
                for (auto cell:room.cells) std::cout<<' '<<unsigned(cell.kind);std::cout<<'\n';
            }
            return 0;
        }
        if (argc==2 && std::string(argv[1])=="--type-at") {
            RoomScriptContext context;RoomCondition condition;condition.kind=RoomConditionKind::type_at;condition.item_type="weapon";
            bool random_subtype,cached;ScriptItemType prototype;
            while (std::cin>>context.origin.x>>context.origin.y>>condition.point.x>>condition.point.y>>random_subtype>>cached>>prototype.kind>>prototype.subtype) {
                std::size_t count;std::cin>>count;if (count>4096) throw std::invalid_argument("too many script tile objects");
                std::vector<ScriptTileItem> objects(count);for (auto& object:objects) std::cin>>object.item>>object.type.kind>>object.type.subtype;
                if (!std::cin) throw std::runtime_error("truncated type-at fixture");
                condition.item_subtype=random_subtype?"random":"test weapon";condition.cached_item=cached?std::optional<ScriptItemType>(prototype):std::nullopt;
                int created=0,queried=0;Position target;
                context.create_item=[&](std::string_view type,std::string_view subtype)->std::optional<ScriptItemType> {if (type!="weapon" || subtype!=condition.item_subtype) throw std::logic_error("wrong prototype lookup");++created;return prototype;};
                context.tile_items=[&](Position point) {target=point;++queried;return objects;};
                const auto first=room_condition_value(condition,context),second=room_condition_value(condition,context);
                std::cout<<first.positive<<' '<<second.positive<<' '<<created<<' '<<queried<<' '<<condition.cached_item->kind<<' '<<condition.cached_item->subtype<<' '<<target.x<<' '<<target.y<<'\n';
            }
            return 0;
        }
        RoomScriptContext context;ScriptObjectState object;bool event;
        while (std::cin>>context.room>>context.player_room>>context.previous_room>>context.origin.x>>context.origin.y
            >>object.position.x>>object.position.y>>object.monster>>object.activatable>>object.clock>>object.dead
            >>object.activation.current>>object.activation.previous>>object.activation.older>>object.timer.step>>event) {
            std::size_t count;std::cin>>count;if (count>4096) throw std::invalid_argument("too many room conditions");
            std::vector<RoomCondition> conditions(count);
            for (std::size_t i=0;i<count;++i) {int kind;bool found;auto& c=conditions[i];std::cin>>kind>>c.negated>>c.missing_value>>c.wanted>>c.point.x>>c.point.y>>found;c.kind=RoomConditionKind(kind);c.object_id=std::string(found?"found":"missing")+std::to_string(i);c.event_name="event";}
            if (!std::cin) throw std::runtime_error("truncated room script fixture");
            std::vector<std::size_t> lookups,events;object.room_local_id="target";
            context.find_object=[&](std::int32_t room,std::string_view name)->std::optional<ScriptObjectState> {
                if (room!=context.room) throw std::logic_error("wrong script lookup room");const auto found=name.substr(0,5)=="found";lookups.push_back(std::stoull(std::string(name.substr(found?5:7))));return found?std::optional<ScriptObjectState>(object):std::nullopt;
            };
            context.event_occurred=[&](std::int32_t room,std::string_view name,std::string_view requested) {
                if (room!=context.room || name!="target" || requested!="event") throw std::logic_error("wrong script event query");events.push_back(lookups.back());return event;
            };
            const auto result=all_room_conditions_positive(conditions,context);
            auto emit=[](auto value) {std::cout<<value<<' ';};emit(result.positive);emit(result.evaluated.size());for (auto i:result.evaluated) emit(i);
            emit(result.errors.size());for (auto error:result.errors) {emit(error.first);emit(int(error.second));}
            emit(lookups.size());for (auto i:lookups) emit(i);emit(events.size());for (auto i:events) emit(i);std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
