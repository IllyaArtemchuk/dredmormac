#include "dredmor/random.hpp"
#include <algorithm>
#include <stdexcept>

namespace dredmor {
void Random::seed(std::uint32_t value) {
    state_[0] = value;
    for (std::size_t i=1; i<state_.size(); ++i)
        state_[i] = 1812433253u * (state_[i-1] ^ (state_[i-1] >> 30)) + static_cast<std::uint32_t>(i);
    index_ = state_.size();
}
void Random::seed(const std::vector<std::uint32_t>& key) {
    if (key.empty()) throw std::invalid_argument("RNG seed key must not be empty");
    seed(19650218u);
    std::size_t i=1, j=0;
    for (auto count=std::max(state_.size(),key.size()); count; --count) {
        state_[i] = (state_[i] ^ ((state_[i-1] ^ (state_[i-1] >> 30)) * 1664525u))
                    + key[j] + static_cast<std::uint32_t>(j);
        if (++i == state_.size()) { state_[0] = state_.back(); i=1; }
        if (++j == key.size()) j=0;
    }
    for (std::size_t count=state_.size()-1; count; --count) {
        state_[i] = (state_[i] ^ ((state_[i-1] ^ (state_[i-1] >> 30)) * 1566083941u))
                    - static_cast<std::uint32_t>(i);
        if (++i == state_.size()) { state_[0] = state_.back(); i=1; }
    }
    state_[0] = 0x80000000u;
}
void Random::twist() {
    if (index_ == state_.size()+1) seed(5489u);
    for (std::size_t i=0; i<state_.size(); ++i) {
        const auto joined = (state_[i] & 0x80000000u) | (state_[(i+1)%624] & 0x7fffffffu);
        state_[i] = state_[(i+397)%624] ^ (joined >> 1) ^ ((joined & 1u) ? 0x9908b0dfu : 0u);
    }
    index_=0;
}
std::uint32_t Random::u32() {
    if (index_ >= state_.size()) twist();
    auto value=state_[index_++];
    value ^= value >> 11;
    value ^= (value << 7) & 0x9d2c5680u;
    value ^= (value << 15) & 0xefc60000u;
    value ^= value >> 18;
    return value;
}
double Random::real53() {
    const auto a=u32() >> 5;
    const auto b=u32() >> 6;
    return (a*67108864.0+b) * (1.0/9007199254740992.0);
}
}
