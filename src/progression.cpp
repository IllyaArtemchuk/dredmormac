#include "dredmor/progression.hpp"
#include <cstring>
#include <stdexcept>
namespace dredmor {
namespace {
std::int32_t value(std::uint32_t bits) { std::int32_t result;std::memcpy(&result,&bits,4);return result; }
}
std::array<std::int32_t,50> experience_table(std::int32_t n) {
    std::array<std::int32_t,50> result;
    for (std::uint32_t i=0;i<50;++i) result[i]=value((i*(i+1)/2)*std::uint32_t(n)+std::uint32_t(n));
    return result;
}
bool Progression::gain(std::int32_t amount,std::int32_t n,bool doubled) {
    if (level<0 || level>=50) throw std::out_of_range("experience table level outside supported original range");
    amount=value(std::uint32_t(amount) << (doubled?1:0));
    experience=value(std::uint32_t(experience)+std::uint32_t(amount));
    lifetime_experience=value(std::uint32_t(lifetime_experience)+std::uint32_t(amount));
    score_experience=value(std::uint32_t(score_experience)+std::uint32_t(amount)*4);
    if (experience_table(n)[level]<experience) {
        ++level;++unspent_points;experience=0;return true;
    }
    return false;
}
}
