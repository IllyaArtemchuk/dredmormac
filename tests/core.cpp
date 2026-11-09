#include "dredmor/world.hpp"
#include "dredmor/distance.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

void check(bool condition) {
    if (!condition) throw std::runtime_error("test assertion failed");
}
int main() {
    using namespace dredmor;
    try {
        check(dredmor::distance(0,0,3,4) == 5);
        check(dredmor::distance(0,0,1,1) == 1);
        check(dredmor::distance(0,0,46341,0) == -2147483647-1);
        check(dredmor::distance(0,0,65536,0) == 0);
        std::istringstream room("#####\r\n#@..#\r\n##..#\r\n#####\r\n");
        auto w = World::read(room);
        check(w.step(Action::west) == Outcome::blocked && w.turn() == 0);
        check(w.step(Action::south) == Outcome::blocked && w.turn() == 0);
        check(w.step(Action::east) == Outcome::moved);
        check(w.step(Action::south) == Outcome::moved);
        check(w.step(Action::wait) == Outcome::waited);
        check(w.snapshot() == "{\"turn\":3,\"x\":2,\"y\":2}");
        for (auto bad : {"", "..", "@@", "@\n..", "@?", "@\n\n"}) {
            bool rejected = false;
            try { std::istringstream in(bad); World::read(in); }
            catch (const std::runtime_error&) { rejected = true; }
            check(rejected);
        }
        std::istringstream edge("@"); auto e = World::read(edge);
        for (auto a : {Action::north, Action::south, Action::east, Action::west})
            check(e.step(a) == Outcome::blocked && e.turn() == 0);
        std::cout << "core: collision, replay, bounds and malformed rooms passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
