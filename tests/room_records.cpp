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
        room.cells[30].kind=0;
        auto element=read_room_element(parse_xml("<ELEMENT AT='3' TYPE='bookshelf' CONTENTS='Fixture Bolt' CAN_PUSH='258' ID='shelf' percent=' 0tail'/>"),markers,room);
        check(element.placement.relative.x==2 && element.placement.relative.y==4 && element.placement.percent==0);
        check(element.placement.id=="shelf" && element.placement.can_push==2 && room.cells[30].kind==1);
        DarwinRandom random(1);auto plan=plan_room_element(element,{10,20},random);
        check(plan.accepted && plan.kind==RoomElementKind::bookshelf && plan.position.x==12 && plan.position.y==24 && plan.contents=="Fixture Bolt");
        check(random.state()==16807); // zero percent still succeeds on bucket zero
        element.placement.percent=-1;random.seed(1);plan=plan_room_element(element,{10,20},random);
        check(!plan.accepted && plan.kind==RoomElementKind::none && random.state()==16807);
        element.placement.percent=100;random.seed(1);element.type="craftvending";
        plan=plan_room_element(element,{10,20},random);check(plan.accepted && plan.kind==RoomElementKind::dispenser && plan.variant==4 && random.state()==1);
        element.type="FoodVending";plan=plan_room_element(element,{10,20},random);check(plan.accepted && plan.kind==RoomElementKind::none);
        room.cells[30].kind=0;bool rejected=false;
        try {read_room_element(parse_xml("<element at='3'/>"),markers,room);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && room.cells[30].kind==0);
        definition.source=parse_xml("<room width='5' height='1'><element at='1' type='lever'/><element x='2' y='0' type='foodvending'/></room>");
        auto elements=definition.elements();check(elements.size()==2 && elements[0].placement.relative.x==0 && elements[1].type=="foodvending");
        std::cout<<"room point resolution, attribute consumption, marker floors and nested script references passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
