#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dredmor {
struct XmlNode {
    std::string tag;
    std::map<std::string,std::string,std::less<>> attributes;
    std::string text;
    std::vector<XmlNode> children;
    // Expat source order, for callbacks whose case-insensitive aliases assign
    // the same field. Values remain authoritative in attributes.
    std::vector<std::string> attribute_order;
    const XmlNode* first(std::string_view name) const;
    std::string attribute(std::string_view name, std::string fallback={}) const;
    std::optional<int> integer(std::string_view name) const;
};
// All attributes, text and node ordering are retained; no inferred gameplay
// defaults or expansion overrides are applied by the syntax layer.
XmlNode parse_xml(std::string_view content, std::string_view source="memory");
XmlNode read_xml(const std::filesystem::path& path);
}
