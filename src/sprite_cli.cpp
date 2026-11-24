#include "dredmor/sprite.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        const bool selection=argc==2 && std::string(argv[1])=="--palette-select";
        if (selection || (argc==2 && std::string(argv[1])=="--palette")) {
            std::int32_t angle;
            while (std::cin>>angle) {
                bool use_replacement=false;if (selection && !(std::cin>>use_replacement)) throw std::invalid_argument("missing palette selection");
                dredmor::SpritePalette palette,replacement;
                for (auto& colour:palette) {
                    unsigned value;
                    if (!(std::cin>>value) || value>255) throw std::invalid_argument("invalid palette byte");
                    colour=static_cast<std::uint8_t>(value);
                }
                if (selection) {
                    if (use_replacement) for (auto& colour:replacement) {
                        unsigned value;if (!(std::cin>>value) || value>255) throw std::invalid_argument("invalid replacement byte");colour=static_cast<std::uint8_t>(value);
                    }
                    std::vector<std::uint8_t> bytes={'S','P','R',1,0,0,1,1,0,0,0};
                    bytes.insert(bytes.end(),palette.begin(),palette.end());
                    for (unsigned i=0;i<256;++i) bytes.push_back(static_cast<std::uint8_t>(i));
                    const auto sprite=dredmor::Sprite::decode(bytes,angle,use_replacement?&replacement:nullptr);
                    for (unsigned i=0;i<256;++i) for (unsigned c=0;c<3;++c) palette[i*3+c]=sprite.frames[0].rgba[i*4+c];
                } else dredmor::tint_sprite_palette(palette,angle);
                for (auto colour:palette) std::cout<<unsigned(colour)<<' ';
                std::cout<<'\n';
            }
            return 0;
        }
        if (argc!=2) throw std::invalid_argument("Usage: dredmor-sprite FILE.spr");
        const auto sprite=dredmor::Sprite::read(argv[1]);
        // Stable RGBA/delay fingerprint for independent full-frame comparisons.
        std::uint64_t hash=14695981039346656037ull;
        auto byte=[&](std::uint8_t value){hash=(hash^value)*1099511628211ull;};
        for (const auto& frame:sprite.frames) {
            byte(frame.delay_ms&255);byte(frame.delay_ms>>8);
            for (auto value:frame.rgba) byte(value);
        }
        std::cout<<"{\"width\":"<<sprite.width<<",\"height\":"<<sprite.height
                 <<",\"frames\":"<<sprite.frames.size()<<",\"hash\":\""<<std::hex<<hash<<"\"}\n";
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
