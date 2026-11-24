#include "dredmor/game_tick.hpp"
#include "dredmor/xml.hpp"
#include <charconv>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace dredmor {
namespace {
constexpr std::size_t limit=100000,cell_limit=1000000,text_limit=1024*1024;
void require(bool valid) {if (!valid) throw std::runtime_error("invalid game tick snapshot");}
bool whitespace(std::string_view text) {return text.find_first_not_of(" \t\r\n")==std::string::npos;}
void container(const XmlNode& node,std::string_view tag,std::size_t attributes,std::size_t children) {
    require(node.tag==tag && node.attributes.size()==attributes && node.children.size()==children && whitespace(node.text));
}
template<class T> T number(std::string_view text) {
    T value{};const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    require(parsed.ec==std::errc{} && parsed.ptr==text.data()+text.size());return value;
}
template<class T> T attribute(const XmlNode& node,std::string_view key) {return number<T>(node.attribute(key));}
bool flag(const XmlNode& node,std::string_view key) {const auto value=attribute<unsigned>(node,key);require(value<=1);return value!=0;}
template<class T> std::vector<T> numbers(const XmlNode& node,std::string_view tag) {
    require(node.tag==tag && node.attributes.empty() && node.children.empty());
    std::vector<T> values;std::istringstream in(node.text);std::string token;
    while (in>>token) {require(values.size()<limit);values.push_back(number<T>(token));}return values;
}
std::string escaped(std::string_view text) {
    require(text.size()<=text_limit);std::string out;
    for (unsigned char c:text) switch (c) {
        case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;
        case '\r':out+="&#13;";break;
        default:require(c>=32 || c=='\n' || c=='\t');out+=char(c);
    }
    return out;
}
void validate(const GameTickState& state) {
    require(state.effect_durations.size()<=limit && state.colour_strings.size()<=limit && state.spell_cooldowns.size()<=limit && state.teleports.size()<=limit);
    require(state.game_message.size()<=text_limit);std::size_t text_bytes=state.game_message.size();
    for (const auto& text:state.colour_strings) {require(text.size()<=text_limit && text.size()<=16*text_limit-text_bytes);text_bytes+=text.size();}
    if (!state.level) return;
    const auto& level=*state.level;require(level.width>=0 && level.height>=0);
    const auto cells=std::uint64_t(level.width)*std::uint64_t(level.height);
    require(cells<=cell_limit && cells==level.tile_effects.size() && level.objects.size()<=limit);
    std::size_t memberships=0;
    for (const auto& tile:level.tile_effects) {
        require(tile.size()<=limit-memberships);memberships+=tile.size();
        for (auto id:tile) require(state.effect_durations.count(id)!=0);
    }
}
template<class T> void write_numbers(std::ostream& out,std::string_view tag,const std::vector<T>& values) {
    out<<'<'<<tag<<'>';for (auto value:values) out<<value<<' ';out<<"</"<<tag<<">\n";
}
}
void write_game_tick_state(std::ostream& output,const GameTickState& state) {
    validate(state);std::ostringstream out;
    out<<"<game-tick version=\"1\" announced=\""<<state.announced_full_inventory<<"\" slow=\""<<state.slow_time_counter
        <<"\" tutorial=\""<<state.tutorial<<"\" last-autosave=\""<<state.last_autosave<<"\">\n";
    out<<"<message>"<<escaped(state.game_message)<<"</message>\n";
    out<<"<player x=\""<<state.player.position.x<<"\" y=\""<<state.player.position.y<<"\" dead=\""<<state.player.dead
        <<"\" respawn=\""<<state.player.respawn_counter<<"\" delay=\""<<state.player.action_delay<<"\"/>\n";
    if (state.level) {
        const auto& level=*state.level;out<<"<level width=\""<<level.width<<"\" height=\""<<level.height<<"\" updates-player=\""<<level.updates_player<<"\">\n";
        write_numbers(out,"objects",level.objects);out<<"<tiles>\n";
        for (const auto& tile:level.tile_effects) write_numbers(out,"tile",tile);
        out<<"</tiles>\n</level>\n";
    } else out<<"<level/>\n";
    out<<"<effects>\n";for (const auto& [id,duration]:state.effect_durations) out<<"<effect id=\""<<id<<"\" duration=\""<<duration<<"\"/>\n";
    out<<"</effects>\n<colours>\n";for (const auto& text:state.colour_strings) out<<"<text>"<<escaped(text)<<"</text>\n";
    out<<"</colours>\n";write_numbers(out,"cooldowns",state.spell_cooldowns);out<<"<teleports>\n";
    for (const auto& move:state.teleports) out<<"<move object=\""<<move.object<<"\" level=\""<<move.level<<"\" x=\""<<move.position.x<<"\" y=\""<<move.position.y<<"\"/>\n";
    out<<"</teleports>\n</game-tick>\n";
    // Validate XML encoding before exposing any bytes to the owner. In
    // particular, arbitrary invalid UTF-8 cannot produce an unusable snapshot.
    const auto bytes=out.str();require(bytes.size()<=64*1024*1024);parse_xml(bytes);output<<bytes;
    if (!output) throw std::runtime_error("cannot write game tick snapshot");
}
GameTickState read_game_tick_state(const XmlNode& root) {
    container(root,"game-tick",5,7);require(root.attribute("version")=="1");GameTickState state;
    state.announced_full_inventory=flag(root,"announced");state.slow_time_counter=attribute<std::int32_t>(root,"slow");
    state.tutorial=attribute<std::int32_t>(root,"tutorial");state.last_autosave=attribute<std::uint32_t>(root,"last-autosave");
    const auto& message=root.children[0];require(message.tag=="message" && message.attributes.empty() && message.children.empty() && message.text.size()<=text_limit);state.game_message=message.text;
    const auto& player=root.children[1];container(player,"player",5,0);
    state.player.position={attribute<int>(player,"x"),attribute<int>(player,"y")};state.player.dead=attribute<std::int32_t>(player,"dead");
    state.player.respawn_counter=attribute<std::int32_t>(player,"respawn");state.player.action_delay=attribute<std::int32_t>(player,"delay");
    const auto& level=root.children[2];
    if (level.attributes.empty()) container(level,"level",0,0);
    else {
        container(level,"level",3,2);TickLevelState data;data.width=attribute<int>(level,"width");data.height=attribute<int>(level,"height");data.updates_player=flag(level,"updates-player");
        data.objects=numbers<std::uint64_t>(level.children[0],"objects");const auto& tiles=level.children[1];
        require(tiles.tag=="tiles" && tiles.attributes.empty() && whitespace(tiles.text) && tiles.children.size()<=cell_limit);
        std::size_t memberships=0;for (const auto& tile:tiles.children) {
            auto values=numbers<std::uint64_t>(tile,"tile");require(values.size()<=limit-memberships);memberships+=values.size();data.tile_effects.push_back(std::move(values));
        }
        state.level=std::move(data);
    }
    const auto& effects=root.children[3];require(effects.children.size()<=limit);container(effects,"effects",0,effects.children.size());
    for (const auto& node:effects.children) {container(node,"effect",2,0);require(state.effect_durations.emplace(attribute<std::uint64_t>(node,"id"),attribute<std::int32_t>(node,"duration")).second);}
    const auto& colours=root.children[4];require(colours.children.size()<=limit);container(colours,"colours",0,colours.children.size());
    for (const auto& node:colours.children) {require(node.tag=="text" && node.attributes.empty() && node.children.empty() && node.text.size()<=text_limit);state.colour_strings.push_back(node.text);}
    state.spell_cooldowns=numbers<std::int32_t>(root.children[5],"cooldowns");
    const auto& moves=root.children[6];require(moves.children.size()<=limit);container(moves,"teleports",0,moves.children.size());
    for (const auto& node:moves.children) {container(node,"move",4,0);state.teleports.push_back({attribute<std::uint64_t>(node,"object"),attribute<std::uint64_t>(node,"level"),{attribute<int>(node,"x"),attribute<int>(node,"y")}});}
    validate(state);return state;
}
}
