#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dredmor {
// The game's MT19937 stream, including its original integer/real interfaces.
// Explicit state keeps sequences independent of the host's unsigned-long size.
class Random {
public:
    Random() = default;
    explicit Random(std::uint32_t value) { seed(value); }
    void seed(std::uint32_t value);
    void seed(const std::vector<std::uint32_t>& key);
    std::uint32_t u32();
    std::int32_t i31() { return static_cast<std::int32_t>(u32() >> 1); }
    double real_closed() { return u32() * (1.0 / 4294967295.0); }
    double real_half_open() { return u32() * (1.0 / 4294967296.0); }
    double real_open() { return (static_cast<double>(u32()) + 0.5) * (1.0 / 4294967296.0); }
    double real53();
private:
    std::array<std::uint32_t, 624> state_{};
    std::size_t index_ = 625;
    void twist();
};
// Portable model of Apple's published non-weak-seeding libc rand algorithm.
// Kept separate from MT19937; actual Mac libc agreement is checked on macOS.
class DarwinRandom {
public:
    explicit DarwinRandom(std::uint32_t seed=1):state_(seed) {}
    void seed(std::uint32_t value) { state_=value; }
    std::int32_t next();
    std::uint32_t state() const { return state_; }
private:
    std::uint32_t state_;
};
struct GameRandom {
    Random mt;
    DarwinRandom libc;
    void seed(std::uint32_t value) { libc.seed(value);mt.seed(value); }
    void wizard_seed(std::uint32_t value) { if (value) { mt.seed(value);libc.seed(value); } }
};
}
