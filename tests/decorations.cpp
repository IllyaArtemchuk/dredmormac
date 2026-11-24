#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("decoration assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& s) {std::ostringstream out;s.save(out);return out.str();}
int main() {
    try {
        const auto xml=parse_xml("<text><noun statue='s.spr' plural='Subjects' floor='f.spr' painting='p.spr'/><noun plural='First' PLURAL='Last' painting='a.spr'/><noun PLURAL='Last' plural='First' floor='b.spr'/><noun plural='Subjects' painting=''/></text>");
        const auto table=read_room_decorations(xml);check(table.size()==5 && table[0].kind==RoomDecorationKind::painting && table[1].kind==RoomDecorationKind::statue && table[2].kind==RoomDecorationKind::floor && table[3].subject=="Last" && table[4].subject=="First");
        RoomStamp room;room.width=2;room.height=3;room.cells.resize(6);room.cells[0].wall_marker=2;room.cells[1].wall_marker=1;room.cells[2].wall_marker=3;room.cells[4].wall_marker=1;
        const auto plan=plan_room_decorations(room,{2,1},"Subjects",table);check(plan.size()==4 && plan[0].position.x==2 && plan[0].position.y==3 && plan[1].position.x==3 && plan[1].position.y==1 && plan[2].sprite=="s.spr" && plan[3].sprite=="f.spr");
        check(plan_room_decorations(room,{2,1},"subjects",table).empty() && plan_room_decorations(room,{2,1},"",table).empty());
        const auto object=make_room_decoration(plan[3]);check(object.kind==CustomRoomKind::engraving && !object.blocks_movement && !object.blocks_vision && !object.playing && object.frame==0);
        std::istringstream rows("#######\n#@....#\n#.....#\n#.....#\n#.....#\n#######\n");Session session(World::read(rows),make_player({3,2,2}),99);
        const auto indices=session.create_room_decorations(room,{2,1},"Subjects",table);check(indices.size()==4 && !session.passable({2,3}) && !session.passable({3,1}) && !session.passable({2,1}) && session.passable({2,2}));
        const auto before=saved(session);std::istringstream input(before);auto restored=Session::load(input);check(saved(restored)==before && !restored.custom_objects()[3].playing);
        bool rejected=false;try {session.create_room_decorations(room,{6,1},"Subjects",table);} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(session)==before);
        rejected=false;try {plan_room_decorations(room,{2,1},"Subjects",table,1);} catch(const std::runtime_error&) {rejected=true;}check(rejected);
        std::cout<<"noun registration/attribute order, marker traversal, stationary constructor metadata, collision and save rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
