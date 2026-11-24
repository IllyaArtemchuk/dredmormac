#include "dredmor/factory.hpp"
#include "dredmor/secret_doors.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace dredmor;
std::string hex(const std::string& value) {
    if (value.empty()) return "-";std::string result;const char* digits="0123456789abcdef";
    for (unsigned char c:value) {result+=digits[c>>4];result+=digits[c&15];}return result;
}
void emit_random(GameRandom& random) {
    const auto cache=random.normal.state();std::uint32_t bits;std::memcpy(&bits,&cache.spare,4);
    std::cout<<' '<<random.libc.state()<<' '<<cache.available<<' '<<bits;
    const auto mt=random.mt.state();std::cout<<' '<<mt.index;for (auto n:mt.words) std::cout<<' '<<n;
    for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
}
int main(int argc,char** argv) {
    try {
        const bool secret_mode=argc==2 && std::string(argv[1])=="--secret-floor";
        if (argc!=1 && !secret_mode) throw std::invalid_argument("expected --secret-floor or no arguments");
        std::uint32_t seed;ItemFactoryContext context;int level_depth;bool artifact;TreasureOptions options;std::size_t count;
        while (std::cin>>seed>>context.depth.depth>>context.depth.alternate>>context.depth.alternate_depth>>level_depth>>context.level_set>>context.trap_deviation>>artifact>>options.roll_size) {
            context.level_depth=level_depth;for (auto& range:options.ranges) std::cin>>range.minimum>>range.maximum;
            std::cin>>count;if (count>10000) throw std::invalid_argument("too many treasure definitions");
            std::vector<ItemDefinition> definitions(count);ItemFactoryTables tables;
            for (std::size_t i=0;i<count;++i) {
                auto& item=definitions[i];unsigned kind,mask;int craft,predefined,quality,maximum;
                std::cin>>std::quoted(item.name)>>kind>>item.weapon_type>>item.armour_type>>item.level>>item.special>>item.debug>>craft>>predefined>>quality>>mask>>maximum>>item.food_mp;
                item.kind=ItemKind(kind);item.description="Description "+std::to_string(i);if (item.kind==ItemKind::misc) item.builtin_subtype=0;
                item.wand_min_charge=item.wand_max_charge=1;
                item.source.attributes["craftoutput"]=std::to_string(craft);item.source.attributes["artifact"]=std::to_string(predefined);item.source.attributes["maxstack"]=std::to_string(maximum);
                XmlNode node;node.tag="artifact";node.attributes["quality"]=std::to_string(quality);item.source.children.push_back(std::move(node));
                register_factory_item(tables,std::uint32_t(i),item,mask);
            }
            if (!std::cin) throw std::invalid_argument("incomplete treasure fixture");
            ArtifactWords words;std::size_t pool=0;
            for (auto* entries:{&words.starts,&words.phonemes,&words.titles,&words.nouns,&words.plurals,&words.adjectives,&words.qualities,&words.materials,&words.decor}) {
                for (int i=0;i<3;++i) entries->push_back("W"+std::to_string(pool)+"_"+std::to_string(i));++pool;
            }
            context.artifact_words=&words;GameRandom random;random.seed(seed);
            if (secret_mode) {
                int width,height;std::size_t rooms;std::cin>>width>>height>>rooms;
                if (width<6 || width>512 || height<6 || height>512 || !rooms || rooms>256) throw std::invalid_argument("invalid secret floor");
                std::vector<bool> blocked(rooms);for (std::size_t i=0;i<rooms;++i) {unsigned value;std::cin>>value;blocked[i]=value!=0;}
                std::vector<DungeonCell> cells(std::size_t(width)*height);std::vector<bool> occupied(cells.size());
                for (std::size_t i=0;i<cells.size();++i) {
                    auto& c=cells[i];unsigned kind,room,door,link,object;std::cin>>kind>>room>>door>>c.flags>>link>>object;
                    if (!std::cin || kind>255 || room>=rooms || door>255 || link>255 || object>1) throw std::invalid_argument("invalid secret floor cell");
                    c.kind=kind;c.room_id=room;c.door_flags=door;c.stair_link=link;occupied[i]=object!=0;
                }
                DungeonLayout layout(width,height,std::move(cells));std::uint64_t identity=0;
                const auto doors=generate_secret_door_plan(layout,random,blocked,occupied,[&](GameRandom& stream) {return make_level_treasure(definitions,tables,context,options,false,++identity,stream);});
                std::cout<<doors.size();
                for (const auto& door:doors) {
                    const auto& item=door.treasure.value();
                    std::cout<<' '<<door.wall.x<<' '<<door.wall.y<<' '<<door.pocket.x<<' '<<door.pocket.y<<' '<<(door.lever?door.lever->x:-1)<<' '<<(door.lever?door.lever->y:-1);
                    std::cout<<' '<<item.definition<<' '<<unsigned(item.kind)<<' '<<item.quantity<<' '<<item.stackable;
                }
                for (const auto& c:layout.cells()) std::cout<<' '<<unsigned(c.kind)<<' '<<unsigned(c.room_id)<<' '<<unsigned(c.door_flags)<<' '<<c.flags<<' '<<unsigned(c.stair_link);
                emit_random(random);continue;
            }
            const auto item=make_level_treasure(definitions,tables,context,options,artifact,1,random);
            std::cout<<item.definition<<' '<<unsigned(item.kind)<<' '<<item.quantity<<' '<<item.stackable<<' '<<bool(item.artifact);
            if (item.artifact) {
                const auto& a=*item.artifact;std::cout<<' '<<hex(a.name)<<' '<<hex(a.description)<<' '<<hex(a.buff_description)<<' '<<a.quality;
                for (auto n:a.modifiers.primary) std::cout<<' '<<n;for (auto n:a.modifiers.secondary) std::cout<<' '<<n;
                for (auto n:a.modifiers.damage) std::cout<<' '<<n;for (auto n:a.modifiers.resistance) std::cout<<' '<<n;for (auto n:a.weapon_damage) std::cout<<' '<<n;
            }
            emit_random(random);
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
