#include "dredmor/dungeon.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    try {
        const bool shops=argc==2 && std::string(argv[1])=="--shops";
        if (argc!=1 && !shops) throw std::invalid_argument("expected --shops or no arguments");
        std::uint32_t seed;
        while (std::cin>>seed) {
            dredmor::GeometryOptions options;std::size_t count,starter_count;
            if (!(std::cin>>options.width>>options.height>>options.depth>>options.short_game>>options.max_shops>>count>>starter_count) ||
                !count || count>1000 || starter_count>count) throw std::invalid_argument("invalid geometry fixture");
            for (std::size_t i=0,index;i<starter_count;++i) {std::cin>>index;options.starters.push_back(index);}
            std::vector<dredmor::RoomStamp> pool(count);
            for (auto& room:pool) {
                std::cin>>room.width>>room.height>>room.rules.minimum_level>>room.rules.maximum_level>>room.shop>>room.zoo
                    >>room.rules.quest>>room.rules.unique>>room.rules.diggle_god>>std::quoted(room.fixed_name);
                if (!std::cin || room.width<1 || room.height<1 || room.width>512 || room.height>512) throw std::invalid_argument("invalid template");
                if (shops) {
                    std::size_t size;std::cin>>room.shop_markers.home.x>>room.shop_markers.home.y>>room.shop_markers.guard.x>>room.shop_markers.guard.y>>size;
                    if (!std::cin || size>10000) throw std::invalid_argument("invalid shop fixture markers");
                    room.shop_markers.pedestals.resize(size);for (auto& point:room.shop_markers.pedestals) std::cin>>point.x>>point.y;
                }
                room.cells.resize(std::size_t(room.width)*room.height);
                for (auto& cell:room.cells) {
                    unsigned kind,orientation,marker;std::cin>>kind>>orientation>>marker;
                    if (!std::cin || kind>255 || orientation>255 || marker>255) throw std::invalid_argument("invalid cell");
                    cell={std::uint8_t(kind),std::uint8_t(orientation),std::uint8_t(marker)};
                }
            }
            dredmor::RoomNameWords words;
            for (auto* values:{&words.adjectives,&words.architecture,&words.architecture_plural,&words.subjects}) {
                std::size_t size;
                if (!(std::cin>>size) || !size || size>10000) throw std::invalid_argument("invalid geometry word pool");
                for (std::size_t i=0;i<size;++i) {std::string word;std::cin>>std::quoted(word);values->push_back(std::move(word));}
            }
            dredmor::GameRandom random;random.seed(seed);
            const auto result=dredmor::generate_level_geometry(pool,words,random,options);
            std::cout<<result.layout.floor_count()<<' '<<result.shops<<' '<<result.rooms.size()<<' '<<random.libc.state();
            for (int i=0;i<8;++i) std::cout<<' '<<random.mt.u32();
            if (options.depth==0) std::cout<<' '<<result.initial_player->x<<' '<<result.initial_player->y;
            for (const auto& cell:result.layout.cells())
                std::cout<<' '<<unsigned(cell.kind)<<' '<<unsigned(cell.room_id)<<' '<<unsigned(cell.door_flags)<<' '<<cell.flags;
            for (const auto& room:result.rooms) {
                std::cout<<' '<<room.template_index<<' '<<room.origin.x<<' '<<room.origin.y<<' '<<std::quoted(room.name.display)<<' '<<room.neighbors.size();
                for (auto neighbor:room.neighbors) std::cout<<' '<<neighbor;
            }
            if (shops) for (const auto& room:result.rooms) {
                std::cout<<' '<<bool(room.shop);
                if (room.shop) {const auto& shop=*room.shop;std::cout<<' '<<shop.guard.x<<' '<<shop.guard.y<<' '<<shop.home.x<<' '<<shop.home.y;}
            }
            std::cout<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
