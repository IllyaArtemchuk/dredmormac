#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace dredmor {
struct ResourceMount {std::string source,point;bool append=true;};
// Fixed SDL_main mount requests, before mods. Missing directories are skipped.
std::vector<ResourceMount> initial_resource_mounts();
// Directory-backed virtual resource lookup, preserving original expansion
// precedence. The supplied installation has loose files and no core ZIP.
// Packed core ZIP lookup and runtime mod mounts require further implementation.
std::filesystem::path resolve_directory_resource(const std::filesystem::path& root,std::string relative);
}
