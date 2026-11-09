#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        std::size_t count;
        while (std::cin>>count) {
            if (count>10) throw std::invalid_argument("too many buff definitions");std::vector<SpellDefinition> profile;
            for (std::size_t i=0;i<count;++i) {
                SpellDefinition spell;spell.name="Buff "+std::to_string(i);spell.type="self";BuffDefinition buff;
                if (!(std::cin>>buff.time>>buff.use_timer>>buff.allow_stacking>>buff.stack_size>>std::quoted(buff.icon)>>buff.mana_upkeep)) throw std::invalid_argument("incomplete buff definition");
                for (auto& n:buff.modifiers.primary) std::cin>>n;for (auto& n:buff.modifiers.secondary) std::cin>>n;
                for (auto& n:buff.modifiers.damage) std::cin>>n;for (auto& n:buff.modifiers.resistance) std::cin>>n;
                if (!std::cin) throw std::invalid_argument("incomplete buff modifiers");spell.buffs.push_back(buff);profile.push_back(spell);
            }
            std::string operations;if (!(std::cin>>std::quoted(operations))) throw std::invalid_argument("missing buff operations");
            auto spells=std::make_shared<const std::vector<SpellDefinition>>(std::move(profile));auto items=std::make_shared<const std::vector<ItemDefinition>>();
            auto player=make_player({20,20,20});player.health-=20;player.mana-=10;
            std::istringstream map("#####\n#.@.#\n#####\n");Session session(World::read(map),player,1);session.equipment_profile({20,20,20},items);session.spell_profile(spells);
            auto output=[&]() {
                const auto& actor=session.player();for (auto n:actor.primary) std::cout<<n<<' ';for (auto n:actor.secondary) std::cout<<n<<' ';
                for (auto n:actor.damage) std::cout<<n<<' ';for (auto n:actor.resistance) std::cout<<n<<' ';
                std::cout<<actor.health<<' '<<actor.mana<<' '<<session.player_buffs().size()<<' ';
                for (const auto& buff:session.player_buffs()) std::cout<<buff.spell<<' '<<buff.remaining<<' '<<buff.upkeep_remaining<<' ';
            };
            output();for (char op:operations) {
                if (op=='t') session.update_player_buffs();
                else if (op=='m') {SpellDefinition drain;drain.name="Drain";drain.type="self";SpellEffectDefinition effect;effect.type="spellpoints";effect.amount=-10000;drain.effects.push_back(effect);session.cast_spell(drain,session.player().position,session.player().position);}
                else if (op=='s') {std::ostringstream out;session.save(out);std::istringstream in(out.str());session=Session::load(in,items,{},spells);}
                else if (op>='0' && op<='9' && std::size_t(op-'0')<spells->size()) session.cast_spell(spells->at(op-'0'),session.player().position,session.player().position);
                else throw std::invalid_argument("unknown buff operation");output();
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
