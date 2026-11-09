#include "dredmor/combat.hpp"
#include "dredmor/random.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::SpellDamageInput input;unsigned secondary;
            if (!(std::cin>>input.haywire>>input.resist>>input.allow_haywire>>input.force_unresisted>>input.armour>>input.scaling_attribute>>secondary)) return 1;
            for (auto& value:input.damage) if (!(std::cin>>value)) return 1;
            for (auto& value:input.scaling) {std::uint32_t bits;if (!(std::cin>>bits)) return 1;std::memcpy(&value,&bits,4);}
            for (auto& value:input.resistance) if (!(std::cin>>value)) return 1;
            dredmor::DarwinRandom random(seed);const auto result=dredmor::resolve_spell_damage(input,[&]{return random.next();});
            std::cout<<result.haywire<<' '<<result.resisted<<' '<<result.physical<<' '<<result.elemental<<' '<<result.total<<' '<<result.draws<<' '<<random.state()<<'\n';
        }
        return std::cin.eof()?0:1;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
