#include "dredmor/world.hpp"
#include "dredmor/distance.hpp"
#include "dredmor/random.hpp"
#include "dredmor/legacy_number.hpp"
#include "dredmor/stats.hpp"
#include <random>
#include <iostream>
#include <sstream>
#include <stdexcept>

void check(bool condition) {
    if (!condition) throw std::runtime_error("test assertion failed");
}
int main() {
    using namespace dredmor;
    try {
        check(legacy_integer(" \t+003tail")==3 && legacy_integer("-12.5")==-12 && legacy_integer("none")==0);
        check(legacy_integer("2147483648")==2147483647 && legacy_integer("-2147483649")==(-2147483647-1));
        Random bounded(1),reference(1);
        check(bounded.bounded(0)==0 && bounded.bounded(0xffffffffU)==0 && bounded.u32()==reference.u32());
        check(bounded.bounded(1)==0);reference.u32();check(bounded.u32()==reference.u32());
        check(dredmor::derive_primary({3,2,2}) == dredmor::PrimaryStats({10,9,9,12,12,11}));
        auto stats=dredmor::derive_secondary({10,10,10,10,10,10});
        check(stats[0]==20 && stats[1]==20 && stats[2]==1 && stats[3]==5);
        check(stats[6]==5 && stats[7]==2 && stats[8]==2 && stats[9]==3);
        dredmor::SecondaryStats flat{};
        flat[0]=-100;flat[13]=-4;flat[14]=-5;flat[18]=-3;
        auto percent=dredmor::default_percentages();percent[1]=150;
        stats=dredmor::derive_secondary({10,10,10,10,10,10},flat,percent);
        check(stats[0]==0 && stats[1]==30 && stats[13]==-4 && stats[14]==-5 && stats[18]==-3);
        // Independent standard-library MT19937 checks run without game files.
        for (auto seed : {0u,1u,5489u,0xffffffffu}) {
            dredmor::Random rng(seed); std::mt19937 standard(seed);
            for (int i=0;i<2000;++i) check(rng.u32()==standard());
        }
        dredmor::Random implicit, explicit_seed(5489u);
        for (int i=0;i<1250;++i) check(implicit.u32()==explicit_seed.u32());
        dredmor::Random array_seed;
        array_seed.seed(std::vector<std::uint32_t>{0x123,0x234,0x345,0x456});
        for (auto expected : {1067595299u,955945823u,477289528u,4107218783u})
            check(array_seed.u32()==expected);
        bool rejected_key=false;
        try { array_seed.seed(std::vector<std::uint32_t>{}); }
        catch (const std::invalid_argument&) { rejected_key=true; }
        check(rejected_key);
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
