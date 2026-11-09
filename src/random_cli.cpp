#include "dredmor/random.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    dredmor::Random random;
    dredmor::NormalRandom normal;
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
            } else if (command == "normal") {
                std::uint32_t mean_bits,deviation_bits;
                if (!(std::cin>>mean_bits>>deviation_bits)) throw std::runtime_error("missing normal parameters");
                float mean,deviation;std::memcpy(&mean,&mean_bits,4);std::memcpy(&deviation,&deviation_bits,4);
                const auto value=normal.sample(mean,deviation,random);std::uint32_t result,spare;
                const auto state=normal.state();std::memcpy(&result,&value,4);std::memcpy(&spare,&state.spare,4);
                std::cout<<result<<' '<<state.available<<' '<<spare<<'\n';
            } else if (command == "normal-reset") {
                unsigned available;std::uint32_t bits;
                if (!(std::cin>>available>>bits) || available>1) throw std::runtime_error("invalid normal cache");
                float spare;std::memcpy(&spare,&bits,4);normal.restore({available!=0,spare});
            } else if (command == "u32") std::cout << random.u32() << '\n';
            else if (command == "bounded") {
                std::uint32_t count;if (!(std::cin>>count)) throw std::runtime_error("missing MT bound");
                std::cout<<random.bounded(count)<<'\n';
            }
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
