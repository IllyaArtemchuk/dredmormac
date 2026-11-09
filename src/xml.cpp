#include "dredmor/xml.hpp"
#include <expat.h>
#include <charconv>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace dredmor {
const XmlNode* XmlNode::first(std::string_view name) const {
    for (const auto& child:children) if (child.tag==name) return &child;
    return nullptr;
}
std::string XmlNode::attribute(std::string_view name,std::string fallback) const {
    auto found=attributes.find(name);
    return found==attributes.end() ? fallback : found->second;
}
std::optional<int> XmlNode::integer(std::string_view name) const {
    auto found=attributes.find(name);
    if (found==attributes.end()) return std::nullopt;
    int value;
    const auto& s=found->second;
    auto parsed=std::from_chars(s.data(),s.data()+s.size(),value);
    if (parsed.ec!=std::errc() || parsed.ptr!=s.data()+s.size())
        throw std::runtime_error("invalid integer attribute "+std::string(name)+" in "+tag);
    return value;
}

namespace {
constexpr std::size_t max_bytes=64*1024*1024;
struct Parser {
    XML_Parser handle=XML_ParserCreate(nullptr);
    XmlNode root;
    std::vector<XmlNode*> stack;
    std::exception_ptr error;
    std::size_t nodes=0;
    Parser() { if (!handle) throw std::bad_alloc(); }
    ~Parser() { XML_ParserFree(handle); }
    template<class Function> void guarded(Function function) noexcept {
        if (error) return;
        try { function(); }
        catch (...) { error=std::current_exception(); XML_StopParser(handle,XML_FALSE); }
    }
    static void XMLCALL start(void* context,const XML_Char* name,const XML_Char** attrs) noexcept {
        auto& p=*static_cast<Parser*>(context);
        p.guarded([&] {
            if (p.stack.size()>=128 || ++p.nodes>200000)
                throw std::runtime_error("XML structural limit exceeded");
            XmlNode* node;
            if (p.stack.empty()) node=&p.root;
            else { p.stack.back()->children.emplace_back(); node=&p.stack.back()->children.back(); }
            node->tag=name;
            for (;*attrs;attrs+=2) node->attributes.emplace(attrs[0],attrs[1]);
            p.stack.push_back(node);
        });
    }
    static void XMLCALL end(void* context,const XML_Char*) noexcept {
        auto& p=*static_cast<Parser*>(context);
        p.guarded([&] { p.stack.pop_back(); });
    }
    static void XMLCALL text(void* context,const XML_Char* value,int size) noexcept {
        auto& p=*static_cast<Parser*>(context);
        p.guarded([&] {
            if (!p.stack.empty()) p.stack.back()->text.append(value,static_cast<std::size_t>(size));
        });
    }
    static void XMLCALL doctype(void* context,const XML_Char*,const XML_Char*,const XML_Char*,int) noexcept {
        auto& p=*static_cast<Parser*>(context);
        p.guarded([] { throw std::runtime_error("XML DTDs and external entities are unsupported"); });
    }
};
}
XmlNode parse_xml(std::string_view content,std::string_view source) {
    if (content.size()>max_bytes) throw std::runtime_error("XML exceeds 64 MiB: "+std::string(source));
    Parser parser;
    XML_SetUserData(parser.handle,&parser);
    XML_SetElementHandler(parser.handle,Parser::start,Parser::end);
    XML_SetCharacterDataHandler(parser.handle,Parser::text);
    XML_SetStartDoctypeDeclHandler(parser.handle,Parser::doctype);
    XML_SetParamEntityParsing(parser.handle,XML_PARAM_ENTITY_PARSING_NEVER);
    const auto status=XML_Parse(parser.handle,content.data(),static_cast<int>(content.size()),XML_TRUE);
    if (parser.error) std::rethrow_exception(parser.error);
    if (status!=XML_STATUS_OK) {
#if XML_MAJOR_VERSION > 2 || (XML_MAJOR_VERSION == 2 && XML_MINOR_VERSION >= 9)
        const auto line=XML_GetCurrentLineNumber64(parser.handle);
#else
        const auto line=XML_GetCurrentLineNumber(parser.handle);
#endif
        throw std::runtime_error(std::string(source)+":"+std::to_string(line)+
                                 ": "+XML_ErrorString(XML_GetErrorCode(parser.handle)));
    }
    return std::move(parser.root);
}
XmlNode read_xml(const std::filesystem::path& path) {
    const auto size=std::filesystem::file_size(path);
    if (size>max_bytes) throw std::runtime_error("XML exceeds 64 MiB: "+path.string());
    std::ifstream stream(path,std::ios::binary);
    if (!stream) throw std::runtime_error("cannot open XML: "+path.string());
    std::string content(static_cast<std::size_t>(size),'\0');
    if (!stream.read(content.data(),static_cast<std::streamsize>(size)))
        throw std::runtime_error("cannot read XML: "+path.string());
    return parse_xml(content,path.string());
}
}
