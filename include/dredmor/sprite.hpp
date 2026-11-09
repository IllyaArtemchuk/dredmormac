#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace dredmor {
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
    static Sprite decode(const std::vector<std::uint8_t>& bytes);
    static Sprite read(const std::filesystem::path& path);
};
}
