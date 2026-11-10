#include "dredmor/spells.hpp"
#include "dredmor/abilities.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/session.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
using namespace dredmor;
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--costs") {
            SpellDefinition::Casting casting;std::int32_t magic,savvy;std::uint32_t magic_bits,savvy_bits,gold_bits;SpawnDepthContext depth;
            while (std::cin>>casting.mana>>casting.minimum_mana>>magic>>savvy>>magic_bits>>savvy_bits>>casting.gold>>gold_bits>>depth.depth>>depth.alternate>>depth.alternate_depth) {
                std::memcpy(&casting.magic_bonus,&magic_bits,4);std::memcpy(&casting.savvy_bonus,&savvy_bits,4);std::memcpy(&casting.gold_scale,&gold_bits,4);
                std::cout<<spell_mana_cost(casting,magic,savvy)<<' '<<spell_gold_cost(casting,savvy,depth)<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--actions") {
            std::uint32_t seed;std::int32_t mana,gold,downtime,multi,percent;bool muted;
            while (std::cin>>seed>>mana>>gold>>downtime>>multi>>percent>>muted) {
                auto spells=std::make_shared<std::vector<SpellDefinition>>();
                auto root=read_spell(parse_xml("<spell name='Root' type='self' noAnimation='1'><effect type='spellpoints' amount='-1'/></spell>"));
                root.casting.mana=mana;root.casting.gold=gold;root.casting.downtime=downtime;root.casting.cooldown=downtime>0;root.casting.multi_attack=multi;
                spells->push_back(root);spells->push_back(read_spell(parse_xml("<spell name='Child' type='self'><effect type='spellpoints' amount='-1'/></spell>")));
                spells->push_back(read_spell(parse_xml("<spell name='Silence' type='self'><buff time='5' icon='mute'><mute/></buff></spell>")));
                auto abilities=std::make_shared<std::vector<AbilityDefinition>>();auto ability=read_ability(parse_xml("<ability name='Fixture' skill='0' level='0'><spell name='Root'/><triggeroncast name='Child'/></ability>"));ability.cast_trigger_percent=percent;abilities->push_back(ability);
                auto items=std::make_shared<const std::vector<ItemDefinition>>();std::istringstream map("#####\n#@..#\n#...#\n#####\n");auto player=make_player({20,20,20});player.zorkmids=20;
                Session session(World::read(map),player,seed);std::vector<SkillRank> skills;for (int i=0;i<7;++i) skills.push_back({i,1});session.character_profile({20,20,20},items,skills,abilities);session.spell_profile(spells);
                if (muted) session.cast_spell(spells->at(2),{1,1},{1,1});
                std::cout<<session.player().mana<<' '<<session.player().secondary[3]<<' '<<session.player().primary[5]<<' ';
                session.use_spell(0,{1,1});std::ostringstream saved;session.save(saved);const auto xml=parse_xml(saved.str());
                std::cout<<session.player().mana<<' '<<session.player().zorkmids<<' '<<session.spell_cooldown(0)<<' '<<session.world().turn()<<' '<<xml.first("random")->attributes.at("libc")<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--potions") {
            std::int32_t quantity,amount;bool linked;
            while (std::cin>>quantity>>amount>>linked) {
                auto items=std::make_shared<std::vector<ItemDefinition>>();items->push_back(read_item(parse_xml("<item name='Fixture'><potion spell='Effect'/></item>")));
                auto spells=std::make_shared<std::vector<SpellDefinition>>();auto effect=read_spell(parse_xml("<spell name='Effect' type='self'><effect type='spellpoints' amount='0'/></spell>"));effect.effects[0].amount=amount;
                if (linked) spells->push_back(effect);
                std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({20,20,20}),1);session.equipment_profile({20,20,20},items);session.spell_profile(spells);
                auto prepare=effect;prepare.effects[0].amount=-10;session.cast_spell(prepare,{1,1},{1,1});
                std::cout<<session.player().mana<<' '<<session.player().secondary[1]<<' ';
                const auto instance=session.create_ground_item(0,{1,1},quantity);session.pickup();const auto used=session.drink_potion(instance);const auto remaining=session.inventory().find(instance);
                std::cout<<used<<' '<<session.player().mana<<' '<<(remaining?remaining->quantity:0)<<' '<<session.world().turn()<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--potion-metadata") {
            std::string xml;while (std::getline(std::cin,xml)) if (!xml.empty()) {const auto item=read_item(parse_xml(xml));std::cout<<int(item.kind)<<' '<<std::quoted(item.cast_spell)<<'\n';}
        } else if (argc==2 && std::string(argv[1])=="--regeneration") {
            RegenerationState state;std::int32_t hp,max_hp,mp,max_mp,hp_bonus,mp_bonus;bool allowed;
            while (std::cin>>state.health_turns>>state.mana_turns>>state.health_clock>>state.mana_clock>>state.food>>state.booze>>state.vampire>>hp>>max_hp>>mp>>max_mp>>hp_bonus>>mp_bonus>>allowed) {
                regenerate(state,hp,max_hp,mp,max_mp,hp_bonus,mp_bonus,allowed);
                std::cout<<hp<<' '<<mp<<' '<<state.health_clock<<' '<<state.mana_clock<<' '<<state.food<<' '<<state.booze<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--metadata") {
            std::string xml;while (std::getline(std::cin,xml)) if (!xml.empty()) {
                const auto casting=read_spell(parse_xml(xml)).casting;
                for (auto value:{casting.mana,casting.minimum_mana,casting.health,casting.level,casting.booze,casting.gold,casting.weapon,casting.shield,casting.multi_attack,casting.downtime}) std::cout<<value<<' ';
                for (float value:{casting.magic_bonus,casting.savvy_bonus,casting.gold_scale}) {std::uint32_t bits;std::memcpy(&bits,&value,4);std::cout<<bits<<' ';}
                std::cout<<casting.cooldown<<' '<<casting.no_animation<<' '<<casting.attack<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--ability") {
            std::string xml;while (std::getline(std::cin,xml)) if (!xml.empty()) {
                const auto ability=read_ability(parse_xml(xml));std::cout<<ability.cast_trigger_percent<<' '<<std::quoted(ability.cast_trigger)<<'\n';
            }
        } else throw std::invalid_argument("Usage: dredmor-casting --costs | --metadata | --ability | --actions | --potions");
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
