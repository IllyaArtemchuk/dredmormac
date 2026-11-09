#include "dredmor/spells.hpp"
#include "dredmor/legacy_number.hpp"
#include <cstdlib>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>

namespace dredmor {
namespace {
std::int32_t scaled_resource(std::int32_t current,std::int32_t amount,float scale,std::int32_t attribute) {
    const auto bits=std::uint32_t(current)+std::uint32_t(amount);std::int32_t value;std::memcpy(&value,&bits,4);
    if (scale>0) {
        const float bonus=scale*static_cast<float>(attribute);const float result=static_cast<float>(value)+bonus;
        value=!std::isfinite(result) || result>=2147483648.0f || result< -2147483648.0f?std::numeric_limits<std::int32_t>::min():static_cast<std::int32_t>(result);
    }
    return value;
}
}
std::int32_t resolve_healing(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute) {
    if (current<1) return current;
    return std::min(scaled_resource(current,amount,scale,attribute),maximum);
}
std::int32_t resolve_spellpoints(std::int32_t current,std::int32_t maximum,std::int32_t amount,float scale,std::int32_t attribute) {
    return std::min(std::max(0,scaled_resource(current,amount,scale,attribute)),maximum);
}
namespace {
std::string lower(std::string value) {for (auto& c:value) if (c>='A' && c<='Z') c+=32;return value;}
}
ManeuverTemplate read_maneuver_template(const XmlNode& node) {
    if (lower(node.tag)!="template") throw std::invalid_argument("expected maneuver template");
    ManeuverTemplate result;result.source=node;result.id=-1;result.affects_player=false;
    for (const auto& a:node.attributes) {
        const auto key=lower(a.first);const auto value=legacy_integer(a.second);
        if (key=="name") result.id=value;else if (key=="width") result.width=value;else if (key=="height") result.height=value;
        else if (key=="affectsplayer") result.affects_player=value!=0;
    }
    if (result.width<1 || result.height<1 || result.width>256 || result.height>256) throw std::invalid_argument("maneuver template dimensions outside budget");
    for (const auto& row:node.children) if (lower(row.tag)=="row") {
        std::string text;for (const auto& a:row.attributes) if (lower(a.first)=="text") text=a.second;
        for (int x=0;x<result.width;++x) {
            // The original leaves missing cells uninitialized. Supplied
            // template22 has three such cells; the port normalizes them empty.
            const char c=std::size_t(x)<text.size()?text[x]:'.';
            if (c!='.' && c!='@' && c!='#') throw std::invalid_argument("unknown maneuver template cell");
            result.cells.push_back(c=='@'?1:c=='#'?2:0);
        }
    }
    if (result.cells.size()!=std::size_t(result.width)*result.height) throw std::invalid_argument("maneuver row count mismatch");
    return result;
}
ManeuverPlan plan_maneuver(const std::vector<ManeuverTemplate>& templates,int id,int anchored,int facing,
    Position origin,Position target,DarwinRandom& random) {
    if (facing<0 || facing>3) throw std::invalid_argument("invalid maneuver facing");
    auto candidate=random;const ManeuverTemplate* selected=nullptr;
    if (id==-1) {
        if (templates.empty() || templates.size()>0x7ffffffeU) throw std::invalid_argument("invalid random maneuver pool");
        selected=&templates[candidate.bounded(static_cast<std::uint32_t>(templates.size()))];
    } else for (const auto& entry:templates) if (entry.id==id) selected=&entry;
    ManeuverPlan result;if (!selected) return result;
    const auto& entry=*selected;
    if (entry.width<1 || entry.height<1 || entry.width>256 || entry.height>256 || entry.cells.size()!=std::size_t(entry.width)*entry.height)
        throw std::invalid_argument("invalid maneuver cells");
    const auto marker=std::find(entry.cells.begin(),entry.cells.end(),2);
    if (marker!=entry.cells.end()) {
        result.accepted=true;const int anchor_x=int(marker-entry.cells.begin())%entry.width,anchor_y=int(marker-entry.cells.begin())/entry.width;
        const auto base=anchored==1?origin:target;
        auto add=[](int a,int b) {const auto bits=std::uint32_t(a)+std::uint32_t(b);std::int32_t value;std::memcpy(&value,&bits,4);return value;};
        for (int x=0;x<entry.width;++x) for (int y=entry.height-1;y>=0;--y) {
            const auto cell=entry.cells[std::size_t(y)*entry.width+x];if (cell!=1 && !(cell==2 && entry.affects_player)) continue;
            const int dx=x-anchor_x,dy=y-anchor_y;int rx=dx,ry=dy;
            if (anchored && facing!=2) {
                if (facing==0) ry=-dy;else if (facing==1) {rx=dy;ry=dx;}else {rx=-dy;ry=dx;}
            }
            result.targets.push_back({add(base.x,rx),add(base.y,ry)});
        }
    }
    random=candidate;return result;
}
SpellEffectDefinition read_spell_effect(const XmlNode& node) {
    if (lower(node.tag)!="effect") throw std::invalid_argument("expected spell effect");
    SpellEffectDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);const auto& value=attribute.second;
        const auto integer=legacy_integer(value);const auto real=[&]{return static_cast<float>(std::strtod(value.c_str(),nullptr));};
        if (key=="type") result.type=value;
        else if (key=="spell") result.spell=value;
        else if (key=="taxa") result.taxa=value;
        else if (key=="bufftag") result.buff_tag=value;
        else if (key=="amount") result.amount=integer;
        else if (key=="amountf") result.amount_scale=real();
        else if (key=="floorscalef") result.floor_scale=real();
        else if (key=="percent" || key=="percentage") result.percent=integer;
        else if (key=="turns") result.turns=integer;
        else if (key=="primaryscale") result.primary_scale=integer;
        else if (key=="secondaryscale") result.secondary_scale=integer;
        else if (key=="self") result.self=integer;
        else if (key=="affectscaster") result.affects_caster=integer;
        else if (key=="unblockable") result.unblockable=integer;
        else if (key=="permanent") result.permanent=integer;
        else if (key=="kickback") result.kickback=integer;
        else if (key=="resistable") result.resistable=integer!=0;
        else if (key=="affectscorpses") result.affects_corpses=integer!=0;
        else if (key=="issleeping") result.is_sleeping=integer!=0;
        else if (key=="midas") result.midas=integer!=0;
        else if (key=="burn") result.burn=integer!=0;
        else if (key=="regengfx") result.regeneration_graphic=integer!=0;
        else for (std::size_t i=0;i<damage_names.size();++i) {
            // Generic property names are case insensitive; the original
            // ConvertEffectType/ConvertEffectTypeF fallback is case sensitive.
            if (attribute.first==damage_names[i]) result.damage[i]=integer;
            else if (attribute.first==std::string(damage_names[i])+"F") result.scaling[i]=real();
        }
    }
    return result;
}
SpellDefinition read_spell(const XmlNode& node) {
    if (lower(node.tag)!="spell") throw std::invalid_argument("expected spell definition");
    SpellDefinition result;result.source=node;
    for (const auto& attribute:node.attributes) {
        const auto key=lower(attribute.first);
        if (key=="name") result.name=attribute.second;else if (key=="type") result.type=attribute.second;
    }
    if (result.name.empty()) throw std::invalid_argument("spell name missing");
    for (const auto& child:node.children) if (lower(child.tag)=="effect") result.effects.push_back(read_spell_effect(child));
    return result;
}
}
