#include "dredmor/content.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <type_traits>

void sprite(const dredmor::RoomSprite& value) {
    std::cout<<' '<<std::quoted(value.image)<<' '<<std::quoted(value.series)<<' '<<value.first<<' '<<value.count<<' '<<value.rate;
}
void snapshot(const dredmor::BreakableRoomObject& value) {
    const auto& object=value.object;
    std::cout<<' '<<object.position.x<<' '<<object.position.y<<' '<<unsigned(object.can_push)<<' '<<object.blocks_movement<<' '<<object.blocks_vision
        <<' '<<object.frame<<' '<<object.playing<<' '<<object.looping<<' '<<value.hits_needed<<' '<<value.hits_taken<<' '
        <<(value.single_breaking_delay?value.single_breaking_delay:100)<<' '<<(value.hits_taken>=value.hits_needed?2:0)
        <<' '<<std::quoted(object.name)<<' '<<std::quoted(object.description)<<' '<<std::quoted(object.room_local_id);
}
int main(int argc,char** argv) {
    try {
        using namespace dredmor;
        if (argc>1) {
            if ((argc!=3 && argc!=4) || std::string(argv[1])!="--content") throw std::invalid_argument("expected --content ROOT [--expansions]");
            const auto content=GameContent::load(argv[2],argc==4);
            for (std::size_t room=0;room<content.rooms().size();++room) {
                const auto records=content.rooms()[room].records();
                for (std::size_t order=0;order<records.size();++order) std::visit([&](const auto& record) {
                    using T=std::decay_t<decltype(record)>;
                    std::string type;
                    if constexpr (std::is_same_v<T,CustomRoomDefinition>) type=record.kind==CustomRoomKind::blocker?"customblocker":"customengraving";
                    else if constexpr (std::is_same_v<T,BreakableRoomDefinition>) type="custombreakable";
                    else if constexpr (std::is_same_v<T,RoomPedestalRecord>) type="pedestal";
                    else if constexpr (std::is_same_v<T,RoomElementDefinition>) type="element";
                    else if constexpr (std::is_same_v<T,RoomMonsterRecord>) type="monster";
                    else if constexpr (std::is_same_v<T,RoomTrapRecord>) type="trap";
                    else if constexpr (std::is_same_v<T,RoomLootRecord>) type="loot";
                    else if constexpr (std::is_same_v<T,RoomHordeRecord>) type="horde";
                    else type=record.type;
                    std::cout<<room<<' '<<order<<' '<<type;
                    if constexpr (!std::is_same_v<T,UnimplementedRoomRecord>) {
                        const auto& p=record.placement;std::cout<<' '<<p.relative.x<<' '<<p.relative.y<<' '<<p.percent<<' '<<unsigned(p.can_push)<<' '<<std::quoted(p.id);
                        if constexpr (std::is_same_v<T,BreakableRoomDefinition>) {
                            std::cout<<' '<<record.hits_needed<<' '<<record.hits_taken<<' '<<record.breaking.has_value();
                            sprite(record.intact);sprite(record.breaking.value_or(record.intact));sprite(record.broken);
                            std::cout<<' '<<std::quoted(record.name)<<' '<<std::quoted(record.description)<<' '<<record.passable.has_value()<<' '<<record.passable.value_or(false)
                                <<' '<<record.vis_block.has_value()<<' '<<record.vis_block.value_or(false);
                        } else if constexpr (std::is_same_v<T,RoomPedestalRecord>) std::cout<<' '<<std::quoted(record.name)<<' '<<std::quoted(record.text);
                    }
                    std::cout<<'\n';
                },records[order]);
            }
            return 0;
        }
        std::uint32_t seed;
        while (std::cin>>seed) {
            BreakableRoomDefinition definition;Position origin;unsigned push,intact,breaking,broken,mode,hits;bool pass_set,pass,vis_set,vis,has_breaking;
            auto& p=definition.placement;
            std::cin>>p.percent>>origin.x>>origin.y>>p.relative.x>>p.relative.y>>push>>pass_set>>pass>>vis_set>>vis
                >>definition.hits_needed>>definition.hits_taken>>intact>>breaking>>broken>>has_breaking>>mode>>hits
                >>std::quoted(definition.name)>>std::quoted(definition.description)>>std::quoted(p.id);
            if (!std::cin || push>255 || !intact || !breaking || !broken || intact>65535 || breaking>65535 || broken>65535 || hits>100)
                throw std::invalid_argument("invalid breakable fixture");
            p.can_push=push;if (pass_set) definition.passable=pass;if (vis_set) definition.vis_block=vis;
            auto make_sprite=[&](const char* name,unsigned frames,unsigned bit) {
                RoomSprite result;if (mode&(1U<<bit)) {result.series=std::string(name)+"_series";result.first=2;result.count=frames;result.rate=100;}
                else result.image=std::string(name)+".spr";return result;
            };
            definition.intact=make_sprite("intact",intact,0);definition.broken=make_sprite("broken",broken,2);
            if (has_breaking) definition.breaking=make_sprite("breaking",breaking,1);
            GameRandom random;random.seed(seed);auto object=create_breakable_room_object(definition,origin,random.libc,
                {static_cast<std::uint16_t>(intact),static_cast<std::uint16_t>(breaking),static_cast<std::uint16_t>(broken)});
            std::cout<<bool(object)<<' '<<random.libc.state();
            if (object) {
                snapshot(*object);
                for (unsigned i=0;i<hits;++i) {
                    const auto result=hit_breakable(*object);std::cout<<' '<<result.handled<<' '<<result.broken_now<<' '<<result.triggers.size();
                    for (const auto& trigger:result.triggers) std::cout<<' '<<std::quoted(trigger);
                    std::cout<<' '<<(result.animation?(has_breaking?1:0):-1);snapshot(*object);
                }
            }
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
