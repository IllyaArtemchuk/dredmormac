#include "dredmor/session.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace dredmor;
int main() {
    try {
        MonsterStatInput input;
        while (std::cin>>input.ranks[0]) {
            std::cin>>input.ranks[1]>>input.ranks[2];
            for (auto* values:{&input.definition_primary,&input.primary_flat,&input.primary_percent}) for (auto& n:*values) std::cin>>n;
            for (auto* values:{&input.definition_secondary,&input.secondary_flat,&input.secondary_percent}) for (auto& n:*values) std::cin>>n;
            std::cin>>input.dungeon_level>>input.health_scale>>input.health_percent_per_level>>input.health_per_level;
            Actor actor;for (auto& n:actor.damage) std::cin>>n;for (auto& n:actor.resistance) std::cin>>n;
            std::size_t count;std::cin>>count;if (!std::cin || count>10) throw std::invalid_argument("invalid monster buff fixture");
            std::vector<SpellDefinition> definitions;
            for (std::size_t i=0;i<count;++i) {
                SpellDefinition spell;spell.name="Buff "+std::to_string(i);spell.type="self";BuffDefinition buff;
                std::cin>>buff.time>>buff.use_timer>>buff.allow_stacking>>buff.stack_size>>std::quoted(buff.icon)>>buff.mana_upkeep;
                for (auto& n:buff.modifiers.primary) std::cin>>n;for (auto& n:buff.modifiers.secondary) std::cin>>n;
                for (auto& n:buff.modifiers.damage) std::cin>>n;for (auto& n:buff.modifiers.resistance) std::cin>>n;
                spell.buffs.push_back(buff);definitions.push_back(spell);
            }
            std::string operations;if (!(std::cin>>std::quoted(operations))) throw std::invalid_argument("missing monster buff operations");
            const auto profile=std::make_shared<const std::vector<SpellDefinition>>(std::move(definitions));
            const auto stats=derive_monster(input);actor.stat_profile=input;actor.primary=stats.primary;actor.secondary=stats.secondary;actor.health=stats.secondary[0]-20;actor.mana=stats.secondary[1]-10;actor.position={5,2};actor.name="Fixture monster";
            std::istringstream map("#######\n#.@...#\n#.....#\n#######\n");Session session(World::read(map),make_player({20,20,20}),1);session.spell_profile(profile);session.spawn_enemy(actor);
            auto output=[&]() {
                const auto& owner=session.enemies()[0];for (auto n:owner.primary) std::cout<<n<<' ';for (auto n:owner.secondary) std::cout<<n<<' ';
                for (auto n:owner.damage) std::cout<<n<<' ';for (auto n:owner.resistance) std::cout<<n<<' ';
                std::cout<<owner.health<<' '<<owner.mana<<' '<<owner.corpse<<' '<<owner.buffs.size()<<' ';
                for (const auto& buff:owner.buffs) std::cout<<buff.spell<<' '<<buff.remaining<<' '<<buff.upkeep_remaining<<' ';
            };
            output();for (char op:operations) {
                if (op=='t') session.update_buffs(2);
                else if (op=='k') {SpellDefinition kill;kill.name="Kill";kill.type="target";SpellEffectDefinition effect;effect.damage[15]=100000;effect.unblockable=1;kill.effects.push_back(effect);session.cast_spell(kill,session.player().position,{5,2});}
                else if (op=='s') {std::ostringstream out;session.save(out);std::istringstream in(out.str());session=Session::load(in,{},{},profile);}
                else if (op>='0' && op<='9' && std::size_t(op-'0')<profile->size()) session.cast_spell(profile->at(op-'0'),{5,2},{5,2},2);
                else throw std::invalid_argument("unknown monster buff operation");output();
            }
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
