#pragma once
#include "dredmor/xml.hpp"
#include <memory>

namespace dredmor {
struct Database {
    std::string layer;
    std::filesystem::path source;
    XmlNode root;
};
struct DataEntry {
    const Database* database;
    const XmlNode* node;
    const XmlNode* parent_variant;
};
class Catalog {
public:
    // Explicit content selection; base only by default. Parsing does not decide
    // which duplicate/overriding definitions the game should use.
    static Catalog load(const std::filesystem::path& data_root,bool expansions=false);
    const std::vector<std::unique_ptr<Database>>& databases() const { return databases_; }
    const std::vector<DataEntry>& entries() const { return entries_; }
    std::vector<DataEntry> find(std::string_view kind,std::string_view name) const;
private:
    std::vector<std::unique_ptr<Database>> databases_;
    std::vector<DataEntry> entries_;
};
}
