#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("content assertion at line "+std::to_string(__LINE__)); } while(false)
int main(int argc,char** argv) {
    try {
        check(argc==2);auto content=dredmor::GameContent::load(argv[1]);
        check(content.items().size()==6 && content.monsters().size()==2 && content.skills().size()==7);
        check(content.items()[4].builtin_subtype==0 && content.items()[5].builtin_subtype==1);
        check(content.abilities().size()==1 && content.abilities()[0].skill_name=="Fixture 0");
        auto world=content.room("Starting Room").instantiate();
        check(world.width()==7 && world.height()==5 && !world.passable({3,2}));
        check(world.player().x==3 && world.player().y==1);
        check(content.starting_ranks({0,1,2,3,4,5,6})==dredmor::ClassRanks({3,2,2}));
        const auto loadout=content.starting_loadout({6,5,4,3,2,1,0});
        check(loadout.size()==2 && loadout[0].skill_id==0 && loadout[0].item.amount==1);
        check(loadout[1].skill_id==1 && loadout[1].item.always && loadout[1].item.subtype=="Fixture");
        auto enemy=content.make_monster("Fixture variant",{2,1},dredmor::Difficulty::medium);
        check(enemy.health==11 && enemy.damage[0]==1 && enemy.experience_reward==10);
        check(content.tweaks().integer(dredmor::Difficulty::medium,"monster additional HP per level")==0);
        bool rejected=false;try {content.starting_ranks({0,0,1,2,3,4,5});} catch (const std::invalid_argument&) {rejected=true;}
        check(rejected);
        dredmor::RoomDefinition geometry;
        geometry.name="Geometry fixture";geometry.source=dredmor::XmlNode{};
        geometry.source=dredmor::parse_xml("<room width='6' height='4'><flags shop='1' zoo='1' minLevel='2' maxLevel='8' roomset='1' special='1'/><flags NoMonsters='1'/></room>");
        geometry.rows={"!P@^Dd","WL GIX","Ssi123",".     ignored outside declared width"};
        auto numeric=geometry.numeric_template();
        check(numeric.width==6 && numeric.height==4 && numeric.shop && numeric.zoo);
        check(numeric.rules.minimum_level==2 && numeric.rules.maximum_level==8 && numeric.rules.room_set==1 && numeric.rules.unique && numeric.rules.no_monsters);
        check(numeric.cells[0].kind==2 && numeric.cells[0].wall_marker==4);
        check(numeric.cells[1].wall_marker==1 && numeric.cells[2].wall_marker==2 && numeric.cells[3].wall_marker==3);
        check(numeric.cells[4].kind==6 && numeric.cells[4].door_orientation==0 && numeric.cells[5].door_orientation==1);
        check(numeric.cells[6].kind==7 && numeric.cells[7].kind==8 && numeric.cells[8].kind==0);
        check(numeric.cells[9].kind==9 && numeric.cells[10].kind==10 && numeric.cells[11].kind==15);
        check(numeric.cells[12].kind==1 && numeric.cells[13].kind==1 && numeric.cells[14].kind==1 && numeric.cells[15].kind==0);
        geometry.source.children.clear();geometry.source.attributes["shop"]="1";
        numeric=geometry.numeric_template();check(!numeric.shop && numeric.rules.maximum_level==25);
        std::cout<<"content-to-session definitions, selected classes, variant stats and room collision passed\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
