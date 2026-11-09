#include "dredmor/abilities.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
template<class T> void read(T& value) {if (!(std::cin>>value)) throw std::invalid_argument("invalid passive fixture input");}
template<class T,std::size_t N> void read(std::array<T,N>& array) {for (auto& value:array) read(value);}
}
int main() {
    try {
        dredmor::PassiveStatInput base;
        while (std::cin>>base.ranks[0]) {
            read(base.ranks[1]);read(base.ranks[2]);read(base.primary_flat);read(base.primary_percent);
            read(base.secondary_flat);read(base.secondary_percent);
            std::vector<dredmor::SkillRank> skills(7);
            for (auto& skill:skills) {read(skill.id);read(skill.rank);}
            std::size_t count;read(count);if (count>512) throw std::invalid_argument("too many passive abilities");
            std::vector<dredmor::AbilityDefinition> abilities(count);
            for (auto& ability:abilities) {
                read(ability.skill_id);read(ability.level);
                if (!(std::cin>>std::quoted(ability.skill_name)>>ability.polymorphed_only>>std::quoted(ability.polymorph_tag)))
                    throw std::invalid_argument("invalid passive ability metadata");
                read(ability.modifiers.primary);read(ability.modifiers.secondary);
                read(ability.modifiers.damage);read(ability.modifiers.resistance);
            }
            const auto result=dredmor::derive_passives(base,skills,abilities);
            for (auto value:result.primary) std::cout<<value<<' ';
            for (auto value:result.secondary) std::cout<<value<<' ';
            for (auto value:result.damage) std::cout<<value<<' ';
            for (auto value:result.resistance) std::cout<<value<<' ';
            std::cout<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
