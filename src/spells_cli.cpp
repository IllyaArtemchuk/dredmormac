#include "dredmor/spells.hpp"
#include "dredmor/catalog.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
void output(const dredmor::SpellEffectDefinition& effect) {
    auto bits=[](float value){std::uint32_t result;std::memcpy(&result,&value,4);return result;};
    std::cout<<effect.amount<<' '<<effect.percent<<' '<<effect.turns<<' '<<effect.primary_scale<<' '<<effect.secondary_scale<<' '
        <<effect.self<<' '<<effect.affects_caster<<' '<<effect.unblockable<<' '<<effect.permanent<<' '<<effect.kickback<<' '
        <<bits(effect.amount_scale)<<' '<<bits(effect.floor_scale)<<' '<<effect.resistable<<' '<<effect.affects_corpses<<' '<<effect.is_sleeping<<' '
        <<effect.midas<<' '<<effect.burn<<' '<<effect.regeneration_graphic;
    for (auto n:effect.damage) std::cout<<' '<<n;for (auto n:effect.scaling) std::cout<<' '<<bits(n);
    for (const auto* value:{&effect.spell,&effect.taxa,&effect.buff_tag}) std::cout<<' '<<std::quoted(*value);std::cout<<'\n';
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && (std::string(argv[1])=="--heal" || std::string(argv[1])=="--spellpoints")) {
            std::int32_t current,maximum,amount,attribute;std::uint32_t bits;
            while (std::cin>>current>>maximum>>amount>>bits>>attribute) {
                float scale;std::memcpy(&scale,&bits,4);
                const auto result=std::string(argv[1])=="--heal"?dredmor::resolve_healing(current,maximum,amount,scale,attribute):dredmor::resolve_spellpoints(current,maximum,amount,scale,attribute);
                std::cout<<result<<'\n';
            }
            return std::cin.eof()?0:1;
        }
        if (argc==3 && std::string(argv[1])=="--content") {
            const auto catalog=dredmor::Catalog::load(argv[2],true);
            for (const auto& entry:catalog.entries()) if (entry.node->tag=="spell" && entry.database->source.filename()=="spellDB.xml") {
                const auto spell=dredmor::read_spell(*entry.node);
                for (std::size_t i=0;i<spell.effects.size();++i) {
                    std::cout<<std::quoted(entry.database->layer)<<' '<<std::quoted(spell.name)<<' '<<i<<' ';output(spell.effects[i]);
                }
            }
            return 0;
        }
        if (argc!=1) throw std::invalid_argument("Usage: dredmor-spells [--content DATA_ROOT]");
        std::size_t count;
        while (std::cin>>count) {
            if (count>10000) throw std::invalid_argument("too many effect attributes");dredmor::XmlNode node;node.tag="effect";
            for (std::size_t i=0;i<count;++i) {std::string key,value;if (!(std::cin>>std::quoted(key)>>std::quoted(value))) throw std::invalid_argument("incomplete spell effect");node.attributes.emplace(key,value);}
            output(dredmor::read_spell_effect(node));
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
