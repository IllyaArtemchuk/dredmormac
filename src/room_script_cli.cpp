#include "dredmor/room_scripts.hpp"
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
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
