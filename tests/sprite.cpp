#include "dredmor/sprite.hpp"
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("sprite assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    try {
        // Two frames with distinct palettes, transparency and unequal durations.
        std::vector<std::uint8_t> bytes={'S','P','R',2,0,2,0,1,0};
        for (unsigned i=0;i<2;++i) {
            bytes.push_back(i?200:100);bytes.push_back(0);
            for (unsigned c=0;c<256;++c) {
                bytes.push_back(c);bytes.push_back(i?200:100);bytes.push_back(255-c);
            }
            bytes.push_back(0);bytes.push_back(i?255:1);
        }
        auto sprite=dredmor::Sprite::decode(bytes);
        check(sprite.width==2 && sprite.height==1 && sprite.frames.size()==2);
        check(sprite.frames[0].rgba==std::vector<std::uint8_t>({0,100,255,0,1,100,254,255}));
        check(sprite.frames[1].rgba==std::vector<std::uint8_t>({0,200,255,0,255,200,0,255}));
        check(sprite.frame_at(0)==0 && sprite.frame_at(99)==0 && sprite.frame_at(100)==1);
        check(sprite.frame_at(299)==1 && sprite.frame_at(300)==0 && sprite.frame_at(UINT64_MAX)<2);
        sprite.frames[0].delay_ms=0;check(sprite.frame_at(1)==1 && sprite.frame_at(201)==0);
        auto reject=[](const auto& data) {
            try {dredmor::Sprite::decode(data);return false;} catch(const std::invalid_argument&) {return true;}
        };
        for (std::size_t size=0;size<bytes.size();++size) {
            check(reject(std::vector<std::uint8_t>(bytes.begin(),bytes.begin()+size)));
        }
        auto invalid=bytes;invalid.push_back(0);check(reject(invalid));
        invalid=bytes;invalid[0]='X';check(reject(invalid));
        invalid=bytes;invalid[3]=0;check(reject(invalid));
        invalid=bytes;invalid[5]=invalid[6]=invalid[7]=invalid[8]=255;check(reject(invalid));
        bool empty=false;try {dredmor::Sprite{}.frame_at(0);} catch (const std::logic_error&) {empty=true;}check(empty);
        std::cout<<"SPR palettes, all frames, loop boundaries and malformed data passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
