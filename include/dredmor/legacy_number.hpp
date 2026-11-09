#pragma once
#include <cstdint>
#include <limits>
#include <string_view>

namespace dredmor {
// Original XML callbacks use atoi: ASCII whitespace/sign, decimal prefix,
// and zero when no digits exist. Out-of-range inputs saturate to int32 here;
// original overflow behavior has not been executed as a reference.
inline std::int32_t legacy_integer(std::string_view text) {
    std::size_t i=0;
    while (i<text.size() && (text[i]==' ' || text[i]=='\t' || text[i]=='\n' || text[i]=='\r' || text[i]=='\v' || text[i]=='\f')) ++i;
    bool negative=false;
    if (i<text.size() && (text[i]=='-' || text[i]=='+')) {negative=text[i]=='-';++i;}
    const std::uint64_t limit=negative?2147483648ULL:2147483647ULL;std::uint64_t value=0;
    while (i<text.size() && text[i]>='0' && text[i]<='9') {
        const auto digit=static_cast<unsigned>(text[i++]-'0');
        value=value>(limit-digit)/10?limit:value*10+digit;
    }
    if (negative && value==2147483648ULL) return std::numeric_limits<std::int32_t>::min();
    return negative?-static_cast<std::int32_t>(value):static_cast<std::int32_t>(value);
}
}
