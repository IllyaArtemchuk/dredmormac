#include "dredmor/catalog.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

void check(bool value) { if (!value) throw std::runtime_error("data assertion failed"); }
int main(int argc,char** argv) {
    try {
        check(argc==2);
        auto node=dredmor::parse_xml("<db><thing name='A &amp; B' n='-4'>hello&#10;<effect amount='2'/></thing></db>");
        check(node.tag=="db" && node.children.size()==1);
        const auto* thing=node.first("thing");
        check(thing && thing->attribute("name")=="A & B");
        check(thing->text=="hello\n" && thing->integer("n")==-4);
        check(!thing->integer("absent") && thing->first("effect")->integer("amount")==2);
        for (const auto& invalid: {"<db>","<!DOCTYPE db [<!ENTITY x 'expanded'>]><db>&x;</db>","<db/><other/>"}) {
            bool rejected=false;
            try { dredmor::parse_xml(invalid); } catch (const std::runtime_error&) { rejected=true; }
            check(rejected);
        }
        bool rejected=false;
        try { dredmor::parse_xml("<db n='1.2'/>").integer("n"); }
        catch (const std::runtime_error&) { rejected=true; }
        check(rejected);
        auto catalog=dredmor::Catalog::load(argv[1]);
        check(catalog.databases().size()==2);
        check(catalog.find("item","Fixture item").size()==1);
        auto variants=catalog.find("monster","Fixture variant");
        check(variants.size()==1 && variants[0].parent_variant);
        check(variants[0].parent_variant->attribute("name")=="Fixture monster");
        auto moved=std::move(catalog);
        check(moved.find("monster","Fixture variant")[0].node->first("stats")->integer("xpValue")==7);
        std::cout << "XML and catalog: syntax, limits, entities, variants and ownership passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n';return 1; }
}
