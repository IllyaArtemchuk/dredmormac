#include "dredmor/session.hpp"
#include "dredmor/monsters.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#define check(v) do {if (!(v)) throw std::runtime_error("treasure assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
std::string saved(const Session& session) {std::ostringstream out;session.save(out);return out.str();}
int main() {
    try {
        auto definitions=std::make_shared<std::vector<ItemDefinition>>();
        for (auto xml:{"<item name='Sword' level='1' type='0'><weapon/></item>","<item name='Thrown' level='1' type='5' maxstack='2'><weapon/></item>","<item name='Bolt' level='1' type='6' maxstack='2'><weapon/></item>","<item name='Head' level='1'><armour type='head'/></item>","<item name='Ring' level='1'><armour type='ring'/></item>","<item name='Legs' level='1'><armour type='legs'/></item>","<item name='Potion'><potion/></item>","<item name='Wand'><wand mincharge='3' maxcharge='7'/></item>","<item name='Food' level='1'><food hp='2'/></item>","<item name='Booze' level='1'><food mp='2'/></item>","<item name='Mushroom'><mushroom/></item>"}) definitions->push_back(read_item(parse_xml(xml)));
        // Isolate category selection from the separately tested constructor
        // enchantments, while retaining the real XML armour-type mapping.
        definitions->at(4).source.children[0].attributes["randoms"]="0";
        ItemDefinition lockpick;lockpick.name="Lockpick";lockpick.kind=ItemKind::misc;lockpick.builtin_subtype=0;definitions->push_back(lockpick);
        definitions->push_back(read_item(parse_xml("<item name='Neck' level='1'><armour type='neck' randoms='0'/></item>")));
        ItemFactoryTables tables;for (std::size_t i=0;i<definitions->size();++i) register_factory_item(tables,std::uint32_t(i),definitions->at(i),15);
        ItemFactoryContext context;TreasureOptions options;options.roll_size=1;GameRandom random;random.seed(1);
        // Native type6 is neck armour, while the legacy port slot field uses9.
        // Keep classifier semantics independent from the equipment-slot field.
        options.ranges[std::size_t(TreasureCategory::armour)]={0,0};
        for (int i=0;i<8;++i) {const auto item=make_level_treasure(*definitions,tables,context,options,false,i+1,random);check(item.definition==3 || item.definition==5);}
        options.ranges[std::size_t(TreasureCategory::armour)]={0,-1};options.ranges[std::size_t(TreasureCategory::ring)]={0,0};bool neck_seen=false;
        for (int i=0;i<32;++i) {const auto item=make_level_treasure(*definitions,tables,context,options,false,i+1,random);check(item.definition==4 || item.definition==12);neck_seen=neck_seen || item.definition==12;}check(neck_seen);
        options.ranges[std::size_t(TreasureCategory::ring)]={0,-1};options.ranges[std::size_t(TreasureCategory::ammo)]={0,0};
        for (int i=0;i<32;++i) {const auto item=make_level_treasure(*definitions,tables,context,options,false,i+1,random);check((item.definition==1 || item.definition==2) && item.quantity>=1 && item.quantity<=2 && item.stackable);}
        // Initial category draw happens even for a forced artifact, before the
        // missing-word-table failure. Failed work must not consume either RNG.
        const auto mt=random.mt.state();const auto libc=random.libc.state();const auto normal=random.normal.state();bool rejected=false;
        try {make_level_treasure(*definitions,tables,context,options,true,99,random);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && random.mt.state().index==mt.index && random.mt.state().words==mt.words && random.libc.state()==libc && random.normal.state().available==normal.available && random.normal.state().spare==normal.spare);
        std::istringstream map("#####\n#@..#\n#...#\n#####\n");Session session(World::read(map),make_player({3,2,2}),77);session.equipment_profile({3,2,2},definitions);
        options.ranges[std::size_t(TreasureCategory::ammo)]={0,-1};options.ranges[std::size_t(TreasureCategory::wand)]={0,0};
        session.create_ground_treasure(tables,context,options,false,{2,1});check(session.ground_items().back().item.charges && *session.ground_items().back().item.charges>=3 && *session.ground_items().back().item.charges<7);
        const auto before=saved(session);rejected=false;try {session.create_ground_treasure(tables,context,options,false,{0,0});} catch(const std::invalid_argument&) {rejected=true;}check(rejected && saved(session)==before);
        std::istringstream input(before);auto restored=Session::load(input,definitions);
        for (int i=0;i<4;++i) {session.create_ground_treasure(tables,context,options,false,{2,1});restored.create_ground_treasure(tables,context,options,false,{2,1});check(saved(session)==saved(restored));}
        // A pool containing only accessories cannot satisfy ordinary armour.
        ItemFactoryTables bad;bad.named[0]={4};options.ranges[std::size_t(TreasureCategory::wand)]={0,-1};options.ranges[std::size_t(TreasureCategory::armour)]={0,0};options.max_retries=2;
        const auto pending=saved(session);rejected=false;try {session.create_ground_treasure(bad,context,options,false,{2,1});} catch(const std::runtime_error&) {rejected=true;}check(rejected && saved(session)==pending);
        const auto tweaks=Tweaks::read(parse_xml("<tweaks><tweak name='treasure roll size' ival='690'/><tweak name='ammo roll min' ival='270'/><tweak name='ammo roll max' ival='349'/></tweaks>"));
        const auto resolved=treasure_options(tweaks,Difficulty::medium);check(resolved.roll_size==690 && resolved.ranges[std::size_t(TreasureCategory::ammo)].minimum==270 && resolved.ranges[std::size_t(TreasureCategory::ammo)].maximum==349);
        std::cout<<"level treasure categories, accessory classification, stack caps, wand charges, transactional ground creation and save continuation passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
