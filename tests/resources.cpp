#include "dredmor/resources.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#define check(value) do {if (!(value)) throw std::runtime_error("resource assertion at line "+std::to_string(__LINE__));} while(false)
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("dredmor-resources-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 1;
    struct Cleanup {fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{root};
    try {
        auto file=[&](const fs::path& name){fs::create_directories((root/name).parent_path());std::ofstream(root/name)<<"synthetic fixture";};
        file("dungeon/fixture.png");file("expansion/dungeon/fixture.png");file("expansion2/dungeon/fixture.png");file("expansion3/dungeon/fixture.png");
        const auto canonical=fs::canonical(root);
        check(dredmor::resolve_directory_resource(root,"dungeon/fixture.png")==canonical/"expansion3/dungeon/fixture.png");
        fs::remove(root/"expansion3/dungeon/fixture.png");
        check(dredmor::resolve_directory_resource(root,"dungeon\\fixture.png")==canonical/"expansion2/dungeon/fixture.png");
        file("expansion2/game/fixture.xml");file("game/fixture.xml");
        check(dredmor::resolve_directory_resource(root,"expansion2/fixture.xml")==canonical/"expansion2/game/fixture.xml");
        check(dredmor::resolve_directory_resource(root,"game/fixture.xml")==canonical/"game/fixture.xml");
        for (const auto& name:{"../escape.png","dungeon/../../escape.png","/absolute.png","missing.png"}) {
            bool rejected=false;try {dredmor::resolve_directory_resource(root,name);} catch(const std::exception&) {rejected=true;}check(rejected);
        }
        fs::create_symlink(root.parent_path(),root/"escape");bool rejected=false;
        try {dredmor::resolve_directory_resource(root,"escape/"+root.filename().string()+"/game/fixture.xml");} catch(const std::invalid_argument&) {rejected=true;}
        // This symlink resolves back inside the root, which is permitted.
        check(!rejected);
        fs::remove(root/"escape");fs::create_symlink(root.parent_path(),root/"dungeon/outside");
        const auto external=root.parent_path()/(root.filename().string()+"-external");std::ofstream(external)<<"synthetic external file";
        try {dredmor::resolve_directory_resource(root,"dungeon/outside/"+external.filename().string());} catch(const std::invalid_argument&) {rejected=true;}
        fs::remove(external);check(rejected);
        std::cout<<"resource expansion precedence, virtual game mounts, loose fallback and canonical containment passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
