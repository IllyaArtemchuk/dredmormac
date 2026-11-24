#include "dredmor/hordes.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        std::size_t size;std::uint64_t victim;
        while (std::cin>>size>>victim) {
            if (!size || size>10000) throw std::invalid_argument("invalid horde link fixture size");
            dredmor::HordeLinkMap graph;
            for (std::size_t i=0;i<size;++i) {
                std::uint64_t id;dredmor::HordeLinks links;std::size_t count;std::cin>>id>>links.leader>>count;
                if (!id || count>10000) throw std::invalid_argument("invalid horde link record");
                links.followers.resize(count);for (auto& follower:links.followers) std::cin>>follower;
                if (!graph.emplace(id,std::move(links)).second) throw std::invalid_argument("duplicate horde member identity");
            }
            if (!std::cin) throw std::runtime_error("truncated horde link fixture");
            dredmor::unlink_destroyed_horde_member(graph,victim);std::cout<<graph.size();
            for (const auto& [id,links]:graph) {std::cout<<' '<<id<<' '<<links.leader<<' '<<links.followers.size();for (auto follower:links.followers) std::cout<<' '<<follower;}
            std::cout<<'\n';
        }
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
