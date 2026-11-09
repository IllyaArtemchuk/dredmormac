#include "dredmor/catalog.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace dredmor {
Catalog Catalog::load(const std::filesystem::path& data_root,bool expansions) {
    Catalog catalog;
    std::vector<std::string> layers={"base"};
    if (expansions) layers.insert(layers.end(),{"expansion","expansion2","expansion3"});
    for (const auto& layer:layers) {
        auto folder=layer=="base" ? data_root/"game" : data_root/layer/"game";
        if (!std::filesystem::is_directory(folder))
            throw std::runtime_error("missing selected database directory: "+folder.string());
        std::vector<std::filesystem::path> paths;
        for (const auto& entry:std::filesystem::directory_iterator(folder))
            if (entry.is_regular_file() && entry.path().extension()==".xml") paths.push_back(entry.path());
        std::sort(paths.begin(),paths.end());
        if (paths.empty()) throw std::runtime_error("no XML databases: "+folder.string());
        for (const auto& path:paths) {
            auto db=std::make_unique<Database>(Database{layer,path,read_xml(path)});
            // Nested monsters represent variants. Retain the ancestry rather
            // than flattening and guessing inheritance/override semantics.
            std::function<void(const XmlNode&,const XmlNode*)> add=[&](const XmlNode& node,const XmlNode* parent) {
                catalog.entries_.push_back({db.get(),&node,parent});
                if (node.tag=="monster")
                    for (const auto& child:node.children)
                        if (child.tag=="monster") add(child,&node);
            };
            for (const auto& node:db->root.children) add(node,nullptr);
            catalog.databases_.push_back(std::move(db));
        }
    }
    return catalog;
}
std::vector<DataEntry> Catalog::find(std::string_view kind,std::string_view name) const {
    std::vector<DataEntry> result;
    for (const auto& entry:entries_)
        if (entry.node->tag==kind && entry.node->attribute("name")==name) result.push_back(entry);
    return result;
}
}
