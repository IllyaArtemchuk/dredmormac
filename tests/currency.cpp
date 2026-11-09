#include "dredmor/currency.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
#define check(value) do { if (!(value)) throw std::runtime_error("currency assertion at line "+std::to_string(__LINE__)); } while(false)
int main() {
    try {
        using namespace dredmor;
        auto first=read_ability(parse_xml("<ability name='Fixture bonus' skill='2' level='0'><zorkmidbuff percent='.1'/></ability>"));
        auto second=read_ability(parse_xml("<ability name='Fixture second' skill='2' level='1'><ZorkmidBuff percentage='.25'/></ability>"));
        auto object=make_currency(100,{{2,2}},{first,second});
        check(object.kind==ItemKind::currency && object.value==137 && object.quantity==1 && !object.stackable);
        check(object.sprite_key=="cash3" && object.sprite=="items/cash2.png");
        currency_quantity_override(object,20);check(object.value==137 && object.quantity==20 && object.stackable);
        check(make_currency(100,{{2,0},{2,2}},{first}).value==100); // first matching selected skill
        std::vector<SkillRank> selected;for (int i=0;i<7;++i) selected.push_back({i,1});selected.push_back({99,2});
        first.skill_id=99;check(make_currency(100,selected,{first}).value==100);
        const int values[]={9,10,49,50,249,250,999,1000,3999,4000};
        const int images[]={0,1,1,2,2,3,3,4,4,5};
        for (std::size_t i=0;i<10;++i) check(make_currency(values[i]).sprite=="items/cash"+std::to_string(images[i])+".png");
        DarwinRandom random(1);check(roll_currency({-1,false,-1},random)==1 && random.state()==1);
        check(roll_currency({0,false,-1},random)==2 && random.state()==16807);
        random.seed(1);check(roll_currency({0,true,3},random)==8 && random.state()==16807);
        random.seed(1);check(roll_named_currency(0,1,random)==5 && random.state()==16807);
        random.seed(1);check(roll_named_currency(-1,10,random)==5 && random.state()==1);
        auto transfer=transfer_currency(2147483647,1,CurrencyOperation::give);
        check(transfer.balance==(-2147483647-1) && transfer.delta==1 && transfer.money_sound);
        transfer=transfer_currency(0,5,CurrencyOperation::take);check(transfer.balance==-5 && transfer.delta==-5 && !transfer.money_sound);
        transfer=transfer_currency(10,-3,CurrencyOperation::take);check(transfer.balance==13 && transfer.delta==3 && transfer.money_sound);
        transfer=transfer_currency(10,-3,CurrencyOperation::give);check(transfer.balance==7 && transfer.delta==-3 && !transfer.money_sound);
        check(transfer_currency(0,0,CurrencyOperation::give).money_sound && !transfer_currency(0,0,CurrencyOperation::take).money_sound);
        bool rejected=false;try {transfer_currency(0,std::numeric_limits<std::int32_t>::min(),CurrencyOperation::give);} catch(const std::overflow_error&) {rejected=true;}
        check(rejected);
        std::cout<<"currency eligibility/order/rounding, value versus quantity, sprite thresholds, rolls and wallet wrap passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
