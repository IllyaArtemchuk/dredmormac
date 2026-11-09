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
void Random::restore(const State& state) {
    if (state.index>625) throw std::invalid_argument("invalid MT19937 state index");
    state_=state.words;index_=state.index;
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
std::int32_t DarwinRandom::next() {
    if (state_==0) state_=123459876;
    state_=static_cast<std::uint32_t>((std::uint64_t(state_)*16807u)%2147483647u);
    return static_cast<std::int32_t>(state_);
}
std::uint32_t DarwinRandom::bounded(std::uint32_t count) {
    if (count-1U>=0x7ffffffeU) return 0;
    return static_cast<std::uint32_t>(next())/(0x7fffffffU/count+1U);
}
}
