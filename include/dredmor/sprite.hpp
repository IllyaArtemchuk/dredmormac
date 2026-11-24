#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace dredmor {
using SpritePalette=std::array<std::uint8_t,768>;
// Original SPR palette conversion; -1 bypasses the conversion. The port
// bounds angles to +/-360000 to keep malformed resource requests bounded.
void tint_sprite_palette(SpritePalette& palette,std::int32_t angle);
SpritePalette read_sprite_palette(const std::filesystem::path& path);
struct SpriteFrame {
    std::uint16_t delay_ms=0;
    std::vector<std::uint8_t> rgba;
};
struct Sprite {
    std::uint16_t width=0,height=0;
    std::vector<SpriteFrame> frames;
    // Presentation-only loop; zero-delay frames receive one millisecond.
    // Original animation scheduling has not yet been compared.
    std::size_t frame_at(std::uint64_t elapsed_ms) const;
    static Sprite decode(const std::vector<std::uint8_t>& bytes,std::int32_t tint=-1,const SpritePalette* palette=nullptr);
    static Sprite read(const std::filesystem::path& path,std::int32_t tint=-1,const SpritePalette* palette=nullptr);
};
}
