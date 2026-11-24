#include "dredmor/factory.hpp"
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("shop stock assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
bool same(const GameRandom& a,const GameRandom& b) {
    return a.libc.state()==b.libc.state() && a.mt.state().index==b.mt.state().index && a.mt.state().words==b.mt.state().words && a.normal.state().available==b.normal.state().available && a.normal.state().spare==b.normal.state().spare;
}
int main() {
    try {
        std::vector<ItemDefinition> definitions;
        for (auto xml:{"<item name='Sword' type='0' level='1'><weapon/></item>","<item name='Unique' type='0' level='1' artifact='1'><weapon/></item>","<item name='Bolt' type='6' level='1' maxstack='3'><weapon/></item>","<item name='Ring' level='1'><armour type='ring'/></item>","<item name='Cap' level='1'><armour type='head'/></item>","<item name='Craft' craftoutput='1'><potion/></item>","<item name='Special' special='1'><potion/></item>","<item name='Debug' debug='1'><potion/></item>"}) definitions.push_back(read_item(parse_xml(xml)));
        ItemFactoryTables tables;for (std::size_t i=0;i<definitions.size();++i) register_factory_item(tables,std::uint32_t(i),definitions[i],15);
        const auto words=read_artifact_words(parse_xml("<text><startPhoneme text='A'/><phoneme text='b'/><title text='Keeper'/><noun text='Thing' plural='Things'/><adjective text='Bright'/><quality text='polished'/><material text='tin'/><decor text='dots'/></text>"));
        ItemFactoryContext context;context.artifact_words=&words;context.depth.depth=3;
        GameRandom random;random.seed(1);auto expected=random;const auto variant=expected.libc.next()>>29;ItemFactoryState state;
        auto empty=make_shop_inventory(definitions,tables,context,0,0,random,state);check(empty.items.empty() && empty.keeper_variant==variant && same(random,expected));
        bool craft=false,ammo=false,enchanted=false,unique=false,failed_after_first=false;std::set<int> variants;
        for (std::uint32_t seed=0;seed<128;++seed) {
            random.seed(seed*2654435761U);state={};const auto before=random;const auto plan=make_shop_inventory(definitions,tables,context,16,100,random,state);variants.insert(plan.keeper_variant);
            for (std::size_t i=0;i<plan.items.size();++i) {
                const auto& item=plan.items[i];check(item.instance==100+i && item.definition<6 && item.for_sale);
                craft=craft || item.definition==5;
                if (item.definition==2) {ammo=true;check(item.stackable && item.quantity>=2 && item.quantity<=3);}
                if (item.definition==3) {enchanted=true;check(item.artifact && item.artifact->quality>=1);}
                if (state.spawned_artifacts.count(1)) unique=true;
            }
            // A later missing word table must roll back earlier successful stock
            // and unique reservations, not just the single failing constructor.
            if (!plan.items[0].artifact && plan.items.back().artifact) {
                auto pending=before;ItemFactoryState pending_state;pending_state.spawned_artifacts.insert(77);auto missing=context;missing.artifact_words=nullptr;bool rejected=false;
                try {make_shop_inventory(definitions,tables,missing,16,100,pending,pending_state);} catch(const std::invalid_argument&) {rejected=true;}
                check(rejected && same(pending,before) && pending_state.spawned_artifacts==std::set<std::uint32_t>{77});failed_after_first=true;
            }
        }
        check(craft && ammo && enchanted && unique && failed_after_first && variants.size()==4);
        const auto before=random;const auto reserved=state.spawned_artifacts;bool rejected=false;
        try {make_shop_inventory(definitions,tables,context,2,std::numeric_limits<std::uint64_t>::max(),random,state);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && same(random,before) && state.spawned_artifacts==reserved);
        // Deterministic continuation: a batch and the same individual stock calls
        // after the keeper draw produce identical identities and all RNG state.
        random.seed(101);auto sequential=random;state={};ItemFactoryState other;const auto batch=make_shop_inventory(definitions,tables,context,20,1,random,state);
        check(batch.keeper_variant==(sequential.libc.next()>>29));
        for (std::size_t i=0;i<batch.items.size();++i) {const auto item=make_shop_stock(definitions,tables,context,i+1,sequential,other);check(item.definition==batch.items[i].definition && item.quantity==batch.items[i].quantity && bool(item.artifact)==bool(batch.items[i].artifact));}
        check(same(random,sequential) && state.spawned_artifacts==other.spawned_artifacts);
        std::cout<<"shop variant/stock ordering, craft acceptance, rejection, ammo caps, unique state, automatic accessories and batch rollback passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
