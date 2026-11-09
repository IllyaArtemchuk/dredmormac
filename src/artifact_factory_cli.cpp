#include "dredmor/factory.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
std::string hex(const std::string& value) {
    if (value.empty()) return "-";std::string result;const char* digits="0123456789abcdef";
    for (unsigned char c:value) {result+=digits[c>>4];result+=digits[c&15];}return result;
}
int main() {
    try {
        using namespace dredmor;
        std::uint32_t seed;
        while (std::cin>>seed) {
            ItemFactoryContext context;int level_depth;std::string type,subtype;int quantity;bool make_artifact;std::size_t count;
            std::cin>>context.depth.depth>>context.depth.alternate>>context.depth.alternate_depth>>level_depth>>context.level_set
                >>context.trap_deviation>>std::quoted(type)>>std::quoted(subtype)>>quantity>>make_artifact>>count;
            if (count>10000) throw std::invalid_argument("too many artifact factory definitions");
            context.level_depth=level_depth;std::vector<ItemDefinition> definitions(count);ItemFactoryTables tables;
            for (std::size_t i=0;i<count;++i) {
                auto& item=definitions[i];unsigned kind;int craft,artifact,quality;std::uint32_t mask;
                std::cin>>std::quoted(item.name)>>kind>>item.weapon_type>>item.level>>item.special>>craft>>artifact>>quality>>mask;
                item.kind=static_cast<ItemKind>(kind);item.description="Description "+std::to_string(i);
                item.source.attributes["craftoutput"]=std::to_string(craft);item.source.attributes["artifact"]=std::to_string(artifact);
                XmlNode node;node.tag="artifact";node.attributes["quality"]=std::to_string(quality);item.source.children.push_back(std::move(node));
                register_factory_item(tables,static_cast<std::uint32_t>(i),item,mask);
            }
            ItemFactoryState state;std::cin>>count;if (count>10000) throw std::invalid_argument("too many artifact factory flags");
            for (std::size_t i=0;i<count;++i) {std::uint32_t index;std::cin>>index;state.spawned_artifacts.insert(index);}
            ArtifactWords words;std::size_t pool=0;
            for (auto* entries:{&words.starts,&words.phonemes,&words.titles,&words.nouns,&words.plurals,&words.adjectives,&words.qualities,&words.materials,&words.decor}) {
                for (int i=0;i<3;++i) entries->push_back("W"+std::to_string(pool)+"_"+std::to_string(i));++pool;
            }
            if (!std::cin) throw std::invalid_argument("incomplete artifact factory fixture");
            context.artifact_words=&words;GameRandom random;random.seed(seed);
            const auto item=make_loot_item(definitions,tables,context,type,subtype,quantity,make_artifact,1,random,{},{},&state);
            if (!item) throw std::runtime_error("artifact fixture unexpectedly created no item");
            std::cout<<item->definition<<' '<<unsigned(item->kind)<<' '<<item->quantity<<' '<<item->stackable<<' '<<item->artifact.has_value();
            if (item->artifact) {
                const auto& artifact=*item->artifact;std::cout<<' '<<hex(artifact.name)<<' '<<hex(artifact.description)<<' '<<hex(artifact.buff_description)<<' '<<artifact.quality;
                for (auto n:artifact.modifiers.primary) std::cout<<' '<<n;for (auto n:artifact.modifiers.secondary) std::cout<<' '<<n;
                for (auto n:artifact.modifiers.damage) std::cout<<' '<<n;for (auto n:artifact.modifiers.resistance) std::cout<<' '<<n;
                for (auto n:artifact.weapon_damage) std::cout<<' '<<n;
            }
            std::cout<<' '<<state.spawned_artifacts.size();for (auto index:state.spawned_artifacts) std::cout<<' '<<index;
            const auto cache=random.normal.state();std::uint32_t bits;std::memcpy(&bits,&cache.spare,4);
            std::cout<<' '<<random.libc.state()<<' '<<cache.available<<' '<<bits;for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
