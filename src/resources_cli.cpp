#include "dredmor/resources.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--mounts") {
            for (const auto& mount:dredmor::initial_resource_mounts()) std::cout<<std::quoted(mount.source)<<' '<<std::quoted(mount.point)<<' '<<mount.append<<'\n';
            return 0;
        }
        if (argc!=2) throw std::invalid_argument("expected --mounts or installation root");
        const auto root=std::filesystem::canonical(argv[1]);std::string relative;
        while (std::getline(std::cin,relative)) std::cout<<dredmor::resolve_directory_resource(root,relative).lexically_relative(root).generic_string()<<'\n';
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
