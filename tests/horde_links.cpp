#include "dredmor/hordes.hpp"
#include <iostream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("horde lifetime assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        const HordeLinkMap initial{{1,{0,{2,3,2}}},{2,{1,{}}},{3,{1,{}}},{4,{0,{}}}};
        auto follower=initial;unlink_destroyed_horde_member(follower,2);
        check(follower.at(1).followers==std::vector<std::uint64_t>{3} && follower.at(2).leader==1 && follower.at(3).leader==1);
        auto leader=initial;unlink_destroyed_horde_member(leader,1);
        check(!leader.at(2).leader && !leader.at(3).leader && leader.at(1).followers==initial.at(1).followers);
        check(!leader.at(4).leader && leader.at(4).followers.empty());
        HordeLinkMap self{{1,{1,{1,2,2}}},{2,{1,{}}}};unlink_destroyed_horde_member(self,1);
        check((self.at(1).followers==std::vector<std::uint64_t>{2,2}) && self.at(1).leader==1 && !self.at(2).leader);
        auto invalid=initial;invalid.at(1).followers.push_back(99);bool rejected=false;
        try {unlink_destroyed_horde_member(invalid,1);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && invalid.at(2).leader==1 && invalid.at(1).followers.back()==99);
        std::cout<<"horde destruction removes duplicate follower links, clears surviving leaders and preserves unrelated membership\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
