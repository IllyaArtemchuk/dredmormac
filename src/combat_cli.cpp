#include "dredmor/combat.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    dredmor::CombatInput input;
    while (std::cin>>input.critical) {
        if (!(std::cin>>input.dodge>>input.counter>>input.block>>input.melee_power>>input.armour)) return 1;
        for (auto& value:input.damage) if (!(std::cin>>value)) return 1;
        for (auto& value:input.resistance) if (!(std::cin>>value)) return 1;
        std::array<std::int32_t,4> tape;
        for (auto& value:tape) if (!(std::cin>>value)) return 1;
        std::size_t cursor=0;
        try {
            auto result=dredmor::resolve_combat(input,[&] { return tape.at(cursor++); });
            std::cout<<static_cast<int>(result.outcome)<<' '<<result.critical<<' '<<result.blocked<<' '
                     <<result.physical<<' '<<result.elemental<<' '<<result.total<<' '<<result.draws<<'\n';
        } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
    }
    return std::cin.eof()?0:1;
}
