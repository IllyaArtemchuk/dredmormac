#include "dredmor/hordes.hpp"
#include <algorithm>
#include <stdexcept>

namespace dredmor {
void unlink_destroyed_horde_member(HordeLinkMap& members,std::uint64_t identity) {
    const auto found=members.find(identity);
    if (!identity || found==members.end()) throw std::invalid_argument("horde destruction requires a live member");
    auto& member=found->second;
    // Bad raw pointers are not meaningful original inputs. Validate the links
    // that this phase actually dereferences before mutating the port graph.
    if (member.leader && !members.count(member.leader)) throw std::invalid_argument("horde destruction has a missing leader");
    for (auto id:member.followers) if (!id || !members.count(id)) throw std::invalid_argument("horde destruction has a missing follower");
    if (member.leader) {
        auto& followers=members.at(member.leader).followers;
        followers.erase(std::remove(followers.begin(),followers.end(),identity),followers.end());
    }
    for (auto id:member.followers) members.at(id).leader=0;
}
}
