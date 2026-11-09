#include "dredmor/sprite.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace dredmor {
namespace {
constexpr std::size_t budget=64*1024*1024;
std::uint16_t word(const std::vector<std::uint8_t>& bytes,std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]|(unsigned(bytes[offset+1])<<8));
}
}
Sprite Sprite::decode(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size()<9 || bytes.size()>budget || bytes[0]!='S' || bytes[1]!='P' || bytes[2]!='R')
        throw std::invalid_argument("invalid SPR header or file size");
    Sprite result;
    const auto count=word(bytes,3);
    result.width=word(bytes,5);result.height=word(bytes,7);
    const std::size_t pixels=std::size_t(result.width)*result.height;
    if (!count || !pixels || pixels>budget/4 || count>budget/(4*pixels))
        throw std::invalid_argument("SPR dimensions or decoded size exceed limit");
    if (bytes.size()!=9+std::size_t(count)*(770+pixels))
        throw std::invalid_argument("truncated or unsupported SPR layout");
    result.frames.reserve(count);
    for (std::size_t i=0;i<count;++i) {
        const auto start=9+i*(770+pixels);
        SpriteFrame frame;frame.delay_ms=word(bytes,start);frame.rgba.resize(pixels*4);
        const auto palette=start+2,indices=palette+768;
        for (std::size_t p=0;p<pixels;++p) {
            const auto index=bytes[indices+p];
            std::copy_n(bytes.begin()+palette+3*index,3,frame.rgba.begin()+4*p);
            frame.rgba[4*p+3]=index?255:0;
        }
        result.frames.push_back(std::move(frame));
    }
    return result;
}
Sprite Sprite::read(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if (!input) throw std::runtime_error("cannot open SPR: "+path.string());
    const auto size=input.tellg();
    if (size<0 || size>static_cast<std::streamoff>(budget)) throw std::invalid_argument("SPR file size exceeds limit");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("cannot read SPR: "+path.string());
    return decode(bytes);
}
std::size_t Sprite::frame_at(std::uint64_t elapsed_ms) const {
    if (frames.empty()) throw std::logic_error("empty sprite has no frame");
    std::uint64_t duration=0;
    for (const auto& frame:frames) duration+=std::max<unsigned>(1,frame.delay_ms);
    auto phase=elapsed_ms%duration;
    for (std::size_t i=0;i<frames.size();++i) {
        const auto delay=std::max<unsigned>(1,frames[i].delay_ms);
        if (phase<delay) return i;
        phase-=delay;
    }
    throw std::logic_error("invalid animation duration");
}
}
