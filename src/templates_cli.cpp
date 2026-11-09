#include "dredmor/content.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc<2 || argc>3 || (argc==3 && std::string(argv[2])!="--expansions"))
            throw std::invalid_argument("Usage: dredmor-templates DATA_ROOT [--expansions]");
        const auto content=dredmor::GameContent::load(argv[1],argc==3);
        for (const auto& definition:content.rooms()) {
            const auto room=definition.numeric_template();
            std::uint64_t hash=14695981039346656037ull;
            for (auto cell:room.cells) for (auto value:{cell.kind,cell.door_orientation,cell.wall_marker})
                hash=(hash^value)*1099511628211ull;
            const auto& rules=room.rules;
            std::cout<<room.width<<' '<<room.height<<' '<<room.shop<<' '<<room.zoo<<' '
                <<rules.minimum_level<<' '<<rules.maximum_level<<' '<<rules.room_set<<' '
                <<rules.quest<<' '<<rules.unique<<' '<<rules.diggle_god<<' '<<rules.no_traps<<' '
                <<rules.no_monsters<<' '<<rules.no_blockers<<' '<<rules.no_treasure<<' '<<std::hex<<hash<<std::dec<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
