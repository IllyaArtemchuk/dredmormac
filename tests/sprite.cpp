#include "dredmor/sprite.hpp"
#include <algorithm>
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
        const auto tinted=dredmor::Sprite::decode(bytes,120);
        check(tinted.frames.size()==2 && tinted.frames[1].delay_ms==200);
        for (std::size_t i=0;i<2;++i) {
            dredmor::SpritePalette palette;
            std::copy_n(bytes.begin()+11+i*772,768,palette.begin());
            dredmor::tint_sprite_palette(palette,120);
            for (std::size_t pixel=0;pixel<2;++pixel) {
                const auto index=bytes[779+i*772+pixel];
                for (std::size_t c=0;c<3;++c) check(tinted.frames[i].rgba[pixel*4+c]==palette[index*3+c]);
                check(tinted.frames[i].rgba[pixel*4+3]==sprite.frames[i].rgba[pixel*4+3]);
            }
        }
        dredmor::SpritePalette palette{};palette[0]=255;palette[4]=255;palette[8]=255;
        const auto original=palette;
        dredmor::tint_sprite_palette(palette,-1);check(palette==original);
        dredmor::tint_sprite_palette(palette,120);
        check(palette[0]==0 && palette[1]==255 && palette[2]==0);
        check(palette[3]==0 && palette[4]==0 && palette[5]==255);
        check(palette[6]==255 && palette[7]==0 && palette[8]==0);
        check(palette[9]==0 && palette[10]==0 && palette[11]==0);
        dredmor::SpritePalette rounding{};rounding[0]=1;rounding[1]=100;rounding[2]=254;
        auto wrapped=rounding;
        dredmor::tint_sprite_palette(rounding,0);
        dredmor::tint_sprite_palette(wrapped,360);
        check(rounding[0]==0 && rounding[1]==99 && rounding[2]==254);
        check(wrapped[0]==0 && wrapped[1]==100 && wrapped[2]==254);
        const auto unchanged=palette;
        bool angle_guard=false;try {dredmor::tint_sprite_palette(palette,360001);} catch(const std::invalid_argument&) {angle_guard=true;}
        check(angle_guard && palette==unchanged);
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
        std::cout<<"SPR palettes, tint conversion/transparency, all frames, loop boundaries and malformed data passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
