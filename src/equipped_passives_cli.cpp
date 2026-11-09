#include "dredmor/equipment.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
template<class T> void read(T& value) {if (!(std::cin>>value)) throw std::invalid_argument("invalid equipped passive input");}
template<class T,std::size_t N> void read(std::array<T,N>& array) {for (auto& value:array) read(value);}
}
int main() {
    try {
        int weapon_type;
        while (std::cin>>weapon_type) {
            bool include_bow;read(include_bow);
            dredmor::ClassRanks ranks;read(ranks);
            dredmor::PrimaryStats pflat;dredmor::SecondaryStats sflat;read(pflat);read(sflat);
            dredmor::ItemDefinition definition;definition.kind=dredmor::ItemKind::weapon;definition.weapon_type=weapon_type;
            read(definition.modifiers.primary);read(definition.modifiers.secondary);
            read(definition.modifiers.damage);read(definition.modifiers.resistance);
            dredmor::ItemDefinition bow;bow.kind=dredmor::ItemKind::weapon;bow.weapon_type=4;
            if (include_bow) {
                read(bow.modifiers.primary);read(bow.modifiers.secondary);
                read(bow.modifiers.damage);read(bow.modifiers.resistance);
            }
            std::vector<dredmor::SkillRank> skills(7);for (auto& skill:skills) {read(skill.id);read(skill.rank);}
            std::size_t count;read(count);if (count>512) throw std::invalid_argument("too many abilities");
            std::vector<dredmor::AbilityDefinition> abilities(count);
            for (auto& ability:abilities) {
                read(ability.skill_id);read(ability.level);
                if (!(std::cin>>std::quoted(ability.skill_name)>>ability.polymorphed_only>>std::quoted(ability.polymorph_tag)))
                    throw std::invalid_argument("invalid equipped ability metadata");
                read(ability.modifiers.primary);read(ability.modifiers.secondary);
                read(ability.modifiers.damage);read(ability.modifiers.resistance);
            }
            dredmor::Equipment equipment;
            auto displaced=equipment.equip({1,dredmor::ItemKind::weapon,0,weapon_type,1,false},
                weapon_type==4?dredmor::EquipmentSlot::ranged:dredmor::EquipmentSlot::hand_left,{definition});
            if (displaced) throw std::logic_error("unexpected equipment displacement");
            std::vector<dredmor::ItemDefinition> definitions={definition};
            if (include_bow) {
                definitions.push_back(bow);
                displaced=equipment.equip({2,dredmor::ItemKind::weapon,1,4,1,false},dredmor::EquipmentSlot::ranged,definitions);
                if (displaced) throw std::logic_error("unexpected bow displacement");
            }
            const auto result=equipment.rebuild(ranks,definitions,pflat,sflat,skills,abilities);
            for (auto value:result.primary) std::cout<<value<<' ';
            for (auto value:result.secondary) std::cout<<value<<' ';
            for (auto value:result.damage_bonus) std::cout<<value<<' ';
            for (auto value:result.resistance) std::cout<<value<<' ';
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
