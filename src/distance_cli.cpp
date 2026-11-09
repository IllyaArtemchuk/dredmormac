#include "dredmor/distance.hpp"
#include <iostream>
int main() {
    std::int32_t a,b,c,d;
    while (std::cin >> a >> b >> c >> d) std::cout << dredmor::distance(a,b,c,d) << '\n';
    return std::cin.eof() ? 0 : 1;
}
