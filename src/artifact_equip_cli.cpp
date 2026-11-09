#include "dredmor/equipment.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
int main() {
    try {
        unsigned kind;
        while (std::cin>>kind) {
            dredmor::ItemDefinition definition;definition.kind=static_cast<dredmor::ItemKind>(kind);definition.armour_type=0;
            int type;std::cin>>type>>definition.armour_ac;definition.weapon_type=type;
            auto read=[](auto& values){for (auto& n:values) std::cin>>n;};
            read(definition.modifiers.primary);read(definition.modifiers.secondary);
            if (kind==1) read(definition.weapon_damage);else read(definition.modifiers.damage);
            read(definition.modifiers.resistance);dredmor::Artifact artifact;
            read(artifact.modifiers.primary);read(artifact.modifiers.secondary);read(artifact.modifiers.damage);
            read(artifact.modifiers.resistance);read(artifact.weapon_damage);
            if (!std::cin) throw std::invalid_argument("incomplete artifact equip fixture");
            dredmor::InventoryItem item{1,definition.kind,0,type,1,false};item.artifact=artifact;dredmor::Equipment equipment;
            const std::vector<dredmor::ItemDefinition> definitions{definition};
            equipment.equip(item,kind==2?dredmor::EquipmentSlot::head:type==4?dredmor::EquipmentSlot::ranged:dredmor::EquipmentSlot::hand_left,definitions);
            dredmor::SecondaryStats flat{};flat[0]=flat[1]=200;
            const auto stats=equipment.rebuild({3,2,2},definitions,{},flat);
            for (auto n:stats.primary) std::cout<<n<<' ';for (auto n:stats.secondary) std::cout<<n<<' ';
            for (std::size_t i=0;i<16;++i) std::cout<<stats.damage_bonus[i]+(kind==1 && type!=4?definition.weapon_damage[i]+artifact.weapon_damage[i]:0)<<' ';
            for (auto n:stats.resistance) std::cout<<n<<' ';
            std::cout<<std::min(100+artifact.modifiers.secondary[0],stats.secondary[0])<<' '
                <<std::min(100+artifact.modifiers.secondary[1],stats.secondary[1])<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
