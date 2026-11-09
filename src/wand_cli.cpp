#include "dredmor/inventory.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        int minimum,maximum;std::uint32_t seed;
        while (std::cin>>minimum) {
            if (!(std::cin>>maximum>>seed)) throw std::invalid_argument("invalid wand input");
            dredmor::ItemDefinition definition;definition.kind=dredmor::ItemKind::wand;
            definition.wand_min_charge=minimum;definition.wand_max_charge=maximum;
            dredmor::DarwinRandom random(seed);const auto item=dredmor::make_inventory_item(definition,0,1,random);
            std::cout<<*item.charges<<' '<<random.state()<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
