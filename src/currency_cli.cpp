#include "dredmor/currency.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

void print(const dredmor::CurrencyObject& object) {
    std::cout<<unsigned(object.kind)<<' '<<object.value<<' '<<object.quantity<<' '<<object.stackable<<' '
        <<std::quoted(object.sprite_key)<<' '<<std::quoted(object.sprite);
}
int main() {
    try {
        std::string command;
        while (std::cin>>command) {
            if (command=="create") {
                std::int32_t amount;int quantity;std::size_t skill_count,ability_count;
                if (!(std::cin>>amount>>quantity>>skill_count) || skill_count>100) throw std::invalid_argument("invalid currency skills");
                std::vector<dredmor::SkillRank> skills(skill_count);for (auto& skill:skills) std::cin>>skill.id>>skill.rank;
                if (!(std::cin>>ability_count) || ability_count>1000) throw std::invalid_argument("invalid currency abilities");
                std::vector<dredmor::AbilityDefinition> abilities(ability_count);
                for (auto& ability:abilities) {std::uint32_t bits;std::cin>>ability.skill_id>>ability.level>>bits;std::memcpy(&ability.currency_bonus,&bits,4);}
                auto object=dredmor::make_currency(amount,skills,abilities);dredmor::currency_quantity_override(object,quantity);print(object);std::cout<<'\n';
            } else if (command=="roll" || command=="named") {
                std::uint32_t seed;std::int32_t amount;int quantity;
                if (!(std::cin>>seed)) throw std::invalid_argument("invalid currency seed");
                dredmor::DarwinRandom random(seed);
                if (command=="roll") {dredmor::SpawnDepthContext depth;std::cin>>depth.depth>>depth.alternate>>depth.alternate_depth;amount=dredmor::roll_currency(depth,random);}
                else {int depth,per_level;std::cin>>depth>>per_level;amount=dredmor::roll_named_currency(depth,per_level,random);}
                std::cin>>quantity;auto object=dredmor::make_currency(amount);dredmor::currency_quantity_override(object,quantity);print(object);std::cout<<' '<<random.state()<<'\n';
            } else if (command=="give" || command=="take") {
                std::int32_t balance,amount;if (!(std::cin>>balance>>amount)) throw std::invalid_argument("invalid currency transfer");
                const auto result=dredmor::transfer_currency(balance,amount,command=="give"?dredmor::CurrencyOperation::give:dredmor::CurrencyOperation::take);
                std::cout<<result.balance<<' '<<result.delta<<' '<<result.money_sound<<'\n';
            } else throw std::invalid_argument("unknown currency command");
            if (!std::cin) throw std::invalid_argument("incomplete currency fixture");
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
