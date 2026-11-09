#include "dredmor/progression.hpp"
#include <iostream>
int main() {
    std::int32_t n;
    while (std::cin>>n) {
        auto table=dredmor::experience_table(n);
        for (std::size_t i=0;i<table.size();++i) std::cout<<(i?" ":"")<<table[i];
        std::cout<<'\n';
    }
    return std::cin.eof()?0:1;
}
