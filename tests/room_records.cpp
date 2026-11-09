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
        attrs={{"PNGSprite","fixture"},{"PNGFirst"," 2tail"},{"PNGNum","3"},{"PNGRate","150"},{"PNGAnimate","0"},{"id","fixture"}};
        auto sprite=take_room_sprite(attrs);
        check(sprite && sprite->series=="fixture" && sprite->first==2 && sprite->count==3 && sprite->rate==150 && attrs.size()==1);
        attrs={{"pngSprite","fixture"},{"pngNum","3"}};auto original_attrs=attrs;
        check(!take_room_sprite(attrs) && attrs==original_attrs);
        attrs={{"png","fixture.spr"},{"pngAnimate","1"}};original_attrs=attrs;rejected=false;
        try {take_room_sprite(attrs);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && attrs==original_attrs);
        room.cells[30].kind=0;
        auto custom=read_custom_room_object(parse_xml("<customblocker at='3' png='fixture.spr' name='Fixture' description='Details' passable='1' id='fixture' can_push='0'/>"),markers,room);
        check(custom.kind==CustomRoomKind::blocker && custom.passable==true && !custom.vis_block && room.cells[30].kind==1);
        random.seed(1);auto object=create_custom_room_object(custom,{10,20},random);
        check(object && object->position.x==12 && object->position.y==24 && !object->blocks_movement && !object->blocks_vision);
        check(object->room_local_id=="fixture" && object->can_push==0 && object->name=="Fixture" && object->description=="Details");
        check(object->sprite.image=="fixture.spr" && object->frame==0 && object->playing && object->looping && random.state()==1);
        custom=read_custom_room_object(parse_xml("<customengraving x='1' y='2' pngSprite='fixture' pngFirst='0' pngNum='3' pngRate='150' pngAnimate='0' passable='0' visblock='1'/>"),markers,room);
        object=create_custom_room_object(custom,{10,20},random);
        check(object && object->kind==CustomRoomKind::engraving && object->blocks_movement && object->blocks_vision && object->playing && object->looping);
        custom.passable.reset();custom.vis_block.reset();object=create_custom_room_object(custom,{10,20},random);
        check(object && !object->blocks_movement && !object->blocks_vision);
        custom.placement.percent=-1;random.seed(1);check(!create_custom_room_object(custom,{0,0},random) && random.state()==16807);
        room.cells[30].kind=0;rejected=false;
        try {read_custom_room_object(parse_xml("<customblocker at='3' pngSprite='fixture'/>"),markers,room);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && room.cells[30].kind==0);
        definition.source=parse_xml("<room width='5' height='1'><customblocker at='1' png='fixture.spr'/><customengraving x='2' y='0' png='fixture.png'/></room>");
        auto customs=definition.custom_objects();check(customs.size()==2 && customs[0].kind==CustomRoomKind::blocker && customs[1].kind==CustomRoomKind::engraving);
        auto population=read_room_population(parse_xml("<loot at='3' amount=' 7tail' artifact='1'/>"),markers,room);
        const auto& loot=std::get<RoomLootRecord>(population);
        check(loot.type.empty() && loot.subtype=="random" && loot.amount==7 && loot.artifact);
        random.seed(1);auto request=plan_room_population(population,{10,20},random);
        check(request.request==PopulationRequest::loot && request.position.x==12 && request.position.y==24 && request.amount==7 && request.artifact && random.state()==1);
        population=read_room_population(parse_xml("<horde x='1' y='2'/>"),markers,room);
        check(std::get<RoomHordeRecord>(population).minimum==-1 && std::get<RoomHordeRecord>(population).additional==-1);
        request=plan_room_population(population,{0,0},random);check(request.request==PopulationRequest::horde && request.name.empty());
        population=read_room_population(parse_xml("<monster x='1' y='0' percent='0'/>"),markers,room);
        random.seed(1);request=plan_room_population(population,{2147483647,0},random);
        check(request.request==PopulationRequest::random_monster && request.position.x==(-2147483647-1) && random.state()==16807);
        std::get<RoomMonsterRecord>(population).placement.percent=-1;random.seed(1);request=plan_room_population(population,{0,0},random);
        check(request.request==PopulationRequest::none && random.state()==16807);
        population=read_room_population(parse_xml("<trap x='0' y='0' name='Fixture'/>"),markers,room);
        request=plan_room_population(population,{0,0},random);check(request.request==PopulationRequest::named_trap && request.name=="Fixture");
        room.cells[30].kind=0;rejected=false;
        try {read_room_population(parse_xml("<monster at='3' x='1'/>"),markers,room);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && room.cells[30].kind==0);
        definition.source=parse_xml("<room width='5' height='1'><monster at='1'/><loot at='2'/><trap at='3'/><horde at='4'/></room>");
        auto records=definition.population();check(records.size()==4 && records[0].index()==0 && records[1].index()==2 && records[2].index()==1 && records[3].index()==3);
        std::cout<<"room point resolution, attribute consumption, marker floors and nested script references passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
