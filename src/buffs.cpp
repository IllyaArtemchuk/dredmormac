#include "dredmor/buffs.hpp"
#include "dredmor/legacy_number.hpp"
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <functional>
#include <algorithm>

namespace dredmor {
namespace {
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+=32;return value;}
}
BuffDefinition read_buff(const XmlNode& node) {
    if (lower(node.tag)!="buff") throw std::invalid_argument("expected buff definition");
    BuffDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);const auto& value=attribute.second;const auto n=legacy_integer(value);
        if (key=="tag") result.tag=value;
        else if (key=="description") result.description=value;
        else if (key=="gameplaytext") result.gameplay_text=value;
        else if (key=="icon") result.icon=value;
        else if (key=="smallicon") result.small_icon=value;
        else if (key=="insufficientfunds") result.insufficient_funds=value;
        else if (key=="time") result.time=n;
        else if (key=="stacksize") result.stack_size=n;
        else if (key=="applytoplayer") result.apply_to_player=n;
        else if (key=="destroyonmove") result.destroy_on_move=n;
        else if (key=="brittle") result.brittle=n;
        else if (key=="attacks") result.attacks=n;
        else if (key=="manaupkeep") result.mana_upkeep=n;
        else if (key=="zorkmidupkeep") result.zorkmid_upkeep=n;
        else if (key=="requiresweapon") result.requires_weapon=n;
        else if (key=="requiresshield") result.requires_shield=n;
        else if (key=="zorkmidscalef") result.zorkmid_scale=static_cast<float>(std::strtod(value.c_str(),nullptr));
        else if (key=="usetimer") result.use_timer=n!=0;
        else if (key=="allowstacking") result.allow_stacking=n!=0;
        else if (key=="self") result.self=n!=0;
        else if (key=="bad") result.bad=n!=0;
        else if (key=="removable") result.removable=n!=0;
        else if (key=="digglegod") result.diggle_god=n!=0;
    }
    for (const auto& child:node.children) {
        const auto tag=lower(child.tag);
        auto number=[&](const std::string& key) {int value=0;for (const auto& a:child.attributes) if (lower(a.first)==key) value=legacy_integer(a.second);return value;};
        if (tag=="primarybuff" || tag=="highbuff" || tag=="secondarybuff") {
            const auto id=number("id"),amount=number("amount");const bool primary=tag!="secondarybuff";
            if (id<0 || id>=(primary?6:24)) throw std::invalid_argument("buff stat ID outside range");
            if (primary) result.modifiers.primary[id]=amount;else result.modifiers.secondary[id]=amount;
        } else if (tag=="sightbuff") result.modifiers.secondary[18]=number("amount");
        else if (tag=="invisible") result.invisible=true;
        else if (tag=="mute") result.mute=true;
        else if (tag=="damagebuff" || tag=="resistbuff") {
            auto& values=tag=="damagebuff"?result.modifiers.damage:result.modifiers.resistance;
            for (const auto& a:child.attributes) for (std::size_t i=0;i<damage_names.size();++i)
                if (a.first==damage_names[i]) values[i]=legacy_integer(a.second);
        }
    }
    return result;
}
std::uint64_t buff_signature(const BuffDefinition& buff) {
    std::uint64_t hash=14695981039346656037ull;
    auto add=[&](const std::string& value) {for (unsigned char c:value) {hash^=c;hash*=1099511628211ull;}hash^=0;hash*=1099511628211ull;};
    for (const auto* value:{&buff.tag,&buff.description,&buff.gameplay_text,&buff.icon,&buff.small_icon,&buff.insufficient_funds}) add(*value);
    for (auto value:buff.modifiers.primary) add(std::to_string(value));
    for (auto value:buff.modifiers.secondary) add(std::to_string(value));
    for (auto value:buff.modifiers.damage) add(std::to_string(value));
    for (auto value:buff.modifiers.resistance) add(std::to_string(value));
    for (int value:{buff.time,buff.stack_size,buff.apply_to_player,buff.destroy_on_move,buff.brittle,buff.attacks,buff.mana_upkeep,buff.zorkmid_upkeep,buff.requires_weapon,buff.requires_shield,
        int(buff.use_timer),int(buff.allow_stacking),int(buff.self),int(buff.bad),int(buff.removable),int(buff.diggle_god),int(buff.invisible),int(buff.mute)}) add(std::to_string(value));
    std::uint32_t bits;std::memcpy(&bits,&buff.zorkmid_scale,4);add(std::to_string(bits));
    std::function<void(const XmlNode&)> source=[&](const XmlNode& node) {add(node.tag);add(node.text);for (const auto& a:node.attributes) {add(a.first);add(a.second);}for (const auto& child:node.children) source(child);};
    source(buff.source);return hash?hash:1;
}
std::vector<std::size_t> buff_order(const std::vector<std::string>& names,int depth) {
    std::vector<std::size_t> order;for (std::size_t i=0;i<names.size();++i) order.push_back(i);
    auto less=[&](std::size_t a,std::size_t b){return names[a]<names[b];};
    auto heap_adjust=[&](std::size_t first,std::size_t length,std::size_t hole,std::size_t value) {
        const auto top=hole;auto child=2*(hole+1);
        while (child<length) {
            if (less(order[first+child],order[first+child-1])) --child;
            order[first+hole]=order[first+child];hole=child;child=2*(hole+1);
        }
        if (child==length) {order[first+hole]=order[first+child-1];hole=child-1;}
        while (hole>top) {const auto parent=(hole-1)/2;if (!less(order[first+parent],value)) break;order[first+hole]=order[first+parent];hole=parent;}
        order[first+hole]=value;
    };
    auto heap_sort=[&](std::size_t first,std::size_t last) {
        auto length=last-first;if (length<2) return;
        auto parent=(length-2)/2;
        for (;;) {heap_adjust(first,length,parent,order[first+parent]);if (!parent) break;--parent;}
        while (length>1) {const auto value=order[first+length-1];order[first+length-1]=order[first];--length;heap_adjust(first,length,0,value);}
    };
    std::function<void(std::size_t,std::size_t,int)> partition=[&](std::size_t first,std::size_t last,int budget) {
        while (last-first>16) {
            if (!budget) {heap_sort(first,last);return;}
            --budget;const auto a=order[first],b=order[first+(last-first)/2],c=order[last-1];
            const auto pivot=less(a,b)?(less(b,c)?b:less(a,c)?c:a):(less(a,c)?a:less(b,c)?c:b);
            auto left=first,right=last;
            for (;;) {
                while (less(order[left],pivot)) ++left;
                do {--right;} while (less(pivot,order[right]));
                if (right<=left) break;
                std::swap(order[left],order[right]);++left;
            }
            partition(left,last,budget);last=left;
        }
    };
    if (depth<0) {depth=0;for (auto size=order.size();size>1;size>>=1) depth+=2;}
    partition(0,order.size(),depth);
    // Insertion finishing preserves the partition's tie order. The guarded
    // form has the same comparisons/results as the original sentinel form.
    for (std::size_t i=1;i<order.size();++i) {
        const auto value=order[i];auto at=i;
        while (at && less(value,order[at-1])) {order[at]=order[at-1];--at;}
        order[at]=value;
    }
    return order;
}
BuffTick tick_buff(BuffClock clock) {
    bool deapply=false;
    if (!clock.permanent) {
        const auto bits=std::uint32_t(clock.remaining)-1;std::memcpy(&clock.remaining,&bits,4);
        deapply=clock.pending || (clock.remaining<0 && clock.use_timer);
    }
    const bool remove=clock.pending || (clock.remaining<0 && !clock.permanent && clock.use_timer);
    return {clock,deapply,remove};
}
}
