#include "dredmor/monster_sprites.hpp"
#include <iostream>
#include <stdexcept>
#define check(...) do {if (!(__VA_ARGS__)) throw std::runtime_error("monster sprite assertion at line "+std::to_string(__LINE__));} while(false)
using namespace dredmor;
int main() {
    try {
        const auto parent=read_monster(parse_xml(R"(<monster name="parent">
            <idleSprite left="L" right="R" up="U" down="D"/>
            <attackSprite left="AL" right="AR" up="AU" down="AD"/>
            <hitSprite left="HL" right="HR" up="HU" down="HD"/>
            <beamSprite left="BL" right="BR" up="BU" down="BD"/>
            <dieSprite name="die"/><castSpellSprite name="cast"/>
            <digSprites upSprite="dig up" downSprite="dig down"/>
        </monster>)"));
        const auto child=read_monster(parse_xml(R"(<monster name="child">
            <IDLESPRITE left="first" LEFT="last"/><idleSprite down="down"/>
            <attackSprite left=""/><hitSprite left=""/><beamSprite left=""/>
            <digSprites downSprite=""/><dieSprite name=""/>
        </monster>)"),&parent);
        check(child.idle_sprites==std::array<std::string,4>({"last","R","U","down"}));
        check(child.attack_sprites[0].empty() && child.attack_sprites[1]=="AR" && child.cast_sprite=="cast");
        check(parent.horde);
        auto limited=read_monster(parse_xml(R"(<monster name="limited" horde="1" HORDE="0" maxspawns="2"/>)"));
        MonsterSpawnState counters;
        check(!limited.horde && !can_spawn_monster(limited,0,counters,true) && can_spawn_monster(limited,0,counters));
        limited.horde=true;counters.counts[limited.name]=2;check(!can_spawn_monster(limited,0,counters,true));
        counters.counts[limited.name]=INT32_MAX;check(can_spawn_monster(limited,0,counters,true));
        check(!can_spawn_monster(limited,-1,counters,true));
        limited.max_spawns=0;check(can_spawn_monster(limited,-1,counters,true));
        MonsterSpriteState state;state.hit={91,92,93,94};state.attack.fill(42);state.beam.fill(43);
        state.dig_up=state.dig_down=44;state.die=45;state.current=46;state.frame=12;state.phase=13;state.animation_flag=7;
        DarwinRandom random(1);std::vector<std::string> paths;std::vector<int> angles;
        auto load=[&](const auto& path,int angle) {paths.push_back(path);angles.push_back(angle);return 999+paths.size();};
        check(tint_monster_sprites(child,state,120,random,load)==120 && random.state()==1);
        check(paths==std::vector<std::string>({"R","last","U","down","cast"}));
        check(state.hit==std::array<MonsterSpriteHandle,4>({91,92,93,94}));
        check(state.attack==std::array<MonsterSpriteHandle,4>{} && state.beam==std::array<MonsterSpriteHandle,4>{});
        check(state.dig_up==0 && state.dig_down==0 && state.die==0 && state.cast==1004);
        check(state.current==1003 && state.frame==0 && state.phase==0 && state.animation_flag==0);
        // Matching down sprite leaves all animation counters untouched.
        state.frame=42;state.phase=43;state.animation_flag=9;paths.clear();
        tint_monster_sprites(child,state,0,random,load);
        check(state.current==1003 && state.frame==42 && state.phase==43 && state.animation_flag==9);
        // A preserved current sprite stays selected even as every handle changes.
        state.preserve_current=true;state.current=8765;paths.clear();angles.clear();
        DarwinRandom expected(1);const auto angle=std::int32_t((std::uint64_t(expected.next())*0x59ffff5dull)>>53);
        check(tint_monster_sprites(parent,state,-1,random,load)==angle && random.state()==expected.state());
        check(state.current==8765 && state.frame==42 && state.animation_flag==9 && paths.size()==20);
        for (auto value:angles) check(value==angle);
        check(paths==std::vector<std::string>({"R","L","U","D","AR","AL","AU","AD","HR","HL","HU","HD","BR","BL","BU","BD","cast","die","dig up","dig down"}));
        // Empty idle strings still issue four mandatory load requests.
        MonsterDefinition empty;paths.clear();tint_monster_sprites(empty,state,1,random,load);check(paths==std::vector<std::string>(4));
        auto variant=read_monster(parse_xml(R"(<monster name="variant"><palette name="first" NAME="colours.pal" tint="120" TINT="0"/></monster>)"),&parent);
        check(variant.palette=="colours.pal" && variant.palette_tint==0);
        auto random_variant=read_monster(parse_xml(R"(<monster name="random"><palette tint="-1"/></monster>)"),&variant);
        check(random_variant.palette=="colours.pal" && random_variant.palette_tint==-1);
        // Normal loading uses left before right after the idle group and
        // includes the palette in its cache key. Death selects its last frame.
        state=MonsterSpriteState{};paths.clear();const auto before=random.state();
        const auto normal=rebuild_monster_sprites(variant,state,0,random,[&](const auto& key,const auto& path,const auto& palette) {
            check(palette=="colours.pal" && key==path+palette);paths.push_back(path);return 999+paths.size();
        },{},[](auto) {return std::uint16_t(6);});
        check(!normal && random.state()==before && !state.playing && state.frame==5 && state.current==state.die && state.die==1017);
        check(paths==std::vector<std::string>({"R","L","U","D","AL","AR","AU","AD","HL","HR","HU","HD","BL","BR","BU","BD","cast","die","dig up","dig down"}));
        state.current=777;state.preserve_current=true;state.frame=3;state.phase=99;
        rebuild_monster_sprites(variant,state,-1,random,[](const auto&,const auto&,const auto&) {return 1;},{},[](auto) {return std::uint16_t(0);});
        check(state.current==777 && state.frame==UINT32_MAX && state.phase==99 && !state.playing);
        paths.clear();angles.clear();DarwinRandom reference(before);
        const auto chosen=std::int32_t((std::uint64_t(reference.next())*0x59ffff5dull)>>53);
        check(rebuild_monster_sprites(random_variant,state,1,random,{},load,{})==chosen);
        check(random.state()==reference.state() && paths.size()==20);
        for (auto value:angles) check(value==chosen);
        std::cout<<"monster sprite XML inheritance/aliases, tint RNG, resource order, optional clearing/retention and animation reset passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
