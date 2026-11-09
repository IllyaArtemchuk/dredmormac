#include "dredmor/room_records.hpp"
#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("room record assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    try {
        using namespace dredmor;
        RoomStamp room{7,7,std::vector<TemplateCell>(49,{2,3,4})};RoomMarkers markers{{'3',{2,4}}};
        RoomAttributes attrs{{"TargetAT","3"},{"id","fixture"}};
        auto point=take_room_point(attrs,"target",markers,room);
        check(point && point->x==2 && point->y==4 && attrs.size()==1 && attrs.count("id"));
        check(room.cells[30].kind==1 && room.cells[30].door_orientation==3 && room.cells[30].wall_marker==4);
        check(markers.size()==1); // repeated references remain valid
        attrs={{"at","3"}};point=take_room_point(attrs,"",markers,room);check(point && attrs.empty());
        attrs={{"X","  -2tail"},{"y","invalid"},{"id","fixture"}};
        point=take_room_point(attrs,"",markers,room);
        check(point && point->x==-2 && point->y==0 && attrs.size()==1 && room.cells[0].kind==2);
        attrs={{"x","2"}};point=take_room_point(attrs,"",markers,room);check(!point && attrs.size()==1);
        for (const auto& invalid:std::vector<RoomAttributes>{{{"at","3"},{"x","2"}},{{"at","33"}},{{"at","9"}}}) {
            attrs=invalid;bool rejected=false;
            try {take_room_point(attrs,"",markers,room);} catch(const std::invalid_argument&) {rejected=true;}
            check(rejected && attrs==invalid && room.cells[0].kind==2);
        }
        RoomDefinition definition;definition.name="Point fixture";definition.rows={"12344"};
        definition.source=parse_xml("<room width='5' height='1'><customblocker at='1'/><script><condition condition_type='at' at='2'/><action Action_type='spell' TargetAT='3'/><action action_type='move' at='4'/></script></room>");
        auto numeric=definition.numeric_template();
        check(numeric.cells[0].kind==1 && numeric.cells[1].kind==1 && numeric.cells[2].kind==1);
        check(numeric.cells[3].kind==0 && numeric.cells[4].kind==1); // last duplicate marker wins
        definition.source.children.clear();numeric=definition.numeric_template();
        for (const auto& cell:numeric.cells) check(cell.kind==0);
        definition.source.children.push_back(parse_xml("<element x='0' y='0' type='lever'/>"));
        numeric=definition.numeric_template();for (const auto& cell:numeric.cells) check(cell.kind==0);
        std::cout<<"room point resolution, attribute consumption, marker floors and nested script references passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
