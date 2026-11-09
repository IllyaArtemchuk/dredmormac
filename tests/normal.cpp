#include "dredmor/random.hpp"
#include "dredmor/selection.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#define check(value) do { if (!(value)) throw std::runtime_error("normal assertion at line "+std::to_string(__LINE__)); } while(false)
std::uint32_t bits(float value) {std::uint32_t result;std::memcpy(&result,&value,4);return result;}
int main() {
    try {
        using namespace dredmor;Random random(0);NormalRandom normal;
        // Original 0x85d70 executions with a host double-log boundary.
        const std::uint32_t expected[]={0x3f94dfbe,0x400d94ca,0x3ef7b53d,0x3f46251b,0x3e996064,0x3f858f75,0x3e1cb2af,0x3f978add};
        for (std::size_t i=0;i<8;++i) {check(bits(normal.sample(0,1,random))==expected[i]);check(normal.state().available==(i%2==0));}
        check(random.u32()==1819583497U);
        random.seed(0);normal.restore({});check(bits(normal.sample(0,1,random))==expected[0]);
        random.seed(12345);check(bits(normal.sample(0,1,random))==expected[1]);check(random.u32()==3992670690U);
        normal.restore({true,1.25f});auto before=random.state();check(normal.sample(2,4,random,0)==7);
        check(!normal.state().available && normal.state().spare==1.25f && before.words==random.state().words && before.index==random.state().index);
        bool rejected=false;try {normal.sample(0,1,random,0);} catch(const std::runtime_error&) {rejected=true;}
        check(rejected && !normal.state().available && normal.state().spare==1.25f && before.words==random.state().words && before.index==random.state().index);
        GameRandom streams;streams.seed(0);streams.normal.sample(0,1,streams.mt);streams.seed(12345);
        check(bits(streams.normal.sample(0,1,streams.mt))==expected[1] && streams.mt.u32()==3992670690U);
        streams={};streams.seed(0);auto selected=select_trap({{2,false,true}},streams,{0,false,-1},1.4f);
        check(selected.index==0 && selected.target_level==2 && selected.attempts==1 && streams.libc.state()==28925691U);
        check(streams.normal.state().available && bits(streams.normal.state().spare)==0x400d94ca && streams.mt.u32()==3071714933U);
        streams={};streams.seed(0);selected=select_trap({{0,true,true}},streams,{0,false,-1},1.4f);
        check(selected.target_level==2 && selected.attempts==101 && streams.libc.state()==629636832U);
        streams={};streams.seed(0);selected=select_trap({{4,false,true}},streams,{0,true,-1},0);
        check(selected.target_level==4 && selected.attempts==1 && streams.libc.state()==822784415U && streams.normal.state().available);
        streams={};streams.seed(0);selected=select_trap({{5,false,true}},streams,{0,true,5},0);
        check(selected.target_level==5 && !streams.normal.state().available && streams.libc.state()==28925691U);
        auto unchanged=streams;rejected=false;
        try {select_trap({},streams,{0,false,-1},1.4f);} catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && streams.libc.state()==unchanged.libc.state() && streams.mt.state().words==unchanged.mt.state().words);
        check(streams.mt.state().index==unchanged.mt.state().index && streams.normal.state().available==unchanged.normal.state().available && bits(streams.normal.state().spare)==bits(unchanged.normal.state().spare));
        std::cout<<"original normal goldens, cached samples across reseeds, restore and atomic resource guards passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
