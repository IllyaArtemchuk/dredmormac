#include "dredmor/levers.hpp"
#include "dredmor/dungeon.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        int width,height,mode;SecretLever lever;Position player;bool tutorial,held;
        while (std::cin>>width>>height>>lever.position.x>>lever.position.y>>lever.target.x>>lever.target.y>>lever.activated>>tutorial>>mode>>player.x>>player.y>>held) {
            if (width<1 || width>512 || height<1 || height>512 || mode<0 || mode>3) throw std::invalid_argument("invalid lever fixture");
            std::vector<DungeonCell> cells(std::size_t(width)*height);
            for (auto& c:cells) {unsigned kind,room,door,link;std::cin>>kind>>room>>door>>c.flags>>link;if (!std::cin || kind>255 || room>255 || door>255 || link>255) throw std::invalid_argument("invalid lever cell");c.kind=kind;c.room_id=room;c.door_flags=door;c.stair_link=link;}
            DungeonLayout layout(width,height,std::move(cells));const bool handled=mode==0 || mode==2 || can_click_secret_lever(lever,player,held);SecretLeverActivation result;
            if (handled) {result=flip_secret_lever(lever,tutorial);if (result.revealed) layout.at(lever.target.x,lever.target.y).kind=1;}
            std::cout<<handled<<' '<<lever.activated<<' '<<result.tutorial_event<<' '<<result.messages.size();for (const auto& message:result.messages) std::cout<<' '<<std::quoted(message);
            std::cout<<' '<<std::quoted(result.sound)<<' '<<result.revealed;if (result.revealed) for (auto n:result.graphics_bounds) std::cout<<' '<<n;
            for (const auto& c:layout.cells()) std::cout<<' '<<unsigned(c.kind)<<' '<<unsigned(c.room_id)<<' '<<unsigned(c.door_flags)<<' '<<c.flags<<' '<<unsigned(c.stair_link);
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
