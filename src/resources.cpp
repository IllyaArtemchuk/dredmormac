#include "dredmor/resources.hpp"
#include <stdexcept>
#include <algorithm>

namespace dredmor {
std::vector<ResourceMount> initial_resource_mounts() {
    std::vector<ResourceMount> result;
    for (const std::string layer:{"expansion3","expansion2","expansion"}) {
        result.push_back({layer+"/game","/"+layer+"/",true});
        for (const std::string folder:{"dungeon","items","skills","sprites","sfx","tilesets","ui"})
            result.push_back({layer+"/"+folder,"/"+folder+"/",true});
    }
    result.push_back({"dredmorcore.zip","/",false});result.push_back({".","/",true});return result;
}
std::filesystem::path resolve_directory_resource(const std::filesystem::path& root,std::string relative) {
    std::replace(relative.begin(),relative.end(),'\\','/');const std::filesystem::path input(relative);
    if (input.empty() || input.is_absolute()) throw std::invalid_argument("game resource must be relative");
    for (const auto& part:input) if (part=="..") throw std::invalid_argument("game resource escapes installation");
    relative=input.lexically_normal().generic_string();const auto canonical_root=std::filesystem::canonical(root);
    std::vector<ResourceMount> mounted;
    for (const auto& mount:initial_resource_mounts()) if (std::filesystem::exists(canonical_root/mount.source)) {
        if (mount.append) mounted.push_back(mount);else mounted.insert(mounted.begin(),mount);
    }
    const auto virtual_name="/"+relative;
    for (const auto& mount:mounted) {
        if (virtual_name.compare(0,mount.point.size(),mount.point)!=0) continue;
        const auto source=canonical_root/mount.source;
        if (!std::filesystem::is_directory(source)) {
            if (std::filesystem::is_regular_file(source)) throw std::runtime_error("packed core ZIP resource lookup is not implemented");
            continue;
        }
        const auto candidate=source/virtual_name.substr(mount.point.size());
        if (!std::filesystem::is_regular_file(candidate)) continue;
        const auto resolved=std::filesystem::canonical(candidate),within=resolved.lexically_relative(canonical_root);
        if (within.empty() || *within.begin()=="..") throw std::invalid_argument("game resource escapes installation");
        return resolved;
    }
    throw std::runtime_error("game resource not found: "+relative);
}
}
