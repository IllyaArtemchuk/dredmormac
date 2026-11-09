#include "dredmor/stats.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace dredmor {
namespace {
std::int32_t signed_bits(std::uint32_t bits) {
    std::int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
}
std::int32_t add(std::int32_t a,std::int32_t b) { return signed_bits(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t multiply(std::int32_t a,std::int32_t b) { return signed_bits(std::uint32_t(a)*std::uint32_t(b)); }
std::int32_t truncate(float value) {
    if (!std::isfinite(value) || value>=2147483648.0f || value < -2147483648.0f)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}
}
SecondaryStats default_percentages() { SecondaryStats values;values.fill(100);return values; }
PrimaryStats default_primary_percentages() { PrimaryStats values;values.fill(100);return values; }
PrimaryStats derive_primary(const ClassRanks& ranks,const PrimaryStats& flat,const PrimaryStats& percent) {
    const auto w=ranks[0],m=ranks[1],r=ranks[2];
    PrimaryStats primary={add(add(multiply(w,2),m),r),add(add(w,multiply(m,2)),r),
                          add(add(w,m),multiply(r,2)),add(m,multiply(add(w,r),2)),
                          add(r,multiply(add(w,m),2)),add(w,multiply(add(m,r),2))};
    for (std::size_t i=0;i<primary.size();++i)
        primary[i]=std::max(0,truncate(static_cast<float>(multiply(add(primary[i],flat[i]),percent[i])/100)));
    return primary;
}
SecondaryStats derive_secondary(const PrimaryStats& p,const SecondaryStats& flat,const SecondaryStats& percent) {
    const auto burly=p[0],sage=p[1],nimble=p[2],caddish=p[3],stubborn=p[4],savvy=p[5];
    SecondaryStats s{};
    s[0]=add(burly,caddish);
    s[1]=multiply(sage,2);
    s[2]=burly<8 ? 0 : (burly-5)/3;
    s[3]=sage/2;
    s[4]=caddish/2;
    s[5]=savvy/2;
    s[6]=nimble/2;
    s[7]=add(stubborn/6,burly/6);
    s[8]=add(caddish/6,nimble/6);
    s[9]=nimble/3;
    s[11]=stubborn/2;
    // Retain the two single-precision multiplies used by the original SSE code.
    const float sneak_first=static_cast<float>(add(savvy,nimble))*1.5f;
    s[12]=std::max(0,add(truncate(sneak_first*0.5f),-20));
    s[17]=1;
    for (std::size_t i=0;i<s.size();++i) {
        const auto scaled=multiply(add(s[i],flat[i]),percent[i])/100;
        s[i]=truncate(static_cast<float>(scaled));
        if (i!=13 && i!=14 && i!=18) s[i]=std::max(0,s[i]);
    }
    return s;
}
}
