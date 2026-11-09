#include "dredmor/catalog.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    if (argc<2 || argc>3 || (argc==3 && std::string(argv[2])!="--expansions")) {
        std::cerr << "Usage: dredmor-data DATA_ROOT [--expansions]\n"; return 2;
    }
    try {
        auto catalog=dredmor::Catalog::load(argv[1],argc==3);
        for (const auto& db:catalog.databases())
            std::cout << db->layer << '\t' << db->source.filename().string() << '\t'
                      << db->root.tag << '\t' << db->root.children.size() << '\n';
        std::map<std::string,std::size_t> counts;
        for (const auto& entry:catalog.entries()) ++counts[entry.database->layer+"/"+entry.node->tag];
        for (const auto& count:counts) std::cout << count.first << '\t' << count.second << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n';return 1; }
}
