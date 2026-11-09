#include "dredmor/world.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: dredmor-replay ROOM.txt ACTIONS\nProvisional harness; actions: NSWE.\n";
        return 2;
    }
    try {
        std::ifstream file(argv[1]);
        if (!file) throw std::runtime_error("cannot open room");
        auto world = dredmor::World::read(file);
        std::cout << world.snapshot() << '\n';
        for (char c : std::string(argv[2])) {
            world.step(dredmor::parse_action(c));
            std::cout << world.snapshot() << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
