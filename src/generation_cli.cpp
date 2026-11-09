#include "dredmor/generation.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {
std::vector<std::uint8_t> decode(const std::string& hex) {
    if (hex.size()>512*512*2 || hex.size()%2) throw std::invalid_argument("invalid grid hex length");
    auto digit=[](char c) {
        if (c>='0' && c<='9') return c-'0';
        if (c>='a' && c<='f') return c-'a'+10;
        throw std::invalid_argument("invalid grid hex digit");
    };
    std::vector<std::uint8_t> bytes;bytes.reserve(hex.size()/2);
    for (std::size_t i=0;i<hex.size();i+=2) bytes.push_back(digit(hex[i])*16+digit(hex[i+1]));
    return bytes;
}
}
int main() {
    try {
        std::string line;
        while (std::getline(std::cin,line)) {
            int width,height,rw,rh,x,y;std::string level,room,extra;std::istringstream input(line);
            if (!(input>>width>>height>>rw>>rh>>x>>y>>level>>room) || (input>>extra))
                throw std::invalid_argument("expected level/room dimensions, x y and two hex cell arrays");
            std::cout<<dredmor::TileGrid(width,height,decode(level)).accepts(dredmor::TileGrid(rw,rh,decode(room)),x,y)<<'\n';
        }
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
