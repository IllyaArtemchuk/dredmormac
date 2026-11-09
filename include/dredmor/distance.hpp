#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace dredmor {
// Matches i386 distance(int,int,int,int): wrapping 32-bit arithmetic, then
// signed int -> float -> sqrt(float) -> truncating int conversion.
inline std::int32_t distance(std::int32_t x1, std::int32_t y1,
                             std::int32_t x2, std::int32_t y2) {
    const auto dx = std::uint32_t(x1) - std::uint32_t(x2);
    const auto dy = std::uint32_t(y1) - std::uint32_t(y2);
    const std::uint32_t squared = dx*dx + dy*dy;
    std::int32_t signed_squared;
    std::memcpy(&signed_squared, &squared, sizeof(squared));
    const float root = std::sqrt(static_cast<float>(signed_squared));
    if (!std::isfinite(root) || root >= 2147483648.0f)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(root);
}
}
