#include "dredmor/buffs.hpp"
#include "dredmor/catalog.hpp"
#include "dredmor/selection.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
void output(const BuffDefinition& buff) {
    std::uint32_t scale;std::memcpy(&scale,&buff.zorkmid_scale,4);
    std::cout<<buff.time<<' '<<buff.stack_size<<' '<<buff.apply_to_player<<' '<<buff.destroy_on_move<<' '<<buff.brittle<<' '<<buff.attacks<<' '<<buff.mana_upkeep
        <<' '<<buff.zorkmid_upkeep<<' '<<buff.requires_weapon<<' '<<buff.requires_shield<<' '<<scale<<' '<<buff.use_timer<<' '<<buff.allow_stacking
        <<' '<<buff.self<<' '<<buff.bad<<' '<<buff.removable<<' '<<buff.diggle_god<<' '<<buff.invisible<<' '<<buff.mute;
    for (auto n:buff.modifiers.primary) std::cout<<' '<<n;for (auto n:buff.modifiers.secondary) std::cout<<' '<<n;
    for (auto n:buff.modifiers.damage) std::cout<<' '<<n;for (auto n:buff.modifiers.resistance) std::cout<<' '<<n;
    for (const auto* value:{&buff.tag,&buff.description,&buff.gameplay_text,&buff.icon,&buff.small_icon,&buff.insufficient_funds}) std::cout<<' '<<std::quoted(*value);
    std::cout<<'\n';
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--gold-upkeep") {
            std::int32_t base;std::uint32_t bits;SpawnDepthContext depth;
            while (std::cin>>base>>bits>>depth.depth>>depth.alternate>>depth.alternate_depth) {
                float scale;std::memcpy(&scale,&bits,4);std::cout<<buff_gold_upkeep(base,scale,depth)<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--order") {
            std::size_t count;int depth;while (std::cin>>count>>depth) {
                if (count>4096) throw std::invalid_argument("buff order capacity exceeded");std::vector<std::string> names(count);
                for (auto& name:names) if (!(std::cin>>std::quoted(name))) throw std::invalid_argument("missing buff sort name");
                for (auto index:buff_order(names,depth)) std::cout<<index<<' ';std::cout<<'\n';
            }
        } else if (argc==2 && std::string(argv[1])=="--tick") {
            BuffClock clock;while (std::cin>>clock.remaining>>clock.permanent>>clock.use_timer>>clock.pending) {
                const auto tick=tick_buff(clock);std::cout<<tick.clock.remaining<<' '<<tick.deapply<<' '<<tick.remove<<'\n';
            }
        } else if (argc==3 && std::string(argv[1])=="--content") {
            const auto catalog=Catalog::load(argv[2],true);
            for (const auto& entry:catalog.entries()) if (entry.node->tag=="spell" && entry.database->source.filename()=="spellDB.xml") {
                std::size_t index=0;for (const auto& child:entry.node->children) if (child.tag=="buff") {
                    std::cout<<std::quoted(entry.database->layer)<<' '<<std::quoted(entry.node->attribute("name"))<<' '<<index++<<' ';output(read_buff(child));
                }
            }
        } else if (argc==1) {
            std::string xml;while (std::getline(std::cin,xml)) if (!xml.empty()) output(read_buff(parse_xml(xml)));
        } else throw std::invalid_argument("Usage: dredmor-buffs [--tick | --order | --gold-upkeep | --content DATA_ROOT]");
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
