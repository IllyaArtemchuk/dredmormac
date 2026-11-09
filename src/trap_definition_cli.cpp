#include "dredmor/items.hpp"
#include "dredmor/catalog.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
void print(const dredmor::ItemDefinition& item) {
    const auto& trap=*item.trap;
    std::cout<<unsigned(item.kind)<<' '<<item.level<<' '<<unsigned(trap.trigger)<<' '<<unsigned(trap.mount)<<' '<<unsigned(trap.facing)<<' '
        <<trap.charges<<' '<<trap.target_is_caster<<' '<<std::quoted(trap.spell)<<' '<<std::quoted(trap.origin)<<'\n';
}
int main(int argc,char** argv) {
    try {
        if (argc>1) {
            if (argc!=3 || std::string(argv[1])!="--content") throw std::invalid_argument("Usage: dredmor-trap-definition [--content DATA_ROOT]");
            const auto catalog=dredmor::Catalog::load(argv[2],true);
            for (const auto& entry:catalog.entries()) if (entry.node->tag=="item" && entry.database->source.filename()=="itemDB.xml") {
                const auto item=dredmor::read_item(*entry.node);
                if (item.kind==dredmor::ItemKind::trap) {std::cout<<std::quoted(entry.database->layer)<<' '<<std::quoted(item.name)<<' ';print(item);}
            }
            return 0;
        }
        int root_level;std::size_t count;
        while (std::cin>>root_level>>count) {
            if (!count || count>100) throw std::invalid_argument("invalid trap node count");
            dredmor::XmlNode root;root.tag="item";root.attributes={{"name","Fixture"},{"level",std::to_string(root_level)}};
            for (std::size_t i=0;i<count;++i) {
                dredmor::XmlNode node;node.tag="trap";std::size_t attributes;
                if (!(std::cin>>attributes) || attributes>100) throw std::invalid_argument("invalid trap attributes");
                for (std::size_t j=0;j<attributes;++j) {std::string key,value;std::cin>>std::quoted(key)>>std::quoted(value);node.attributes.emplace(key,value);}
                root.children.push_back(std::move(node));
            }
            print(dredmor::read_item(root));
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
