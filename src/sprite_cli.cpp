#include "dredmor/sprite.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
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
