#include "dredmor/random.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    dredmor::Random random;
    std::string command;
    try {
        while (std::cin >> command) {
            if (command == "seed") {
                std::uint32_t value;
                if (!(std::cin >> value)) throw std::runtime_error("missing seed");
                random.seed(value);
            } else if (command == "array") {
                std::size_t size;
                if (!(std::cin >> size) || size==0 || size>10000) throw std::runtime_error("invalid key size");
                std::vector<std::uint32_t> key(size);
                for (auto& item:key) if (!(std::cin >> item)) throw std::runtime_error("missing key item");
                random.seed(key);
            } else if (command == "u32") std::cout << random.u32() << '\n';
            else if (command == "i31") std::cout << random.i31() << '\n';
            else {
                double value;
                if (command == "r1") value=random.real_closed();
                else if (command == "r2") value=random.real_half_open();
                else if (command == "r3") value=random.real_open();
                else if (command == "r53") value=random.real53();
                else throw std::runtime_error("unknown RNG command");
                std::uint64_t bits;
                std::memcpy(&bits,&value,sizeof(bits));
                std::cout << bits << '\n';
            }
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
