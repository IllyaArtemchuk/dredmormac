#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "InventoryController.h"
#import "SkillSelectionController.h"
#include <dispatch/dispatch.h>
#include <cstring>
#include <cstdlib>
#include "dredmor/content.hpp"
#include "dredmor/character.hpp"
#include "dredmor/floors.hpp"
#include "dredmor/sprite.hpp"
#include "dredmor/resources.hpp"
#include "dredmor/session_room_scripts.hpp"
#include <memory>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <climits>
#include <type_traits>

static std::filesystem::path roomResource(const std::filesystem::path& root,const std::string& relative) {
    if (root.empty() || relative.empty()) throw std::runtime_error("room object resource needs a data root/path");
    return dredmor::resolve_directory_resource(root,relative);
}

@interface SpriteImage : NSObject {
    dredmor::Sprite _sprite;
    NSArray<NSImage*> *_images;
    uint64_t _seriesDelay;
}
- (instancetype)initWithPath:(NSString*)path;
- (instancetype)initWithPath:(NSString*)path palette:(NSString*)palette tint:(int32_t)tint;
- (instancetype)initWithRoomSprite:(const dredmor::RoomSprite&)sprite root:(const std::filesystem::path&)root;
- (NSImage*)frameAt:(uint64_t)milliseconds;
- (NSImage*)frameAtIndex:(uint32_t)index;
- (NSUInteger)frameCount;
- (uint32_t)firstPixelRGB;
- (uint64_t)duration;
- (void)setSingleFrameDelay:(uint16_t)delay;
@end
@implementation SpriteImage
- (instancetype)initWithPath:(NSString*)path {
    return [self initWithPath:path palette:nil tint:-1];
}
- (instancetype)initWithPath:(NSString*)path palette:(NSString*)palette tint:(int32_t)tint {
    self=[super init];
    if (self) {
        if ([[path.pathExtension lowercaseString] isEqualToString:@"spr"]) {
            std::optional<dredmor::SpritePalette> colours;
            if (palette) colours=dredmor::read_sprite_palette(palette.UTF8String);
            _sprite=dredmor::Sprite::read(path.UTF8String,tint,colours?&*colours:nullptr);
            NSMutableArray<NSImage*> *images=[NSMutableArray array];
            for (auto& frame:_sprite.frames) {
                NSBitmapImageRep *bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr
                    pixelsWide:_sprite.width pixelsHigh:_sprite.height bitsPerSample:8 samplesPerPixel:4
                    hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
                    bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:_sprite.width*4 bitsPerPixel:32];
                if (!bitmap) throw std::runtime_error("cannot allocate sprite bitmap");
                std::memcpy(bitmap.bitmapData,frame.rgba.data(),frame.rgba.size());
                NSImage *image=[[NSImage alloc] initWithSize:NSMakeSize(_sprite.width,_sprite.height)];
                [image addRepresentation:bitmap];[images addObject:image];
                frame.rgba.clear();frame.rgba.shrink_to_fit();
            }
            _images=images;
        } else {
            if (palette || tint!=-1) throw std::runtime_error("monster colour conversion requires SPR resources");
            NSImage *image=[[NSImage alloc] initWithContentsOfFile:path];
            if (!image) throw std::runtime_error("cannot load scene image");
            _images=@[image];
        }
    }
    return self;
}
- (instancetype)initWithRoomSprite:(const dredmor::RoomSprite&)sprite root:(const std::filesystem::path&)root {
    if (sprite.series.empty()) {
        const auto path=roomResource(root,sprite.image);
        return [self initWithPath:[NSString stringWithUTF8String:path.c_str()]];
    }
    self=[super init];
    if (self) {
        if (sprite.first<0 || sprite.count<1 || sprite.count>1000 || sprite.first>INT_MAX-sprite.count || sprite.rate<0)
            throw std::runtime_error("room object PNG series exceeds supported bounds");
        NSMutableArray<NSImage*> *images=[NSMutableArray array];
        for (int i=0;i<sprite.count;++i) {
            std::ostringstream relative;relative<<sprite.series<<std::setw(4)<<std::setfill('0')<<sprite.first+i<<".png";
            const auto path=roomResource(root,relative.str());
            NSImage *image=[[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
            if (!image) throw std::runtime_error("cannot load room object PNG frame");
            [images addObject:image];
        }
        _images=images;_seriesDelay=sprite.rate;
    }
    return self;
}
- (NSImage*)frameAt:(uint64_t)milliseconds {
    if (!_sprite.frames.empty()) return _images[_sprite.frame_at(milliseconds)];
    return _images[_seriesDelay?(milliseconds/_seriesDelay)%_images.count:0];
}
- (NSImage*)frameAtIndex:(uint32_t)index { return _images[MIN(NSUInteger(index),_images.count-1)]; }
- (NSUInteger)frameCount { return _images.count; }
- (uint32_t)firstPixelRGB {
    if (!_images.count || !_sprite.width) throw std::runtime_error("sprite pixel inspection requires an SPR image");
    NSBitmapImageRep *bitmap=(NSBitmapImageRep*)_images[0].representations[0];
    const auto* rgba=bitmap.bitmapData;
    return (uint32_t(rgba[0])<<16)|(uint32_t(rgba[1])<<8)|rgba[2];
}
- (uint64_t)duration {
    if (_sprite.frames.empty()) return MAX(uint64_t(1),_seriesDelay)*_images.count;
    uint64_t result=0;for (const auto& frame:_sprite.frames) result+=MAX(uint64_t(1),uint64_t(frame.delay_ms));return result;
}
- (void)setSingleFrameDelay:(uint16_t)delay {
    if (_images.count!=1) throw std::runtime_error("single-frame delay requires one frame");
    if (_sprite.frames.empty()) _seriesDelay=delay;else _sprite.frames[0].delay_ms=delay;
}
@end

static SpriteImage *actorSprite(const dredmor::Actor& actor,const std::filesystem::path& root) {
    const auto path=roomResource(root,actor.sprite);
    NSString *palette=nil;
    if (!actor.sprite_palette.empty()) {
        const auto colours=roomResource(root,actor.sprite_palette);palette=[NSString stringWithUTF8String:colours.c_str()];
    }
    return [[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()] palette:palette tint:actor.sprite_tint];
}

static NSArray<SpriteImage*> *breakableSprites(const dredmor::RoomSprite& intact,const dredmor::RoomSprite& breaking,
    const dredmor::RoomSprite& broken,const std::filesystem::path& root) {
    NSMutableArray<SpriteImage*> *images=[NSMutableArray array];
    for (const auto* sprite:{&intact,&breaking,&broken}) {
        SpriteImage *image=[[SpriteImage alloc] initWithRoomSprite:*sprite root:root];
        if (!image || ![image frameCount] || [image frameCount]>65535) throw std::runtime_error("invalid breakable sprite frames");
        [images addObject:image];
    }
    return images;
}
static dredmor::BreakableFrames breakableFrames(NSArray<SpriteImage*> *images) {
    return {static_cast<uint16_t>([images[0] frameCount]),static_cast<uint16_t>([images[1] frameCount]),static_cast<uint16_t>([images[2] frameCount])};
}
static NSArray<SpriteImage*> *trapSprites(const dredmor::TrapInstance& trap,const dredmor::ItemDefinition& definition,const std::filesystem::path& root) {
    auto prepare=[&](const std::string& relative) {
        const auto path=roomResource(root,relative);SpriteImage *image=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
        if (!image || ![image frameCount]) throw std::runtime_error("cannot prepare trap sprite");return image;
    };
    SpriteImage *hidden=prepare("dungeon/trap_hidden.png"),*visible=prepare(definition.icon);
    SpriteImage *origin=trap.has_origin_object?prepare(definition.trap->origin):hidden;
    return @[hidden,visible,origin];
}

static std::int32_t sceneInteger(NSDictionary *node,NSString *key,std::int32_t fallback=0) {
    id value=node[key];if (!value) return fallback;
    if (![value isKindOfClass:NSNumber.class] || [value doubleValue]!=[value longLongValue] || [value longLongValue]<INT32_MIN || [value longLongValue]>INT32_MAX) throw std::runtime_error("invalid floor integer");
    return [value intValue];
}
static std::vector<dredmor::StairPlacement> sceneStairs(id value) {
    std::vector<dredmor::StairPlacement> stairs;if (!value) return stairs;
    if (![value isKindOfClass:NSArray.class] || [value count]>4096) throw std::runtime_error("invalid stair array");
    for (id node in value) {
        if (![node isKindOfClass:NSDictionary.class] || !node[@"x"] || !node[@"y"]) throw std::runtime_error("invalid stair coordinates");
        const auto up=sceneInteger(node,@"up"),link=sceneInteger(node,@"link");
        if (up<0 || up>1 || link<0 || link>255) throw std::runtime_error("invalid stair direction/link");
        stairs.push_back({{sceneInteger(node,@"x"),sceneInteger(node,@"y")},up!=0,std::uint8_t(link)});
    }
    return stairs;
}
static std::vector<dredmor::SecretLever> sceneSecretLevers(id value) {
    std::vector<dredmor::SecretLever> levers;if (!value) return levers;
    if (![value isKindOfClass:NSArray.class] || [value count]>4096) throw std::runtime_error("invalid secret lever array");
    for (id node in value) {
        if (![node isKindOfClass:NSDictionary.class] || !node[@"x"] || !node[@"y"] || !node[@"target_x"] || !node[@"target_y"]) throw std::runtime_error("invalid secret lever coordinates");
        const auto active=sceneInteger(node,@"activated");if (active<0 || active>1) throw std::runtime_error("invalid secret lever state");
        levers.push_back({{sceneInteger(node,@"x"),sceneInteger(node,@"y")},{sceneInteger(node,@"target_x"),sceneInteger(node,@"target_y")},active!=0});
    }
    return levers;
}

@interface RoomView : NSView<DredmorInventoryOwner> {
    std::unique_ptr<dredmor::Session> _session;
    dredmor::FloorRegistry _floors;
    BOOL _multiFloor;
    NSArray<SpriteImage*> *_stairImages;
    NSArray<SpriteImage*> *_leverImages;
    NSArray<SpriteImage*> *_scriptLeverImages;
    NSArray<NSDictionary*> *_shopImages;
    NSArray<NSArray<SpriteImage*>*> *_doorImages;
    std::optional<dredmor::SessionDoors> _doorBlueprint;
    NSArray<NSString*> *_doorBaseRows;
    std::unique_ptr<dredmor::GameContent> _content;
    std::filesystem::path _dataRoot;
    NSArray<NSString*> *_rows;
    NSImage *_atlas;
    SpriteImage *_hero;
    NSMutableDictionary<NSNumber*,SpriteImage*> *_enemyImages;
    NSArray<SpriteImage*> *_objectImages;
    NSArray<NSArray<SpriteImage*>*> *_breakableImages;
    NSArray<NSArray<SpriteImage*>*> *_trapImages;
    NSMutableArray<NSDictionary*> *_breakAnimations;
    NSTimer *_animationTimer;
    NSTimeInterval _animationStart;
    DredmorInventoryController *_inventoryController;
    std::optional<std::size_t> _targetSpell;
    std::optional<dredmor::SessionRoomScriptProfile> _roomScripts;
    std::optional<dredmor::SessionRoomObjects> _roomScriptObjects;
    dredmor::RoomRuntimeState _roomScriptRuntime;
    NSArray<NSString*> *_scriptMessages;
    NSRect _floorCrop,_wallCrop;
}
- (instancetype)initWithScene:(NSDictionary*)scene;
- (NSString*)snapshot;
- (BOOL)hasLivingEnemies;
- (BOOL)hasAnimatedHero;
- (void)saveToURL:(NSURL*)url;
- (void)loadFromURL:(NSURL*)url;
- (NSDictionary*)preparePresentation:(const dredmor::Session&)session;
- (void)applyPresentation:(NSDictionary*)presentation;
- (NSDictionary*)prepareRoomScripts:(dredmor::Session&)session objects:(dredmor::SessionRoomObjects&)objects runtime:(dredmor::RoomRuntimeState&)runtime;
- (void)commitCandidate:(dredmor::Session&)candidate refresh:(BOOL)refresh;
- (BOOL)smokeRoomScripts;
- (BOOL)smokeFloors;
- (BOOL)smokeSecretLevers;
- (BOOL)smokeScriptLevers;
- (BOOL)smokeDoors;
- (BOOL)smokeDecorations;
- (BOOL)smokeAutomaticArmour;
- (BOOL)smokeShops;
- (void)showShopPedestal:(NSUInteger)shop index:(NSUInteger)pedestal;
- (void)presentShopNotices;
- (void)showShopKeeper:(NSUInteger)index;
- (BOOL)smokeSaveLoad;
- (void)saveDocument:(id)sender;
- (void)openDocument:(id)sender;
- (void)showInventory:(id)sender;
- (void)showSpells:(id)sender;
- (NSMenu*)spellMenu;
- (BOOL)smokeCasting;
- (BOOL)smokeRegeneration;
- (BOOL)smokeFood;
- (void)selectSpell:(id)sender;
- (void)castSelectedSpellAt:(dredmor::Position)target;
- (void)pickUpItems:(id)sender;
- (BOOL)smokeInventory;
- (BOOL)smokeRoomMonsters;
- (BOOL)smokeHordes;
- (BOOL)hasScriptedHordes;
- (BOOL)smokeArtifact;
- (BOOL)smokeSkillSelection;
- (BOOL)smokeCustomObjects;
- (BOOL)smokeBreakables;
- (BOOL)smokeTraps;
- (void)recordBreakableAnimations;
@end

@implementation RoomView
- (instancetype)initWithScene:(NSDictionary*)scene {
    self = [super initWithFrame:NSMakeRect(0,0,600,650)];
    if (self) {
        std::istringstream input([scene[@"collision"] UTF8String]);
        auto world=dredmor::World::read(input);
        dredmor::ClassRanks ranks={3,2,2};
        std::uint32_t seed=0xD0ED;
        if (scene[@"seed"]) {
            if (![scene[@"seed"] isKindOfClass:NSNumber.class] || [scene[@"seed"] doubleValue]<0 ||
                [scene[@"seed"] doubleValue]>UINT32_MAX ||
                [scene[@"seed"] doubleValue]!=[scene[@"seed"] unsignedLongLongValue])
                throw std::runtime_error("scene seed must be a uint32 integer");
            seed=[scene[@"seed"] unsignedIntValue];
        }
        std::vector<int> selected;
        int experience=190;
        if ([scene[@"data_root"] isKindOfClass:NSString.class]) {
            _dataRoot=std::filesystem::canonical([scene[@"data_root"] UTF8String]);
            _content=std::make_unique<dredmor::GameContent>(dredmor::GameContent::load([scene[@"data_root"] UTF8String]));
            if ([scene[@"skills"] isKindOfClass:NSArray.class]) {
                for (id value in scene[@"skills"]) {
                    if (![value isKindOfClass:NSNumber.class]) throw std::runtime_error("invalid selected skill ID");
                    selected.push_back([value intValue]);
                }
            } else for (const auto& skill:_content->skills()) if (selected.size()<7) selected.push_back(skill.id);
            ranks=_content->starting_ranks(selected);
            experience=_content->tweaks().integer(dredmor::Difficulty::medium,"experience n");
        }
        if (_content && [scene[@"starting_loadout"] boolValue])
            _session=std::make_unique<dredmor::Session>(dredmor::create_character(*_content,std::move(world),selected,seed));
        else _session=std::make_unique<dredmor::Session>(std::move(world),dredmor::make_player(ranks),seed,experience);
        if (_content && ![scene[@"starting_loadout"] boolValue]) {
            std::vector<dredmor::SkillRank> skillRanks;for (auto id:selected) skillRanks.push_back({id,1});
            _session->character_profile(ranks,std::make_shared<const std::vector<dredmor::ItemDefinition>>(_content->items()),
                std::move(skillRanks),std::make_shared<const std::vector<dredmor::AbilityDefinition>>(_content->abilities()));
        }
        if (_content && !_session->regeneration() && _content->tweaks().contains(dredmor::Difficulty::medium,"hpregenturns") && _content->tweaks().contains(dredmor::Difficulty::medium,"spregenturns")) {
            bool vampire=false;for (const auto& skill:_content->skills()) if (skill.name=="Vampirism" && std::find(selected.begin(),selected.end(),skill.id)!=selected.end()) vampire=true;
            _session->regeneration_profile(_content->tweaks().integer(dredmor::Difficulty::medium,"hpregenturns"),_content->tweaks().integer(dredmor::Difficulty::medium,"spregenturns"),vampire);
        }
        if (_content) _session->spell_profile(std::make_shared<const std::vector<dredmor::SpellDefinition>>(_content->spells()));
        if (scene[@"dungeon_depth"]) {
            if (![scene[@"dungeon_depth"] isKindOfClass:NSNumber.class]) throw std::runtime_error("invalid dungeon depth");
            _session->depth_context({[scene[@"dungeon_depth"] intValue],false,-1});
        }
        if (scene[@"ordinary_doors"]) {
            if (!_content || ![scene[@"ordinary_doors"] isKindOfClass:NSString.class]) throw std::runtime_error("ordinary doors require a content room");
            bool burglary=false;for (const auto& skill:_content->skills()) if (skill.name=="Burglary" && std::find(selected.begin(),selected.end(),skill.id)!=selected.end()) burglary=true;
            _session->create_room_doors(_content->room([scene[@"ordinary_doors"] UTF8String]).numeric_template(),burglary,[scene[@"female"] boolValue],_content->tweaks().integer(dredmor::Difficulty::medium,"lockpick exp"));
            _doorBlueprint=*_session->doors();
        }
        NSMutableArray<NSArray<SpriteImage*>*> *breakables=[NSMutableArray array];
        if (scene[@"room_objects"]) {
            if (!_content || ![scene[@"room_objects"] isKindOfClass:NSString.class]) throw std::runtime_error("room objects need a content room name");
            const auto& room=_content->room([scene[@"room_objects"] UTF8String]);
            const auto numeric=room.numeric_template();dredmor::TrapMap trapMap{numeric.width,numeric.height,{}};
            for (const auto& cell:numeric.cells) trapMap.tiles.push_back(cell.kind);
            std::vector<dredmor::DungeonCell> hordeCells(numeric.cells.size());
            for (std::size_t i=0;i<hordeCells.size();++i) hordeCells[i].kind=numeric.cells[i].kind;
            dredmor::DungeonLayout hordeLayout(numeric.width,numeric.height,std::move(hordeCells));
            const std::vector<dredmor::HordeRoomBounds> hordeRooms={{{0,0},{numeric.width,numeric.height}}};
            if ([scene[@"room_scripts"] boolValue]) {
                auto program=std::make_shared<const dredmor::RoomScriptProgram>(_content->room_script_program([scene[@"room_objects"] UTF8String]));
                _roomScripts.emplace(hordeLayout);_roomScripts->programs={program};_roomScripts->sources={room.source};
                _roomScripts->rooms={{{true,1,0,std::shared_ptr<const std::vector<dredmor::RoomScriptDefinition>>(program,&program->scripts)},{0,0},1}};
                _roomScriptObjects.emplace();_scriptMessages=@[];
            }
            auto bind=[&](dredmor::SessionRoomObjectKind kind,std::uint64_t subject,bool finalize=true) {
                if (_roomScriptObjects && _roomScriptObjects->bind(*_session,{kind,subject},hordeLayout,finalize).duplicate)
                    throw std::runtime_error("duplicate scripted room object label");
            };
            // Preview builds supported objects/loot in relative source order.
            // Other constructors/RNG and full population still belong to level generation.
            for (const auto& record:room.records()) std::visit([&](const auto& definition) {
                using T=std::decay_t<decltype(definition)>;
                if constexpr (std::is_same_v<T,dredmor::RoomElementDefinition>) {if (definition.type=="lever") if (auto index=_session->create_script_lever(definition)) bind(dredmor::SessionRoomObjectKind::lever,*index+1);}
                else if constexpr (std::is_same_v<T,dredmor::CustomRoomDefinition>) {if (auto index=_session->create_custom_object(definition)) bind(dredmor::SessionRoomObjectKind::custom,*index+1);}
                else if constexpr (std::is_same_v<T,dredmor::RoomPedestalRecord>) {if (auto index=_session->create_pedestal(definition)) bind(dredmor::SessionRoomObjectKind::custom,*index+1);}
                else if constexpr (std::is_same_v<T,dredmor::RoomLootRecord>) {
                    const auto created=_session->create_room_loot(definition,_content->item_factory_tables(),_content->loot_context(_session->depth_context()));
                    if (created.instance) bind(dredmor::SessionRoomObjectKind::item,*created.instance);
                }
                else if constexpr (std::is_same_v<T,dredmor::RoomMonsterRecord>) {
                    auto created=_session->create_room_monster(definition,[&](const auto& plan,auto& random,auto& state) {
                        return _content->create_room_monster(plan,{_session->depth_context()},dredmor::Difficulty::medium,random,state);
                    });
                    if (created) bind(dredmor::SessionRoomObjectKind::monster,*created);
                }
                else if constexpr (std::is_same_v<T,dredmor::RoomHordeRecord>) {
                    _session->queue_room_horde(definition,hordeLayout,hordeRooms,[&](const auto& plan,const auto& world,const auto& counts,auto& random,auto first) {
                        dredmor::HordeOptions options;options.position=plan.position;options.minimum=plan.minimum;options.additional=plan.additional;
                        options.depth=_session->depth_context().depth;if (!plan.name.empty()) options.named=plan.name;
                        return _content->create_horde(options,world,dredmor::Difficulty::medium,_session->depth_context().depth,counts,random,first);
                    });
                }
                else if constexpr (std::is_same_v<T,dredmor::RoomTrapRecord>) {
                    const auto created=_session->create_room_trap(definition,_content->item_factory_tables(),_content->loot_context(_session->depth_context()),trapMap);
                    if (created.instance) bind(dredmor::SessionRoomObjectKind::trap,*created.instance);
                }
                else if constexpr (std::is_same_v<T,dredmor::BreakableRoomDefinition>) {
                    NSArray<SpriteImage*> *prepared=nil;
                    const auto created=_session->create_breakable(definition,{0,0},[&](const auto& value) {
                        prepared=breakableSprites(value.intact,value.breaking.value_or(value.intact),value.broken,_dataRoot);return breakableFrames(prepared);
                    });
                    if (created) {
                        bind(dredmor::SessionRoomObjectKind::breakable,*created+1);
                        const auto delay=_session->breakables()[*created].single_breaking_delay;
                        if (delay) [prepared[1] setSingleFrameDelay:delay];[breakables addObject:prepared];
                    }
                }
            },record);
            if (scene[@"room_subject"]) {
                if (![scene[@"room_subject"] isKindOfClass:NSString.class]) throw std::runtime_error("invalid room decoration subject");
                for (auto index:_session->create_room_decorations(numeric,{0,0},[scene[@"room_subject"] UTF8String],_content->room_decorations()))
                    bind(dredmor::SessionRoomObjectKind::custom,index+1,false);
            }
            if (numeric.shop) {
                std::vector<dredmor::Position> tiles;
                for (int y=0;y<numeric.height;++y) for (int x=0;x<numeric.width;++x) {
                    const auto kind=numeric.cells[std::size_t(y)*numeric.width+x].kind;if (kind==1 || kind==15) tiles.push_back({x,y});
                }
                _session->create_shop(numeric.shop_markers,std::move(tiles),_content->item_factory_tables(),_content->loot_context(_session->depth_context()),[&](dredmor::GameRandom& stream) {
                    return _content->create_monster("Brax",numeric.shop_markers.home,dredmor::Difficulty::medium,_session->depth_context().depth,stream.libc);
                });
                _session->update_shop_presence();
            }
            for (auto actor:_session->place_queued_hordes()) bind(dredmor::SessionRoomObjectKind::monster,actor,false);
            if (_roomScriptObjects) bind(dredmor::SessionRoomObjectKind::player,_session->player().id);
        }
        else if (scene[@"room_subject"] || [scene[@"room_scripts"] boolValue]) throw std::runtime_error("room decorations/scripts require a content room");
        if (_roomScripts && scene[@"floors"]) throw std::runtime_error("scripted prepared rooms do not yet support the native floor cache");
        _breakableImages=breakables;_breakAnimations=[NSMutableArray array];
        NSMutableArray<NSArray<SpriteImage*>*> *traps=[NSMutableArray array];
        for (const auto& trap:_session->traps()) [traps addObject:trapSprites(trap,_content->items().at(trap.definition),_dataRoot)];
        _trapImages=traps;
        NSMutableArray<SpriteImage*> *objects=[NSMutableArray array];
        for (const auto& object:_session->custom_objects()) {
            SpriteImage *image=[[SpriteImage alloc] initWithRoomSprite:object.sprite root:_dataRoot];
            if (!image) throw std::runtime_error("cannot prepare room object sprite");
            [objects addObject:image];
        }
        _objectImages=objects;
        _rows = scene[@"rows"];
        if (_rows.count != (NSUInteger)_session->world().height()) return nil;
        for (id row in _rows)
            if (![row isKindOfClass:NSString.class] || [row length] != (NSUInteger)_session->world().width()) return nil;
        _atlas = [[NSImage alloc] initWithContentsOfFile:scene[@"atlas"]];
        NSString *heroPath=[scene[@"hero_spr"] isKindOfClass:NSString.class]?scene[@"hero_spr"]:scene[@"hero"];
        _hero = [[SpriteImage alloc] initWithPath:heroPath];
        if (!_atlas || !_hero) return nil;
        _floorCrop=NSMakeRect(0,192,32,32);_wallCrop=NSMakeRect(0,96,32,32);
        for (NSString *key in @[@"floor_crop",@"wall_crop"]) {
            id value=scene[key];
            if ([value isKindOfClass:NSArray.class] && [value count]==4) {
                for (id number in value) if (![number isKindOfClass:NSNumber.class]) return nil;
                NSRect crop=NSMakeRect([value[0] doubleValue],[value[1] doubleValue],[value[2] doubleValue],[value[3] doubleValue]);
                if (crop.origin.x<0 || crop.origin.y<0 || crop.size.width<=0 || crop.size.height<=0 ||
                    NSMaxX(crop)>_atlas.size.width || NSMaxY(crop)>_atlas.size.height) return nil;
                if ([key isEqualToString:@"floor_crop"]) _floorCrop=crop;else _wallCrop=crop;
            }
        }
        _enemyImages=[NSMutableDictionary dictionary];
        for (const auto& enemy:_session->enemies()) {
            SpriteImage *image=actorSprite(enemy,_dataRoot);
            if (!image) throw std::runtime_error("cannot prepare room monster sprite");_enemyImages[@(enemy.id)]=image;
        }
        if (scene[@"enemies"] && ![scene[@"enemies"] isKindOfClass:NSArray.class]) return nil;
        for (id value in scene[@"enemies"]) {
            if (![value isKindOfClass:NSDictionary.class] || ![value[@"name"] isKindOfClass:NSString.class] ||
                ![value[@"image"] isKindOfClass:NSString.class] || ![value[@"x"] isKindOfClass:NSNumber.class] ||
                ![value[@"y"] isKindOfClass:NSNumber.class]) return nil;
            dredmor::Actor enemy;
            const dredmor::Position position{[value[@"x"] intValue],[value[@"y"] intValue]};
            if (_content) enemy=_content->make_monster([value[@"name"] UTF8String],position,dredmor::Difficulty::medium);
            else {
                enemy.name=[value[@"name"] UTF8String];enemy.position=position;
                enemy.health=enemy.secondary[0]=1;enemy.secondary[2]=1;
            }
            const auto palette=enemy.sprite_palette;const auto tint=enemy.sprite_tint;
            const auto identifier=_session->spawn_enemy(std::move(enemy));
            NSString *spritePath=[value[@"animation"] isKindOfClass:NSString.class]?value[@"animation"]:value[@"image"];
            NSString *palettePath=nil;
            if (!palette.empty()) {const auto path=roomResource(_dataRoot,palette);palettePath=[NSString stringWithUTF8String:path.c_str()];}
            SpriteImage *sprite=[[SpriteImage alloc] initWithPath:spritePath palette:palettePath tint:tint];
            if (!sprite) return nil;
            _enemyImages[@(identifier)]=sprite;
        }
        if (scene[@"ground_items"] && ![scene[@"ground_items"] isKindOfClass:NSArray.class]) return nil;
        for (id value in scene[@"ground_items"]) {
            if (!_content || ![value isKindOfClass:NSDictionary.class] || ![value[@"name"] isKindOfClass:NSString.class] ||
                ![value[@"x"] isKindOfClass:NSNumber.class] || ![value[@"y"] isKindOfClass:NSNumber.class])
                throw std::runtime_error("developer ground items require content and valid positions");
            const std::string name=[value[@"name"] UTF8String];
            std::int32_t quantity=1;
            if (value[@"quantity"]) {
                if (![value[@"quantity"] isKindOfClass:NSNumber.class]) throw std::runtime_error("invalid ground quantity");
                quantity=[value[@"quantity"] intValue];
            }
            dredmor::ItemFactoryContext context;
            context.artifact_words=&_content->artifact_words();
            if (value[@"artifact_quality"]) {
                if (![value[@"artifact_quality"] isKindOfClass:NSNumber.class]) throw std::runtime_error("invalid artifact quality");
                const auto quality=[value[@"artifact_quality"] longLongValue];
                if (quality<INT32_MIN || quality>INT32_MAX || [value[@"artifact_quality"] doubleValue]!=quality)
                    throw std::runtime_error("invalid artifact quality");
                std::optional<std::uint32_t> definition;
                for (const auto& pool:_content->item_factory_tables().named) {
                    for (const auto index:pool) if (_content->items()[index].name==name) {definition=index;break;}
                    if (definition) break;
                }
                if (!definition || quantity!=1) throw std::runtime_error("invalid artifact fixture item");
                _session->create_ground_artifact(*definition,static_cast<int>(quality),_content->artifact_words(),
                    {[value[@"x"] intValue],[value[@"y"] intValue]});continue;
            }
            if (name=="zorkmids") context.gold_per_level=_content->tweaks().integer(dredmor::Difficulty::medium,"gold drop per level");
            if (!_session->create_ground_loot(_content->item_factory_tables(),context,"named",name,quantity,false,
                {[value[@"x"] intValue],[value[@"y"] intValue]})) throw std::runtime_error("developer ground item not found");
        }
        if (scene[@"ground_currency"] && ![scene[@"ground_currency"] isKindOfClass:NSArray.class]) return nil;
        for (id value in scene[@"ground_currency"]) {
            if (!_content || ![value isKindOfClass:NSDictionary.class] ||
                ![value[@"amount"] isKindOfClass:NSNumber.class] ||
                ![value[@"x"] isKindOfClass:NSNumber.class] || ![value[@"y"] isKindOfClass:NSNumber.class])
                throw std::runtime_error("developer currency requires content, amount and position");
            const auto amount=[value[@"amount"] longLongValue];
            if (amount<INT32_MIN || amount>INT32_MAX || [value[@"amount"] doubleValue]!=amount)
                throw std::runtime_error("invalid ground currency amount");
            _session->create_ground_currency(static_cast<std::int32_t>(amount),
                {[value[@"x"] intValue],[value[@"y"] intValue]});
        }
        _session->floor_stairs(sceneStairs(scene[@"stairs"]),sceneInteger(scene,@"floor_branch"));
        _session->floor_secret_levers(sceneSecretLevers(scene[@"secret_levers"]));
        _multiFloor=scene[@"floors"]!=nil;
        if (_multiFloor) {
            if (!_content || ![scene[@"floors"] isKindOfClass:NSArray.class]) throw std::runtime_error("prepared floors require an installation and array");
            for (id node in scene[@"floors"]) {
                if (![node isKindOfClass:NSDictionary.class] || ![node[@"collision"] isKindOfClass:NSString.class] || !node[@"depth"]) throw std::runtime_error("invalid prepared floor");
                std::istringstream floorMap([node[@"collision"] UTF8String]);dredmor::SessionFloor floor(dredmor::World::read(floorMap));
                floor.depth.depth=sceneInteger(node,@"depth");floor.branch=sceneInteger(node,@"branch");floor.stairs=sceneStairs(node[@"stairs"]);
                floor.secret_levers=sceneSecretLevers(node[@"secret_levers"]);
                if (node[@"monsters"] && ![node[@"monsters"] isKindOfClass:NSArray.class]) throw std::runtime_error("invalid prepared monsters");
                for (id monster in node[@"monsters"]) {
                    if (![monster isKindOfClass:NSDictionary.class] || ![monster[@"name"] isKindOfClass:NSString.class]) throw std::runtime_error("invalid prepared monster");
                    floor.enemies.push_back(_content->make_monster([monster[@"name"] UTF8String],{sceneInteger(monster,@"x"),sceneInteger(monster,@"y")},dredmor::Difficulty::medium,floor.depth.depth));
                }
                _floors.add_floor(*_session,std::move(floor));
            }
        }
        _stairImages=@[];_leverImages=@[];_scriptLeverImages=@[];_shopImages=@[];_doorImages=@[];
        if (_multiFloor || !_session->floor_stairs().empty() || !_session->secret_levers().empty() || !_session->script_levers().empty() || !_session->shops().empty() || _session->doors()) {
            _floors.for_each_floor(*_session,[&](const dredmor::Session& floor){@autoreleasepool {(void)[self preparePresentation:floor];}});
            [self applyPresentation:[self preparePresentation:*_session]];
            if (_session->doors()) _doorBaseRows=_rows;
        }
        _animationStart=NSProcessInfo.processInfo.systemUptime;
        __weak RoomView *weakSelf=self;
        _animationTimer=[NSTimer scheduledTimerWithTimeInterval:1.0/60 repeats:YES block:^(NSTimer *timer) {
            RoomView *view=weakSelf;
            if (!view) { [timer invalidate];return; }
            view.needsDisplay=YES;
        }];
    }
    return self;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (void)dealloc { [_animationTimer invalidate];[_inventoryController close]; }
- (BOOL)isFlipped { return YES; }
- (NSString*)snapshot { return [NSString stringWithUTF8String:_session->world().snapshot().c_str()]; }
- (BOOL)hasLivingEnemies {
    for (const auto& enemy:_session->enemies()) if (enemy.alive()) return YES;
    return NO;
}
- (BOOL)hasAnimatedHero { return [_hero frameAt:0]!=[_hero frameAt:100]; }
- (BOOL)smokeCustomObjects {
    if (_session->custom_objects().size()!=2 || _objectImages.count!=2) return NO;
    const auto& solid=_session->custom_objects()[0];const auto& decoration=_session->custom_objects()[1];
    if (!solid.blocks_movement || decoration.blocks_movement || !_session->world().passable(solid.position) || _session->passable(solid.position)) return NO;
    if ([_objectImages[0] frameAt:0]==[_objectImages[0] frameAt:100] || [_objectImages[1] frameAt:0]==[_objectImages[1] frameAt:100]) return NO;
    std::ostringstream before;_session->save(before);_session->step(dredmor::Action::east);std::ostringstream after;_session->save(after);
    return before.str()==after.str() && !_session->events().empty() && _session->events()[0].kind==dredmor::EventKind::blocked;
}
- (BOOL)smokeBreakables {
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:
        [NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    try {
        if (_session->breakables().size()!=1 || _breakableImages.count!=1 || _session->custom_objects().size()!=1) return NO;
        const auto& object=_session->breakables()[0];
        if (object.hits_taken!=1 || object.object.frame!=1 || !_session->world().passable(object.object.position) || _session->passable(object.object.position) ||
            [_breakableImages[0][0] frameCount]!=3 || [_breakableImages[0][1] frameCount]!=1 || [_breakableImages[0][2] frameCount]!=4 || [_breakableImages[0][1] duration]!=10)
            return NO;
        std::ostringstream before;_session->save(before);NSArray *oldImages=_breakableImages;
        std::vector<std::string> invalid;
        auto path=before.str();auto start=path.find("<broken image=\"");
        if (start==std::string::npos) return NO;start+=std::string("<broken image=\"").size();const auto end=path.find('"',start);
        path.replace(start,end-start,"../unavailable-resource.spr");invalid.push_back(std::move(path));
        auto counts=before.str();start=counts.find("intact-frames=\"3\"");if (start==std::string::npos) return NO;
        counts.replace(start,std::string("intact-frames=\"3\"").size(),"intact-frames=\"4\"");invalid.push_back(std::move(counts));
        for (const auto& bytes:invalid) {
            NSData *data=[NSData dataWithBytes:bytes.data() length:bytes.size()];
            if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return NO;
            bool rejected=false;try { [self loadFromURL:url]; } catch(const std::exception&) {rejected=true;}
            std::ostringstream after;_session->save(after);if (!rejected || after.str()!=before.str() || oldImages!=_breakableImages) return NO;
        }
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
        if (![self smokeSaveLoad]) return NO;
        NSEvent *right=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
            windowNumber:self.window.windowNumber context:nil characters:@"\uf703" charactersIgnoringModifiers:@"\uf703" isARepeat:NO keyCode:124];
        [self keyDown:right];
        const auto& broken=_session->breakables()[0];
        if (broken.hits_taken!=2 || broken.object.frame!=3 || broken.object.playing || broken.object.looping ||
            !_session->passable(broken.object.position) || _breakAnimations.count!=1) return NO;
        [self display];[self keyDown:right];
        return _session->player().position.x==2 && _session->player().position.y==1 && !_session->passable({3,2});
    } catch (const std::exception& error) {
        fprintf(stderr,"native breakable smoke failed: %s\n",error.what());
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];return NO;
    }
}
- (void)saveToURL:(NSURL*)url {
    std::ostringstream output;
    if (_roomScripts) dredmor::SessionRoomScriptExecutor::save(output,*_session,*_roomScripts,*_roomScriptObjects,_roomScriptRuntime);
    else if (_multiFloor) _floors.save(*_session,output);else _session->save(output);
    const auto bytes=output.str();
    NSData *data=[NSData dataWithBytes:bytes.data() length:bytes.size()];
    NSError *error=nil;
    if (![data writeToURL:url options:NSDataWritingAtomic error:&error])
        throw std::runtime_error(error.localizedDescription.UTF8String?:"cannot write save");
}
- (void)loadFromURL:(NSURL*)url {
    NSError *error=nil;NSNumber *size=nil;
    if (![url getResourceValue:&size forKey:NSURLFileSizeKey error:&error] || size.unsignedLongLongValue>64*1024*1024)
        throw std::runtime_error("cannot read save or save exceeds 64 MiB limit");
    NSData *data=[NSData dataWithContentsOfURL:url options:0 error:&error];
    if (!data) throw std::runtime_error(error.localizedDescription.UTF8String?:"cannot read save");
    std::string bytes(static_cast<const char*>(data.bytes),data.length);std::istringstream input(bytes);
    auto definitions=_content?std::make_shared<const std::vector<dredmor::ItemDefinition>>(_content->items()):nullptr;
    auto abilities=_content?std::make_shared<const std::vector<dredmor::AbilityDefinition>>(_content->abilities()):nullptr;
    auto spells=_content?std::make_shared<const std::vector<dredmor::SpellDefinition>>(_content->spells()):nullptr;
    const auto root=dredmor::parse_xml(bytes,"native save");dredmor::FloorRegistry registry;
    std::unique_ptr<dredmor::Session> loaded;
    auto scriptObjects=_roomScriptObjects;auto scriptRuntime=_roomScriptRuntime;
    if (_multiFloor) {
        auto saved=dredmor::FloorRegistry::load(input,definitions,abilities,spells);loaded=std::make_unique<dredmor::Session>(std::move(saved.first));registry=std::move(saved.second);
        registry.for_each_floor(*loaded,[&](const dredmor::Session& floor){@autoreleasepool {(void)[self preparePresentation:floor];}});
    } else {
        if (_roomScripts) {
            auto scripted=dredmor::SessionRoomScriptExecutor::load(input,*_roomScripts,definitions,abilities,spells);
            loaded=std::make_unique<dredmor::Session>(std::move(scripted.session));scriptObjects=std::move(scripted.objects);scriptRuntime=std::move(scripted.runtime);
        } else {
            if (root.tag!="dredmor-session") throw std::runtime_error("open the prepared floors before loading this run");
            loaded=std::make_unique<dredmor::Session>(dredmor::Session::load(input,definitions,abilities,spells));
        }
        const auto& old=_session->world();const auto& world=loaded->world();
        if (old.width()!=world.width() || old.height()!=world.height()) throw std::runtime_error("open the save's prepared room before loading this file");
        if (_session->shops().size()!=loaded->shops().size()) throw std::runtime_error("save shops belong to another room");
        auto samePoint=[](dredmor::Position a,dredmor::Position b){return a.x==b.x && a.y==b.y;};
        for (std::size_t i=0;i<_session->shops().size();++i) {
            const auto& a=_session->shops()[i];const auto& b=loaded->shops()[i];
            if (!samePoint(a.guard,b.guard) || !samePoint(a.home,b.home) || a.keeper.name!=b.keeper.name || a.tiles.size()!=b.tiles.size() || a.pedestals.size()!=b.pedestals.size()) throw std::runtime_error("save shop configuration differs");
            for (std::size_t j=0;j<a.tiles.size();++j) if (!samePoint(a.tiles[j],b.tiles[j])) throw std::runtime_error("save shop floor differs");
            for (std::size_t j=0;j<a.pedestals.size();++j) if (!samePoint(a.pedestals[j].position,b.pedestals[j].position)) throw std::runtime_error("save shop pedestal configuration differs");
        }
        const auto& before=_session->secret_levers();const auto& after=loaded->secret_levers();
        if (before.size()!=after.size()) throw std::runtime_error("save secret levers belong to another room");
        for (std::size_t i=0;i<before.size();++i) if (before[i].position.x!=after[i].position.x || before[i].position.y!=after[i].position.y || before[i].target.x!=after[i].target.x || before[i].target.y!=after[i].target.y) throw std::runtime_error("save secret lever configuration differs");
        const auto& originalLevers=_session->script_levers();const auto& loadedLevers=loaded->script_levers();
        if (!_roomScripts && originalLevers.size()!=loadedLevers.size()) throw std::runtime_error("save ordinary levers belong to another room");
        for (std::size_t i=0;!_roomScripts && i<originalLevers.size();++i) {
            const auto& a=originalLevers[i].object;const auto& b=loadedLevers[i].object;
            if (!samePoint(a.position,b.position) || a.room_local_id!=b.room_local_id || a.can_push!=b.can_push) throw std::runtime_error("save ordinary lever configuration differs");
        }
        if (bool(_session->doors())!=bool(loaded->doors())) throw std::runtime_error("save doors belong to another room");
        if (_session->doors()) {
            if (!_doorBlueprint) throw std::runtime_error("door blueprint is unavailable");
            const auto& a=*_doorBlueprint;const auto& b=*loaded->doors();
            if (a.doors.size()!=b.doors.size() || a.burglary!=b.burglary || a.female!=b.female || a.lockpick_xp!=b.lockpick_xp || a.attack_sound!=b.attack_sound) throw std::runtime_error("save door configuration differs");
            for (std::size_t i=0;i<a.doors.size();++i) {
                if (!samePoint(a.doors[i].assembly.center,b.doors[i].assembly.center) || a.doors[i].assembly.horizontal!=b.doors[i].assembly.horizontal) throw std::runtime_error("save door positions differ");
                for (auto p:a.doors[i].assembly.parts) {
                    const auto& original=a.layout.at(p.x,p.y);const auto& restored=b.layout.at(p.x,p.y);
                    const auto tile=b.doors[i].destroyed?'.':[_doorBaseRows[p.y] characterAtIndex:p.x];
                    if (restored.kind!=(b.doors[i].destroyed?1:original.kind) || ((restored.door_flags^original.door_flags)&~6U) || world.tile(p.x,p.y)!=tile) throw std::runtime_error("save door tiles differ from blueprint");
                }
            }
            for (std::size_t i=0;i<a.layout.cells().size();++i) {
                const int x=int(i%old.width()),y=int(i/old.width());bool part=false;
                for (const auto& door:a.doors) for (auto p:door.assembly.parts) part=part || (p.x==x && p.y==y);
                if (!part && (a.layout.cells()[i].kind!=b.layout.cells()[i].kind || a.layout.cells()[i].door_flags!=b.layout.cells()[i].door_flags)) throw std::runtime_error("save door grid belongs to another room");
            }
        }
        for (int y=0;y<old.height();++y) for (int x=0;x<old.width();++x) if (old.tile(x,y)!=world.tile(x,y)) {
            bool secret=false;for (const auto& lever:before) secret=secret || (lever.target.x==x && lever.target.y==y);
            if (_session->doors()) for (const auto& door:_session->doors()->doors) for (auto p:door.assembly.parts) secret=secret || (p.x==x && p.y==y);
            if (!secret) throw std::runtime_error("save belongs to another room layout");
        }
    }
    NSDictionary *presentation=[self preparePresentation:*loaded];
    if (_roomScripts) {NSMutableDictionary *prepared=[presentation mutableCopy];prepared[@"script-messages"]=@[];presentation=prepared;}
    if (!_multiFloor && loaded->secret_levers().empty() && !loaded->doors()) {NSMutableDictionary *originalRows=[presentation mutableCopy];originalRows[@"rows"]=_rows;presentation=originalRows;}
    _session=std::move(loaded);_floors=std::move(registry);_roomScriptObjects=std::move(scriptObjects);_roomScriptRuntime=std::move(scriptRuntime);[self applyPresentation:presentation];
    [_inventoryController refresh];self.needsDisplay=YES;
}
- (NSDictionary*)preparePresentation:(const dredmor::Session&)session {
    NSMutableDictionary<NSNumber*,SpriteImage*> *images=[NSMutableDictionary dictionary];
    for (const auto& enemy:session.enemies()) {
        if (_content) {
            const auto& definition=_content->monster(enemy.name);
            if (enemy.sprite.empty() || definition.idle_sprites[3]!=enemy.sprite) throw std::runtime_error("saved monster idle sprite does not match content");
            images[@(enemy.id)]=actorSprite(enemy,_dataRoot);
        } else {
            SpriteImage *image=_enemyImages[@(enemy.id)];
            if (!image) throw std::runtime_error("saved actor image is unavailable in this scene");
            images[@(enemy.id)]=image;
        }
    }
    NSMutableArray<SpriteImage*> *objects=[NSMutableArray array];
    for (const auto& object:session.custom_objects()) {
        SpriteImage *image=[[SpriteImage alloc] initWithRoomSprite:object.sprite root:_dataRoot];
        if (!image) throw std::runtime_error("cannot prepare saved room object sprite");
        [objects addObject:image];
    }
    NSMutableArray<NSArray<SpriteImage*>*> *breakables=[NSMutableArray array];
    for (const auto& object:session.breakables()) {
        NSArray<SpriteImage*> *prepared=breakableSprites(object.intact,object.breaking,object.broken,_dataRoot);
        const auto frames=breakableFrames(prepared);
        if (frames.intact!=object.frames.intact || frames.breaking!=object.frames.breaking || frames.broken!=object.frames.broken)
            throw std::runtime_error("saved breakable frames do not match resources");
        if (object.single_breaking_delay) [prepared[1] setSingleFrameDelay:object.single_breaking_delay];[breakables addObject:prepared];
    }
    NSMutableArray<NSArray<SpriteImage*>*> *traps=[NSMutableArray array];
    for (const auto& trap:session.traps()) [traps addObject:trapSprites(trap,_content->items().at(trap.definition),_dataRoot)];
    NSMutableArray<SpriteImage*> *stairs=[NSMutableArray array];
    for (const auto& stair:session.floor_stairs()) {
        if (!_content) throw std::runtime_error("stair rendering requires an installation");
        const auto path=roomResource(_dataRoot,stair.up?"dungeon/stairs_up.spr":"dungeon/stairs_down.spr");
        SpriteImage *image=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
        if (!image) throw std::runtime_error("cannot prepare stair sprite");[stairs addObject:image];
    }
    NSMutableArray<SpriteImage*> *levers=[NSMutableArray array];
    for (const auto& lever:session.secret_levers()) {
        (void)lever;if (!_content) throw std::runtime_error("lever rendering requires an installation");
        const auto path=roomResource(_dataRoot,"dungeon/lever1.spr");SpriteImage *image=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
        if (!image || [image frameCount]<2) throw std::runtime_error("secret lever requires two sprite frames");[levers addObject:image];
    }
    NSMutableArray<SpriteImage*> *scriptLevers=[NSMutableArray array];
    for (const auto& lever:session.script_levers()) {
        const auto path=roomResource(_dataRoot,lever.object.sprite.image);SpriteImage *image=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
        if (!image || [image frameCount]<2) throw std::runtime_error("ordinary lever requires two sprite frames");[scriptLevers addObject:image];
    }
    NSMutableArray<NSDictionary*> *shops=[NSMutableArray array];
    for (const auto& shop:session.shops()) {
        if (!_content) throw std::runtime_error("shop rendering needs installation content");
        (void)_content->monster(shop.keeper.name);
        auto prepare=[&](const std::string& resource) {
            const auto path=roomResource(_dataRoot,resource);SpriteImage *sprite=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
            if (!sprite) throw std::runtime_error("cannot prepare shop sprite");return sprite;
        };
        SpriteImage *keeper=actorSprite(shop.keeper,_dataRoot),*pedestal=prepare("dungeon/store_pedestal.png");NSMutableArray *stock=[NSMutableArray array];
        for (const auto& object:shop.pedestals) [stock addObject:object.item?(id)prepare(_content->items().at(object.item->definition).icon):(id)NSNull.null];
        [shops addObject:@{@"keeper":keeper,@"pedestal":pedestal,@"stock":stock}];
    }
    NSMutableArray<NSArray<SpriteImage*>*> *doors=[NSMutableArray array];
    if (session.doors()) for (const auto& door:session.doors()->doors) {
        if (!_content) throw std::runtime_error("door rendering needs installation content");
        const auto horizontal=door.assembly.horizontal;
        NSMutableArray<SpriteImage*> *parts=[NSMutableArray array];
        for (const auto name:horizontal?std::array<const char*,2>{"dungeon/door_wood3x3_ns_left.spr","dungeon/door_wood3x3_ns_right.spr"}:std::array<const char*,2>{"dungeon/door_wood3x3_ew_top.spr","dungeon/door_wood3x3_ew_bottom.spr"}) {
            const auto path=roomResource(_dataRoot,name);SpriteImage *sprite=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
            if (!sprite || [sprite frameCount]<(horizontal?2U:3U)) throw std::runtime_error("door sprite has too few frames");[parts addObject:sprite];
        }
        [doors addObject:parts];
    }
    NSMutableArray<NSString*> *rows=[NSMutableArray array];
    for (int y=0;y<session.world().height();++y) {std::string row;for (int x=0;x<session.world().width();++x) row+=session.world().tile(x,y);[rows addObject:[NSString stringWithUTF8String:row.c_str()]];}
    return @{@"rows":rows,@"enemies":images,@"objects":objects,@"breakables":breakables,@"traps":traps,@"stairs":stairs,@"levers":levers,@"script-levers":scriptLevers,@"shops":shops,@"doors":doors};
}
- (void)applyPresentation:(NSDictionary*)presentation {
    _rows=presentation[@"rows"];_enemyImages=presentation[@"enemies"];_objectImages=presentation[@"objects"];_breakableImages=presentation[@"breakables"];_trapImages=presentation[@"traps"];_stairImages=presentation[@"stairs"];
    _leverImages=presentation[@"levers"];_scriptLeverImages=presentation[@"script-levers"];_shopImages=presentation[@"shops"];
    _doorImages=presentation[@"doors"];
    _targetSpell.reset();[_breakAnimations removeAllObjects];
    if (presentation[@"script-messages"]) _scriptMessages=presentation[@"script-messages"];
}
- (NSDictionary*)prepareRoomScripts:(dredmor::Session&)session objects:(dredmor::SessionRoomObjects&)objects runtime:(dredmor::RoomRuntimeState&)runtime {
    for (const auto& event:session.events()) if (event.kind==dredmor::EventKind::object_hit || event.kind==dredmor::EventKind::object_broken)
        objects.set_event(session,runtime.triggers.events,{dredmor::SessionRoomObjectKind::breakable,event.target},event.kind==dredmor::EventKind::object_hit?"hit":"broken");
    dredmor::SessionRoomScriptHooks hooks;const auto root=_dataRoot;
    const auto* content=_content.get();dredmor::TrapMap trapMap{_roomScripts->layout.width(),_roomScripts->layout.height(),{}};
    for (const auto& cell:_roomScripts->layout.cells()) trapMap.tiles.push_back(cell.kind);
    hooks.create_record=[content,trapMap](auto& candidate,const auto& record,auto origin) {
        if (!content) throw std::runtime_error("script record requires installation content");
        return content->create_script_record(candidate,record,trapMap,origin);
    };
    const auto layout=_roomScripts->layout;
    hooks.queue_horde=[content,layout](auto& candidate,const auto& record,auto origin) {
        if (!content) throw std::runtime_error("script horde requires installation content");
        content->queue_script_horde(candidate,record,layout,{{{0,0},{layout.width(),layout.height()}}},origin);
    };
    hooks.breakable_frames=[root](const auto& definition) {
        return breakableFrames(breakableSprites(definition.intact,definition.breaking.value_or(definition.intact),definition.broken,root));
    };
    const auto result=dredmor::SessionRoomScriptExecutor::run(session,*_roomScripts,objects,runtime,hooks);
    NSMutableDictionary *prepared=[[self preparePresentation:session] mutableCopy];NSMutableArray<NSString*> *messages=[NSMutableArray array];
    for (const auto& request:result.presentation) if (request.kind==dredmor::RoomActionRequestKind::ticker)
        [messages addObject:[NSString stringWithUTF8String:request.text.c_str()]];
    prepared[@"script-messages"]=messages;return prepared;
}
- (void)commitCandidate:(dredmor::Session&)candidate refresh:(BOOL)refresh {
    auto objects=_roomScriptObjects;auto runtime=_roomScriptRuntime;
    if (objects) objects->synchronize_items(*_session,candidate);
    NSDictionary *prepared=(_roomScripts && candidate.world().turn()!=_session->world().turn())?
        [self prepareRoomScripts:candidate objects:*objects runtime:runtime]:refresh?[self preparePresentation:candidate]:nil;
    *_session=std::move(candidate);_roomScriptObjects=std::move(objects);_roomScriptRuntime=std::move(runtime);
    if (prepared) [self applyPresentation:prepared];
}
- (void)showFailure:(const std::exception&)error {
    NSAlert *alert=[NSAlert new];alert.messageText=@"Cannot complete action";
    alert.informativeText=[NSString stringWithUTF8String:error.what()];[alert runModal];
}
- (void)saveDocument:(id)sender {
    NSSavePanel *panel=[NSSavePanel savePanel];
    panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"dredmor-save" conformingToType:UTTypeXML]];
    panel.nameFieldStringValue=_multiFloor?@"Dungeon.dredmor-save":@"Encounter.dredmor-save";
    panel.message=_multiFloor?@"Save the floors and your character.":@"Save this port's session. Original game saves use a different format.";
    if ([panel runModal]!=NSModalResponseOK) return;
    try { [self saveToURL:panel.URL]; } catch (const std::exception& error) { [self showFailure:error]; }
}
- (void)openDocument:(id)sender {
    NSOpenPanel *panel=[NSOpenPanel openPanel];
    panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"dredmor-save" conformingToType:UTTypeXML]];
    panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;
    panel.message=_multiFloor?@"Load a run with its saved floors.":@"Load a port session for the currently prepared room.";
    if ([panel runModal]!=NSModalResponseOK) return;
    try { [self loadFromURL:panel.URL]; } catch (const std::exception& error) { [self showFailure:error]; }
}
- (BOOL)smokeSaveLoad {
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:
        [NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    try {
        std::ostringstream before;_session->save(before);
        [self saveToURL:url];_session->step(dredmor::Action::wait);[self loadFromURL:url];
        std::ostringstream after;_session->save(after);
        if (before.str()!=after.str()) throw std::runtime_error("native save/load changed session state");
        const auto colour=before.str().find("<sprite-colour");
        if (colour!=std::string::npos) {
            auto bad=before.str();const auto start=bad.find("palette=\"",colour)+9,end=bad.find('"',start);
            if (start<9 || end==std::string::npos) throw std::runtime_error("actor colour save lacks palette");
            bad.replace(start,end-start,"../unavailable-monster.pal");
            NSData *badData=[NSData dataWithBytes:bad.data() length:bad.size()];
            if (![badData writeToURL:url options:NSDataWritingAtomic error:nullptr]) throw std::runtime_error("cannot write palette rejection fixture");
            auto oldImages=_enemyImages;bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
            std::ostringstream unchanged;_session->save(unchanged);
            if (!rejected || unchanged.str()!=before.str() || oldImages!=_enemyImages) throw std::runtime_error("invalid actor palette was not rejected atomically");
        }
        if (!_session->custom_objects().empty()) {
            auto bad=before.str();const auto object=bad.find("<object "),start=bad.find(" image=\"",object);
            if (start==std::string::npos) throw std::runtime_error("object save lacks image field");
            const auto value=start+8,end=bad.find('"',value);bad.replace(value,end-value,"../unavailable-resource.png");
            NSData *badData=[NSData dataWithBytes:bad.data() length:bad.size()];
            if (![badData writeToURL:url options:NSDataWritingAtomic error:nullptr]) throw std::runtime_error("cannot write object rejection fixture");
            bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
            std::ostringstream unchanged;_session->save(unchanged);
            if (!rejected || unchanged.str()!=before.str()) throw std::runtime_error("invalid saved object resource was not rejected atomically");
        }
        [@"invalid save" writeToURL:url atomically:YES encoding:NSUTF8StringEncoding error:nullptr];
        bool rejected=false;
        try { [self loadFromURL:url]; } catch (const std::exception&) { rejected=true; }
        std::ostringstream unchanged;_session->save(unchanged);
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
        return rejected && before.str()==unchanged.str();
    } catch (const std::exception& error) {
        fprintf(stderr,"native save/load failed: %s\n",error.what());
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];return NO;
    }
}
- (BOOL)smokeRoomScripts {
    if (!_roomScripts || !_roomScriptObjects || _session->custom_objects().size()!=1 || _session->script_levers().size()!=1) return NO;
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    const auto resource=roomResource(_dataRoot,"dungeon/replacement.spr");NSString *path=[NSString stringWithUTF8String:resource.c_str()];
    NSData *original=[NSData dataWithContentsOfFile:path];
    try {
        auto snapshot=[&] {std::ostringstream out;dredmor::SessionRoomScriptExecutor::save(out,*_session,*_roomScripts,*_roomScriptObjects,_roomScriptRuntime);return out.str();};
        const auto initial=snapshot();[self saveToURL:url];
        const auto& world=_session->world();const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),(self.bounds.size.height-110)/world.height());
        const auto point=[self convertPoint:NSMakePoint(20+3.5*cell,20+1.5*cell) toView:nil];
        NSEvent *click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point modifierFlags:0 timestamp:0
            windowNumber:self.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
        if (!original || ![[NSFileManager defaultManager] removeItemAtPath:path error:nullptr]) throw std::runtime_error("cannot prepare scripted resource failure");
        auto oldImages=_objectImages;[self mouseDown:click];
        if (snapshot()!=initial || oldImages!=_objectImages) throw std::runtime_error("failed script resource changed committed native state");
        if (![original writeToFile:path atomically:YES]) throw std::runtime_error("cannot restore scripted resource");
        [self mouseDown:click];
        if (_session->player().position.x!=4 || _session->player().position.y!=1 || _session->world().turn()!=3 ||
            _session->custom_objects().size()!=1 || _session->custom_objects()[0].room_local_id!="replacement" ||
            !_session->passable({4,1}) || _objectImages.count!=1 || _scriptMessages.count!=2 ||
            _session->enemies().size()!=1 || _session->ground_items().size()!=1 ||
            !_roomScriptObjects->find(*_session,0,"guardian") || !_roomScriptObjects->find(*_session,0,"reward") ||
            ![_scriptMessages[1] isEqualToString:@"gate visible until clear"] || _roomScriptObjects->find(*_session,0,"gate"))
            throw std::runtime_error("native mouse did not solve scripted puzzle");
        const auto solved=snapshot();[self display];[self loadFromURL:url];
        if (snapshot()!=initial || _session->custom_objects()[0].room_local_id!="gate") throw std::runtime_error("scripted initial reload differs");
        NSEvent *right=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
            windowNumber:self.window.windowNumber context:nil characters:@"\uf703" charactersIgnoringModifiers:@"\uf703" isARepeat:NO keyCode:124];
        [self keyDown:right];if (snapshot()!=solved) throw std::runtime_error("native bump/script continuation differs from click");
        [self saveToURL:url];[self keyDown:[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
            windowNumber:self.window.windowNumber context:nil characters:@"." charactersIgnoringModifiers:@"." isARepeat:NO keyCode:47]];
        if (_scriptMessages.count!=1 || ![_scriptMessages[0] isEqualToString:@"gate gone"]) throw std::runtime_error("later pass did not observe script destruction");
        [self loadFromURL:url];if (snapshot()!=solved) throw std::runtime_error("scripted solved reload differs");
        auto bad=solved;const auto at=bad.find(" image=\"dungeon/replacement.spr\"");
        if (at==std::string::npos) throw std::runtime_error("scripted save lacks replacement resource");
        bad.replace(at,std::string(" image=\"dungeon/replacement.spr\"").size()," image=\"../missing-script.spr\"");
        NSData *badData=[NSData dataWithBytes:bad.data() length:bad.size()];[badData writeToURL:url options:NSDataWritingAtomic error:nullptr];
        oldImages=_objectImages;bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
        if (!rejected || snapshot()!=solved || oldImages!=_objectImages) throw std::runtime_error("scripted load resource rollback failed");
        NSEvent *south=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
            windowNumber:self.window.windowNumber context:nil characters:@"\uf701" charactersIgnoringModifiers:@"\uf701" isARepeat:NO keyCode:125];
        [self keyDown:south];[self keyDown:south];[self pickUpItems:nil];
        if (_session->player().position.x!=4 || _session->player().position.y!=3 || !_session->ground_items().empty() ||
            _roomScriptObjects->carried().size()!=1 || _roomScriptObjects->find(*_session,0,"reward"))
            throw std::runtime_error("scripted reward did not detach into carried storage");
        const auto carried=snapshot();[self saveToURL:url];[self loadFromURL:url];
        if (snapshot()!=carried || _roomScriptObjects->carried().size()!=1) throw std::runtime_error("scripted carried reward reload differs");
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];return YES;
    } catch(const std::exception& error) {
        fprintf(stderr,"native room scripts failed: %s\n",error.what());
        if (original) [original writeToFile:path atomically:YES];[[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];return NO;
    }
}
- (NSArray<NSDictionary*>*)inventoryRows {
    NSMutableArray<NSDictionary*> *rows=[NSMutableArray array];
    auto append=[&](const dredmor::InventoryItem& item,NSString *location,BOOL equipped,NSUInteger slot) {
        NSString *name=item.artifact?[NSString stringWithUTF8String:item.artifact->name.c_str()]:_content && item.definition<_content->items().size()?
            [NSString stringWithUTF8String:_content->items()[item.definition].name.c_str()]:
            [NSString stringWithFormat:@"Item %u",item.definition];
        if (item.charges) name=[NSString stringWithFormat:@"%@ (%d charges)",name,*item.charges];
        if (item.for_sale) location=[@"Unpaid · " stringByAppendingString:location];
        [rows addObject:@{@"name":name,@"quantity":@(item.quantity),@"location":location,@"equipped":@(equipped),
            @"instance":@(item.instance),@"slot":@(slot),@"for_sale":@(item.for_sale)}];
    };
    const auto& inventory=_session->inventory();
    for (std::size_t i=0;i<inventory.bag().size();++i) if (inventory.bag()[i])
        append(*inventory.bag()[i],[NSString stringWithFormat:@"Bag %zu",i+1],NO,i);
    for (std::size_t i=0;i<inventory.belt().size();++i) if (inventory.belt()[i])
        append(*inventory.belt()[i],[NSString stringWithFormat:@"Belt %zu",i+1],NO,i);
    NSArray<NSString*> *slots=@[@"Head",@"Left hand",@"Right hand",@"Left ring",@"Right ring",@"Chest",
        @"Legs",@"Feet",@"Ranged",@"Neck",@"Hands",@"Waist"];
    for (std::size_t i=0;i<12;++i) if (_session->equipment().slots()[i])
        append(*_session->equipment().slots()[i],slots[i],YES,i);
    return rows;
}
- (BOOL)equipInventoryInstance:(uint64_t)instance slot:(NSInteger)slot {
    try {const bool result=_session->equip(instance,static_cast<dredmor::EquipmentSlot>(slot));self.needsDisplay=YES;return result;}
    catch (const std::exception& error) { [self showFailure:error];return NO; }
}
- (BOOL)unequipInventorySlot:(NSInteger)slot {
    try {const bool result=_session->unequip(static_cast<dredmor::EquipmentSlot>(slot));self.needsDisplay=YES;return result;}
    catch (const std::exception& error) { [self showFailure:error];return NO; }
}
- (void)showInventory:(id)sender {
    if (!_inventoryController) _inventoryController=[[DredmorInventoryController alloc] initWithOwner:self];
    [_inventoryController refresh];[_inventoryController showWindow:nil];
}
- (BOOL)useInventoryConsumable:(uint64_t)instance {
    try {auto next=*_session;const bool result=next.eat_food(instance) || next.drink_potion(instance);if (result) {[self commitCandidate:next refresh:NO];[self recordBreakableAnimations];}self.needsDisplay=YES;return result;}
    catch (const std::exception& error) {fprintf(stderr,"consumable failed: %s\n",error.what());return NO;}
}
- (BOOL)quickSellInventoryInstance:(uint64_t)instance {
    try {
        if (!_content) return NO;auto next=*_session;const auto action=next.quick_sell_inventory_item(instance,_content->tweaks().real(dredmor::Difficulty::medium,"store sell price scaler"));
        [self commitCandidate:next refresh:NO];
        self.needsDisplay=YES;return action.handled;
    } catch(const std::exception& error) {[self showFailure:error];return NO;}
}
- (BOOL)sellInventoryInstance:(uint64_t)instance {
    try {
        const auto* item=_session->inventory().find(instance);if (!item || !_content) return NO;
        std::size_t index=0;for (;index<_session->shops().size();++index) {
            const auto& shop=_session->shops()[index];const auto point=shop.keeper.position;
            if (shop.keeper.alive() && !shop.angry && std::abs(point.x-_session->player().position.x)+std::abs(point.y-_session->player().position.y)==1) break;
        }
        if (index==_session->shops().size()) {NSAlert *alert=[NSAlert new];alert.messageText=@"There is no shopkeeper nearby.";[alert addButtonWithTitle:@"OK"];[alert runModal];return YES;}
        const float scaler=_content->tweaks().real(dredmor::Difficulty::medium,"store sell price scaler");const auto& definition=_content->items().at(item->definition);
        const auto offer=dredmor::resolve_keeper_sale(false,item->for_sale,dredmor::item_shop_cost(&definition,*item),scaler,0,_session->player().zorkmids);int choice=0;NSAlert *alert=[NSAlert new];
        if (offer.refused_unpaid) {alert.messageText=@"Quit wavin' that thing at me. You gonna buy it or aincha?";[alert addButtonWithTitle:@"OK"];[alert runModal];}
        else if (offer.refused_price) {alert.messageText=@"I ain't buying that!";[alert addButtonWithTitle:@"OK"];[alert runModal];}
        else {
            alert.messageText=[NSString stringWithFormat:@"I'll give you %d zorkmids for it.",offer.quote];alert.informativeText=[NSString stringWithFormat:@"%@ × %d",[NSString stringWithUTF8String:(item->artifact?item->artifact->name:definition.name).c_str()],item->quantity];
            [alert addButtonWithTitle:@"Don't Sell"];[alert addButtonWithTitle:@"Sell"];choice=[alert runModal]==NSAlertSecondButtonReturn?1:0;
        }
        auto next=*_session;const auto action=next.sell_inventory_item(index,instance,choice,scaler);[self commitCandidate:next refresh:NO];self.needsDisplay=YES;return action.handled;
    } catch(const std::exception& error) {[self showFailure:error];return NO;}
}
- (void)pickUpItems:(id)sender {
    try {auto next=*_session;next.pickup();[self commitCandidate:next refresh:NO];[_inventoryController refresh];self.needsDisplay=YES; }
    catch (const std::exception& error) { [self showFailure:error]; }
}
- (BOOL)smokeInventory {
    const auto primary=_session->player().primary;const auto secondary=_session->player().secondary;
    [self pickUpItems:nil];[self showInventory:nil];
    if ([self inventoryRows].count!=1 || _session->player().zorkmids!=17) return NO;
    if (_session->inventory().bag()[0]->room_local_id!="fixture-loot" || _session->inventory().bag()[0]->can_push!=0) return NO;
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [_inventoryController equipSelection:nil];
    if (!_session->equipment().slots()[0] || _session->inventory().bag()[0] ||
        _session->equipment().slots()[0]->room_local_id!="fixture-loot" || _session->equipment().slots()[0]->can_push!=0) return NO;
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [_inventoryController unequipSelection:nil];
    [_inventoryController close];
    return !_session->equipment().slots()[0] && _session->inventory().bag()[0].has_value() &&
        _session->inventory().bag()[0]->room_local_id=="fixture-loot" && _session->inventory().bag()[0]->can_push==0 &&
        primary==_session->player().primary && secondary==_session->player().secondary;
}
- (BOOL)smokeRoomMonsters {
    try {
    if (_session->enemies().size()!=2 || _enemyImages.count!=2) return NO;
    const auto& named=_session->enemies()[0];const auto& random=_session->enemies()[1];
    const auto count=_session->monster_spawn_state().counts.find("Fixture variant");
    if (!(named.room_spawned && named.room_local_id=="fixture-monster" && named.can_push==0 && !named.sprite.empty() &&
        random.room_spawned && random.room_local_id=="fixture-random" && random.can_push==255 && !random.sprite.empty() &&
        !(random.monster_flags&~10U) && count!=_session->monster_spawn_state().counts.end() && count->second==1)) return NO;
    const auto initial=named;const auto mana=_session->player().mana;
    if (named.sprite_palette!="dungeon/fixture.pal" || named.sprite_tint!=-1 ||
        !random.sprite_palette.empty() || random.sprite_tint!=120 ||
        [_enemyImages[@(named.id)] firstPixelRGB]!=0x1e3c5a ||
        [_enemyImages[@(random.id)] firstPixelRGB]!=0x9527b4) return NO;
    auto cast=[&](const std::string& name) {
        const auto* spell=_content->find_spell(name);return spell && _session->cast_spell(*spell,_session->player().position,_session->enemies()[0].position);
    };
    if (!initial.stat_profile || !cast("Fixture owned buff") || !cast("Fixture owned buff")) return NO;
    const auto& owner=_session->enemies()[0];
    if (owner.buffs.size()!=1 || owner.primary[0]!=initial.primary[0]+1 || owner.health!=initial.health+2 || owner.resistance[7]!=initial.resistance[7]+3) return NO;
    if (!cast("Fixture owned condition")) return NO;
    _session->update_triggers(initial.id);_session->update_buffs(initial.id);
    if (_session->player().mana!=mana-1 || _session->enemies()[0].buffs[0].remaining!=0 || ![self smokeSaveLoad]) return NO;
    if (_session->enemies()[0].sprite_palette!="dungeon/fixture.pal" || _session->enemies()[1].sprite_tint!=120 ||
        [_enemyImages[@(_session->enemies()[0].id)] firstPixelRGB]!=0x1e3c5a ||
        [_enemyImages[@(_session->enemies()[1].id)] firstPixelRGB]!=0x9527b4) return NO;
    _session->update_buffs(initial.id);
    const auto& expired=_session->enemies()[0];
    return expired.buffs.empty() && expired.primary==initial.primary && expired.secondary==initial.secondary && expired.damage==initial.damage &&
        expired.resistance==initial.resistance && expired.health==initial.health && expired.mana==initial.mana && expired.triggers.empty();
    } catch(const std::exception& error) {fprintf(stderr,"native monster buffs: %s\n",error.what());return NO;}
}
- (BOOL)smokeHordes {
    try {
        if (_session->queued_horde_members() || _session->enemies().size()!=4 || _enemyImages.count!=4 || !_session->monster_spawn_state().counts.empty()) return NO;
        std::size_t leaders=0,followers=0;
        for (const auto& actor:_session->enemies()) {
            if (!actor.horde || !actor.stat_profile || !actor.sprite_palette.empty() || ![_enemyImages[@(actor.id)] frameCount]) return NO;
            if (!actor.horde->leader) {
                ++leaders;
                if (actor.horde->followers.size()!=1 || actor.horde->moniker.empty() || actor.horde->platoon.find("Lord Dredmor's ")!=0 ||
                    actor.sprite_tint<0 || actor.sprite_tint>=360 || actor.stat_profile->primary_percent[0]!=120 || actor.stat_profile->primary_percent[1]!=110) return NO;
            } else {
                ++followers;if (!actor.horde->followers.empty() || actor.sprite_tint!=120 || actor.stat_profile->primary_percent[0]!=100) return NO;
            }
        }
        if (leaders!=2 || followers!=2 || ![self smokeSaveLoad]) return NO;
        std::ostringstream before;
        if (_roomScripts) dredmor::SessionRoomScriptExecutor::save(before,*_session,*_roomScripts,*_roomScriptObjects,_roomScriptRuntime);else _session->save(before);
        auto bad=before.str();
        const auto field=bad.find("leader=\"2\"");if (field==std::string::npos) return NO;bad.replace(field,10,"leader=\"999\"");
        NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
        NSData *data=[NSData dataWithBytes:bad.data() length:bad.size()];if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return NO;
        auto oldImages=_enemyImages;bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];std::ostringstream after;
        if (_roomScripts) dredmor::SessionRoomScriptExecutor::save(after,*_session,*_roomScripts,*_roomScriptObjects,_roomScriptRuntime);else _session->save(after);
        return rejected && oldImages==_enemyImages && after.str()==before.str();
    } catch(const std::exception& error) {fprintf(stderr,"native hordes: %s\n",error.what());return NO;}
}
- (BOOL)hasScriptedHordes {
    if (!_roomScripts || !_roomScriptObjects || _roomScriptObjects->bindings().size()!=5) return NO;
    const auto count=_roomScriptRuntime.triggers.success_counts.find({0,0});
    if (count==_roomScriptRuntime.triggers.success_counts.end() || count->second!=1) return NO;
    for (const auto& binding:_roomScriptObjects->bindings()) if (binding.reference.kind==dredmor::SessionRoomObjectKind::monster)
        if (binding.object.assigned_room!=-1 || !binding.object.id.empty()) return NO;
    return YES;
}
- (BOOL)smokeTraps {
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:
        [NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    try {
        auto fail=[&](const char* stage) {
            fprintf(stderr,"native traps failed at %s: HP %d/%d, MP %d/%d, traps %zu, images %lu\n",stage,_session->player().health,
                _session->player().secondary[0],_session->player().mana,_session->player().secondary[1],_session->traps().size(),static_cast<unsigned long>(_trapImages.count));
            for (const auto& trap:_session->traps()) fprintf(stderr,"trap %u: charges %d, discovered %d, pending %d, origin %d,%d/%d, push %u\n",
                trap.definition,trap.charges,trap.discovered,trap.pending_destruction,trap.origin.x,trap.origin.y,trap.has_origin_object,unsigned(trap.can_push));
            return NO;
        };
        if (_session->traps().size()!=2 || _trapImages.count!=2) return fail("ownership");
        const auto& ceiling=_session->traps()[0];const auto& floor=_session->traps()[1];
        if (ceiling.charges!=1 || !ceiling.discovered || ceiling.pending_destruction || ceiling.room_local_id!="fixture-trap" || ceiling.can_push!=0 ||
            _session->player().health!=_session->player().secondary[0]-7 || _session->player().mana!=_session->player().secondary[1]-2 || !floor.has_origin_object || floor.origin.x!=1 || floor.origin.y!=2 ||
            _session->passable(floor.origin) || !_session->world().passable(floor.origin) || [_trapImages[0][1] frameCount]!=2 || [_trapImages[1][2] frameCount]!=2) return fail("initial activation");
        if (![self smokeSaveLoad]) return fail("save/load");
        std::ostringstream before;_session->save(before);NSArray *images=_trapImages;std::vector<std::string> invalid;
        auto wrongSpell=before.str();auto start=wrongSpell.find("<spell-rules content=\"");
        if (start==std::string::npos) return NO;start+=std::string("<spell-rules content=\"").size();const auto end=wrongSpell.find('"',start);
        wrongSpell.replace(start,end-start,"1");invalid.push_back(std::move(wrongSpell));
        auto badOrigin=before.str();start=badOrigin.find("origin-object=\"0\"");if (start==std::string::npos) return NO;
        badOrigin.replace(start,std::string("origin-object=\"0\"").size(),"origin-object=\"1\"");invalid.push_back(std::move(badOrigin));
        for (const auto& bytes:invalid) {
            NSData *data=[NSData dataWithBytes:bytes.data() length:bytes.size()];if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return NO;
            bool failed=false;try {[self loadFromURL:url];} catch(const std::exception&) {failed=true;}
            std::ostringstream after;_session->save(after);
            if (!failed || after.str()!=before.str() || images!=_trapImages) {
                fprintf(stderr,"native traps rollback: rejected %d, same session %d, same images %d\n",failed,after.str()==before.str(),images==_trapImages);
                return fail("invalid load rollback");
            }
        }
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
        for (NSString *key in @[@"\uf702",@"\uf703"]) {
            const unsigned short code=[key isEqualToString:@"\uf702"]?123:124;
            NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
                windowNumber:self.window.windowNumber context:nil characters:key charactersIgnoringModifiers:key isARepeat:NO keyCode:code];[self keyDown:event];
        }
        if (_session->traps()[0].charges!=0 || !_session->traps()[0].pending_destruction ||
            _session->player().health!=_session->player().secondary[0]-14 || _session->player().mana!=_session->player().secondary[1]-4 || _session->passable(_session->traps()[1].origin)) return fail("second activation");
        auto schedule=[&](const std::string& name) {
            for (const auto& spell:_content->spells()) if (spell.name==name) {
                const auto position=_session->player().position;return _session->cast_spell(spell,position,position);
            }
            return false;
        };
        const auto health=_session->player().health,mana=_session->player().mana;
        if (!schedule("Fixture delayed") || _session->player().triggers.size()!=1) return fail("delayed creation");
        _session->step(dredmor::Action::wait);
        if (_session->player().health!=health || _session->player().triggers[0].remaining!=1 || ![self smokeSaveLoad]) return fail("delayed save");
        _session->step(dredmor::Action::wait);
        if (!_session->player().triggers.empty() || _session->player().health!=health+1 || _session->player().mana!=mana-1) return fail("delayed firing");
        if (!schedule("Fixture overtime")) return fail("dot creation");
        for (int i=1;i<=3;++i) {
            _session->step(dredmor::Action::wait);
            if (_session->player().health!=health+1+i || _session->player().mana!=mana-1-i) return fail("dot firing");
            if (i==1 && ![self smokeSaveLoad]) return fail("dot save");
        }
        if (!_session->player().triggers.empty()) return fail("dot expiration");
        const auto primary=_session->player().primary;const auto secondary=_session->player().secondary;
        const auto resistance=_session->player().resistance;const auto buffHealth=_session->player().health,buffMana=_session->player().mana;
        if (!schedule("Fixture active buff") || _session->player_buffs().size()!=1 || _session->player().primary[0]!=primary[0]+1 ||
            _session->player().secondary[0]!=secondary[0]+3 || _session->player().health!=buffHealth+2) return fail("buff application");
        _session->step(dredmor::Action::wait);
        if (_session->player_buffs()[0].remaining!=0 || _session->player_buffs()[0].upkeep_remaining!=1 || ![self smokeSaveLoad]) return fail("buff save");
        if (!schedule("Fixture active buff") || _session->player_buffs().size()!=1 || _session->player_buffs()[0].remaining!=1 || _session->player_buffs()[0].upkeep_remaining!=1) return fail("buff refresh");
        if (!schedule("Fixture gated") || _session->player().triggers.size()!=1 || ![self smokeSaveLoad]) return fail("gated save");
        _session->step(dredmor::Action::wait);_session->step(dredmor::Action::wait);
        if (!_session->player_buffs().empty() || _session->player().primary!=primary || _session->player().secondary!=secondary ||
            _session->player().health!=buffHealth || _session->player().mana!=buffMana-2 || _session->player().resistance[7]!=resistance[7]+3 || !_session->player().triggers.empty()) return fail("buff expiration");
        if (!schedule("Fixture buff rebuild") || _session->player().resistance!=resistance) return fail("buff subsequent rebuild");
        _session->step(dredmor::Action::wait);
        for (int i=0;i<24;++i) if (!schedule("Fixture large buff")) return fail("large buff application");
        _session->step(dredmor::Action::wait);
        if (_session->player_buffs().size()!=12 || _session->player().primary!=primary || ![self smokeSaveLoad]) return fail("large buff traversal");
        _session->step(dredmor::Action::wait);
        if (!_session->player_buffs().empty() || _session->player().primary[0]!=std::max(0,primary[0]-12) || ![self smokeSaveLoad]) return fail("large buff residual state");
        const auto gateMana=_session->player().mana;
        if (!schedule("Fixture effect gate") || _session->player().mana!=gateMana-1 || ![self smokeSaveLoad]) return fail("effect gate application");
        if (!schedule("Fixture effect gate") || _session->player().mana!=gateMana-1 || _session->player_buffs().size()!=1) return fail("effect gate suppression");
        _session->step(dredmor::Action::wait);_session->depth_context({2,false,-1});
        if (_session->player().zorkmids!=0) return fail("gold baseline");
        _session->give_currency(8);const auto goldMana=_session->player().mana;
        if (!schedule("Fixture gold buff")) return fail("gold buff application");
        _session->step(dredmor::Action::wait);
        if (_session->player().zorkmids!=5 || ![self smokeSaveLoad] || _session->depth_context().depth!=2) return fail("gold buff save");
        _session->step(dredmor::Action::wait);_session->step(dredmor::Action::wait);
        if (_session->player().zorkmids!=2 || _session->player().mana!=goldMana+2 || !_session->player_buffs().empty()) return fail("gold buff insufficiency");
        return YES;
    } catch(const std::exception& error) {
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];fprintf(stderr,"native traps: %s\n",error.what());return NO;
    }
}
- (BOOL)smokeSkillSelection {
    if (!_content) return YES;
    DredmorSkillSelectionController *controller=[[DredmorSkillSelectionController alloc] initWithSkills:_content->skills()];
    return [controller smokeTest];
}
- (BOOL)smokeArtifact {
    const auto& item=_session->inventory().bag()[0];
    if (!item || !item->artifact || item->artifact->quality!=10 || item->artifact->name.empty()) return NO;
    NSArray<NSDictionary*> *rows=[self inventoryRows];
    return rows.count==1 && [rows[0][@"name"] isEqualToString:[NSString stringWithUTF8String:item->artifact->name.c_str()]];
}
- (BOOL)smokeFloors {
    if (!_multiFloor || _session->depth_context().depth!=1 || _session->world().width()!=8 || _rows.count!=5 || _stairImages.count!=2 || _session->enemies().size()!=1) return NO;
    auto move=[&](unsigned short code,NSString *key) {
        NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:key charactersIgnoringModifiers:key isARepeat:NO keyCode:code];[self keyDown:event];
    };
    const auto health=_session->player().health,mana=_session->player().mana;
    move(125,@"\uf701");move(126,@"\uf700");
    if (_session->depth_context().depth!=0 || _rows.count!=4 || _stairImages.count!=1 || _enemyImages.count!=0 || _session->custom_objects().size()!=1 || _floors.cached().size()!=1) return NO;
    const auto enemy=_floors.cached()[0].enemies.at(0);
    move(124,@"\uf703");move(123,@"\uf702");
    if (_session->depth_context().depth!=1 || _session->player().zorkmids!=17 || _session->enemies().size()!=1 || _session->player().health!=health || _session->player().mana!=mana) return NO;
    const auto& restored=_session->enemies()[0];
    if (restored.id!=enemy.id || restored.health!=enemy.health || restored.position.x!=enemy.position.x || restored.position.y!=enemy.position.y || !_floors.cached()[0].ground.empty() || ![self smokeSaveLoad]) return NO;
    std::ostringstream before;_floors.save(*_session,before);auto invalid=before.str();const auto object=invalid.find("<object "),field=invalid.find(" image=\"",object);
    if (field==std::string::npos) return NO;const auto start=field+8,end=invalid.find('"',start);invalid.replace(start,end-start,"../missing.spr");
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    NSData *data=[NSData dataWithBytes:invalid.data() length:invalid.size()];if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return NO;
    bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}[[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
    std::ostringstream after;_floors.save(*_session,after);[self display];return rejected && before.str()==after.str();
}
- (BOOL)smokeSecretLevers {
    if (_session->secret_levers().size()!=1 || _leverImages.count!=1 || [_leverImages[0] frameCount]<2) return NO;
    const auto lever=_session->secret_levers()[0];
    if (!lever.activated || !_session->world().passable(lever.target) || _session->passable(lever.position) || [_rows[lever.target.y] characterAtIndex:lever.target.x]!='.' || ![self smokeSaveLoad]) return NO;
    // Load the same prepared room with the secret wall still closed.
    auto initial=*_session;auto floor=initial.floor_state();auto text=floor.world.room_text();text[lever.target.y*(floor.world.width()+1)+lever.target.x]='#';
    std::istringstream rows(text);floor.world=dredmor::World::restore(rows,floor.world.turn());floor.secret_levers[0].activated=false;initial.swap_floor(std::move(floor),initial.player().position);
    std::ostringstream closed;initial.save(closed);
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    auto write=[&](const std::string& value) {NSData *data=[NSData dataWithBytes:value.data() length:value.size()];return [data writeToURL:url options:NSDataWritingAtomic error:nullptr];};
    if (!write(closed.str())) return NO;[self loadFromURL:url];
    if (_session->secret_levers()[0].activated || _session->world().passable(lever.target) || [_rows[lever.target.y] characterAtIndex:lever.target.x]!='#') return NO;
    const auto& world=_session->world();const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),(self.bounds.size.height-110)/world.height());
    const NSPoint local=NSMakePoint(20+(lever.position.x+0.5)*cell,20+(lever.position.y+0.5)*cell),window=[self convertPoint:local toView:nil];
    NSEvent *click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:window modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
    const auto turn=_session->world().turn();[self mouseDown:click];
    if (!_session->secret_levers()[0].activated || !_session->world().passable(lever.target) || _session->world().turn()!=turn+1 || [_rows[lever.target.y] characterAtIndex:lever.target.x]!='.' || ![self smokeSaveLoad]) return NO;
    [self mouseDown:click];if (_session->world().turn()!=turn+2 || !_session->world().passable(lever.target)) return NO;
    std::ostringstream before;_session->save(before);auto invalid=before.str();const auto field=invalid.find("target-x=\"");if (field==std::string::npos) return NO;const auto start=field+10,end=invalid.find('"',start);invalid.replace(start,end-start,"5");
    if (!write(invalid)) return NO;bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
    [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];std::ostringstream after;_session->save(after);[self display];return rejected && before.str()==after.str();
}
- (BOOL)smokeScriptLevers {
    auto fail=[&](const char* stage) {fprintf(stderr,"native ordinary lever smoke failed at %s\n",stage);return NO;};
    if (_session->script_levers().size()!=1 || _scriptLeverImages.count!=1 || [_scriptLeverImages[0] frameCount]<2 || _session->script_levers()[0].activation.activated()) return fail("initial");
    const auto point=_session->script_levers()[0].object.position;
    const auto& world=_session->world();const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),(self.bounds.size.height-110)/world.height());
    const NSPoint local=NSMakePoint(20+(point.x+0.5)*cell,20+(point.y+0.5)*cell),window=[self convertPoint:local toView:nil];
    NSEvent *click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:window modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];
    auto key=[&](NSString *text,unsigned short code) {
        NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];[self keyDown:event];
    };
    const auto turn=_session->world().turn();[self mouseDown:click];
    if (!_session->script_levers()[0].activation.activated() || !_session->script_levers()[0].activation.changed() || _session->world().turn()!=turn+1 || ![self smokeSaveLoad]) return fail("click/save");
    std::ostringstream active;_session->save(active);NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    auto write=[&](const std::string& value) {NSData *data=[NSData dataWithBytes:value.data() length:value.size()];return [data writeToURL:url options:NSDataWritingAtomic error:nullptr];};
    key(@"d",124);if (_session->script_levers()[0].activation.activated() || _session->world().turn()!=turn+2 || _session->player().position.x!=2) return fail("bump");
    if (!write(active.str())) return fail("write");[self loadFromURL:url];
    if (!_session->script_levers()[0].activation.activated() || _session->world().turn()!=turn+1) return fail("restore");
    key(@".",47);if (_session->script_levers()[0].activation.changed()) return fail("history");
    key(@"a",123);const auto distant=_session->world().turn();[self mouseDown:click];if (_session->world().turn()!=distant) return fail("distance");
    std::ostringstream before;_session->save(before);auto invalid=before.str();const auto field=invalid.find("current=\"1\"");if (field==std::string::npos) return fail("history field");invalid.replace(field,11,"current=\"2\"");
    if (!write(invalid)) return fail("invalid write");bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
    [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];std::ostringstream after;_session->save(after);[self display];return rejected && before.str()==after.str();
}
- (BOOL)smokeDoors {
    auto fail=[&](const char* stage) {fprintf(stderr,"native door smoke failed at %s\n",stage);return NO;};
    if (!_session->doors() || _session->doors()->doors.size()!=2 || _doorImages.count!=2 || [_doorImages[0][0] frameCount]!=2 || [_doorImages[1][0] frameCount]!=3) return fail("ownership");
    if (!_session->doors()->doors[0].assembly.locked || !_session->doors()->doors[1].assembly.locked || _session->passable({4,3}) || _session->world().passable({4,2})) return fail("initial locks/collision");
    std::ostringstream original;_session->save(original);
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    auto write=[&](const std::string& bytes) {[[NSData dataWithBytes:bytes.data() length:bytes.size()] writeToURL:url atomically:YES];};
    auto bad=original.str();const auto position=bad.find("<door x=\"2\"");if (position==std::string::npos) return fail("topology fixture");bad[position+9]='3';write(bad);
    bool rejected=false;NSArray *images=_doorImages;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}
    std::ostringstream unchanged;_session->save(unchanged);if (!rejected || unchanged.str()!=original.str() || images!=_doorImages) return fail("atomic topology rejection");
    auto move=[&](unsigned short code,NSString *key) {
        NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:key charactersIgnoringModifiers:key isARepeat:NO keyCode:code];[self keyDown:event];
    };
    auto click=[&](int x,int y) {
        const CGFloat cell=MIN((self.bounds.size.width-40)/_session->world().width(),(self.bounds.size.height-110)/_session->world().height());
        const NSPoint point=[self convertPoint:NSMakePoint(20+(x+.5)*cell,20+(y+.5)*cell) toView:nil];
        NSEvent *event=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil eventNumber:1 clickCount:1 pressure:1];[self mouseDown:event];
    };
    bool supplied=false;for (std::size_t i=0;i<_content->items().size();++i) if (_content->items()[i].builtin_subtype==0) {
        _session->create_ground_item(std::uint32_t(i),_session->player().position,1);_session->pickup();supplied=true;break;
    }
    if (!supplied) return fail("lockpick fixture");
    move(125,@"\uf701");move(125,@"\uf701");move(124,@"\uf703");
    const auto before=_session->world().turn();click(4,3);
    if (!_session->doors()->doors[1].assembly.open || _session->doors()->doors[1].assembly.locked || _session->doors()->frames[1]!=2 || _session->world().turn()!=before+1 || _session->progression().experience!=5 || [self inventoryRows].count!=0 || ![self smokeSaveLoad]) return fail("pick/open/frame/save");
    if (_session->world().passable({4,2}) || _session->world().passable({4,4})) return fail("wall end preservation");
    move(124,@"\uf703");const auto occupied=_session->world().turn();click(4,2);
    if (!_session->doors()->doors[1].assembly.open || _session->world().turn()!=occupied+1 || !_session->last_door_click() || _session->last_door_click()->messages[0]!="The doorway is blocked!") return fail("occupied close");
    click(4,3);if (_session->world().turn()!=occupied+1) return fail("center ignored");
    move(123,@"\uf702");move(126,@"\uf700");click(4,2);
    if (_session->doors()->doors[1].assembly.open || _session->doors()->frames[1]!=0 || ![self smokeSaveLoad]) return fail("close/save");
    move(125,@"\uf701");const auto bump=_session->world().turn();move(124,@"\uf703");
    if (!_session->doors()->doors[1].assembly.open || _session->player().position.x!=3 || _session->world().turn()!=bump+1) return fail("bump opens");
    move(123,@"\uf702");move(125,@"\uf701");
    for (int attempt=0;attempt<4 && !_session->doors()->doors[0].destroyed;++attempt) move(125,@"\uf701");
    if (!_session->doors()->doors[0].destroyed || !_session->world().passable({1,5}) || !_session->world().passable({3,5}) || [_rows[5] characterAtIndex:1]!='.' || ![self smokeSaveLoad]) return fail("shatter/carve/save");
    write(original.str());[self loadFromURL:url];[[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
    if (_session->doors()->doors[0].destroyed || _session->world().passable({1,5}) || [_rows[5] characterAtIndex:1]!='#' || _session->doors()->doors[1].assembly.open) return fail("restore closed walls");
    [self display];return YES;
}
- (BOOL)smokeDecorations {
    const auto& objects=_session->custom_objects();if (objects.size()!=4 || _objectImages.count!=4 || objects[0].name!="Explicit") return NO;
    if (objects[1].sprite.image!="dungeon/painting.spr" || objects[2].sprite.image!="dungeon/statue.spr" || objects[3].sprite.image!="dungeon/floor.spr") return NO;
    for (std::size_t i=1;i<objects.size();++i) if (objects[i].playing || objects[i].frame || objects[i].blocks_vision || [_objectImages[i] frameCount]!=2) return NO;
    if (!objects[1].blocks_movement || !objects[2].blocks_movement || objects[3].blocks_movement || _session->passable({3,1}) || !_session->passable({3,2})) return NO;
    std::ostringstream before;_session->save(before);if (![self smokeSaveLoad]) return NO;std::ostringstream after;_session->save(after);
    [self display];return before.str()==after.str() && !_session->custom_objects()[3].playing;
}
- (BOOL)smokeShops {
    auto fail=[&](const char* stage) {fprintf(stderr,"native shop smoke failed at %s\n",stage);return NO;};
    if (_session->shops().size()!=1 || _shopImages.count!=1 || _session->shops()[0].pedestals.size()!=3 || _session->player().zorkmids!=100) return fail("initial ownership");
    {
        std::ostringstream encoded;_session->save(encoded);const auto before=encoded.str();NSArray *images=_shopImages;
        auto wrong_layout=before;const auto guard=wrong_layout.find("guard-x=\"5\"");if (guard==std::string::npos) return fail("layout mutation fixture");wrong_layout.replace(guard,11,"guard-x=\"4\"");
        auto wrong_asset=before;const auto kind=_session->shops()[0].pedestals[0].item->kind;std::size_t bad_definition=0;
        for (;bad_definition<_content->items().size();++bad_definition) if (_content->items()[bad_definition].name==(kind==dredmor::ItemKind::weapon?"Bad shop sword":"Bad shop cap")) break;
        const auto pedestal=wrong_asset.find("<pedestal "),field=wrong_asset.find("definition=\"",pedestal);if (field==std::string::npos || bad_definition==_content->items().size()) return fail("asset mutation fixture");
        const auto value=field+12,end=wrong_asset.find('"',value);wrong_asset.replace(value,end-value,std::to_string(bad_definition));
        NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
        for (const auto& bytes:{wrong_layout,wrong_asset}) {
            NSData *data=[NSData dataWithBytes:bytes.data() length:bytes.size()];if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return fail("rejection fixture write");
            bool rejected=false;try {[self loadFromURL:url];} catch(const std::exception&) {rejected=true;}std::ostringstream after;_session->save(after);
            if (!rejected || after.str()!=before || images!=_shopImages) {[[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];return fail("atomic rejection");}
        }
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
    }
    auto move=[&](unsigned short code,NSString *key) {
        NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:key charactersIgnoringModifiers:key isARepeat:NO keyCode:code];[self keyDown:event];
    };
    auto prepareChoice=[&](NSString *title) {
        NSTimer *timer=[NSTimer timerWithTimeInterval:.05 repeats:YES block:^(NSTimer *active) {
            std::function<BOOL(NSView*)> press=[&](NSView *view) {
                if ([view isKindOfClass:NSButton.class] && [[(NSButton*)view title] isEqualToString:title]) {[(NSButton*)view performClick:nil];return YES;}
                for (NSView *child in view.subviews) if (press(child)) return YES;return NO;
            };
            if (NSApp.modalWindow && press(NSApp.modalWindow.contentView)) {[active invalidate];}
        }];
        [NSRunLoop.currentRunLoop addTimer:timer forMode:NSModalPanelRunLoopMode];
        return timer;
    };
    auto choose=[&](dredmor::Position position,NSString *title) {
        NSTimer *timer=prepareChoice(title);
        const CGFloat cell=MIN((self.bounds.size.width-40)/_session->world().width(),(self.bounds.size.height-110)/_session->world().height());
        const NSPoint point=[self convertPoint:NSMakePoint(20+(position.x+.5)*cell,20+(position.y+.5)*cell) toView:nil];
        NSEvent *event=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];[self mouseDown:event];const BOOL clicked=!timer.valid;[timer invalidate];return clicked;
    };
    move(124,@"\uf703");move(124,@"\uf703");move(125,@"\uf701");move(125,@"\uf701");move(124,@"\uf703");
    if (!_session->shop_presence().in_shop || !choose({6,3},@"Don't Buy") || !_session->shops()[0].pedestals[1].item) return fail("decline dialog");
    if (!choose({6,3},@"Buy") || _session->player().zorkmids!=90 || _session->shops()[0].pedestals[1].item || _shopImages[0][@"stock"][1]!=NSNull.null || ![self smokeSaveLoad]) return fail("purchase save");
    move(126,@"\uf700");[self showInventory:nil];
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];NSTimer *declineSale=prepareChoice(@"Don't Sell");[_inventoryController sellSelection:nil];const BOOL declinedSale=!declineSale.valid;[declineSale invalidate];
    if (!declinedSale || [self inventoryRows].count!=1 || _session->player().zorkmids!=90) return fail("declined sale");
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];NSTimer *acceptSale=prepareChoice(@"Sell");[_inventoryController sellSelection:nil];const BOOL acceptedSale=!acceptSale.valid;[acceptSale invalidate];[_inventoryController close];
    if (!acceptedSale || [self inventoryRows].count || _session->player().zorkmids!=95 || ![self smokeSaveLoad]) return fail("sold item save");
    _session->create_ground_item(0,_session->player().position);[self pickUpItems:nil];[self showInventory:nil];
    if ([self inventoryRows].count!=1) return fail("quick sale fixture ownership");
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController quickSellSelection:nil];[_inventoryController close];
    if ([self inventoryRows].count || _session->player().zorkmids!=100 || ![self smokeSaveLoad]) return fail("quick sale save");
    move(125,@"\uf701");move(125,@"\uf701");move(124,@"\uf703");move(124,@"\uf703");
    if (!choose({7,3},@"Take unpaid") || !_session->has_unpaid_goods() || !_session->shops()[0].guarding || _session->shops()[0].keeper.position.x!=5 || ![self smokeSaveLoad]) return fail("taking and guard save");
    bool labelled=false;for (NSDictionary *row in [self inventoryRows]) if ([row[@"for_sale"] boolValue] && [row[@"location"] hasPrefix:@"Unpaid"]) labelled=true;if (!labelled) return fail("unpaid inventory label");
    move(123,@"\uf702");move(123,@"\uf702");move(126,@"\uf700");move(126,@"\uf700");
    if (!choose({5,1},@"Don't Buy") || !_session->has_unpaid_goods() || _session->player().zorkmids!=100) return fail("declined debt");
    if (!choose({5,1},@"Buy") || _session->has_unpaid_goods() || _session->player().zorkmids!=90 || _session->shops()[0].guarding || _session->shops()[0].keeper.position.x!=6 || ![self smokeSaveLoad]) return fail("settled debt save");
    move(126,@"\uf700");
    if (!choose({6,1},@"Take unpaid") || !_session->has_unpaid_goods() || !_session->shops()[0].guarding || ![self smokeSaveLoad]) return fail("second unpaid taking");
    move(123,@"\uf702");NSTimer *notice=prepareChoice(@"OK");move(123,@"\uf702");const BOOL dismissed=!notice.valid;[notice invalidate];if (!dismissed) return fail("theft notice dialog");
    if (_session->shop_presence().in_shop || !_session->shop_presence().level_angry || !_session->shops()[0].angry || _session->has_unpaid_goods() || ![self smokeSaveLoad]) return fail("theft exit save");
    [self display];return YES;
}
- (BOOL)smokeAutomaticArmour {
    const auto primary=_session->player().primary;const auto secondary=_session->player().secondary;
    [self pickUpItems:nil];[self showInventory:nil];if ([self inventoryRows].count!=1 || !_session->inventory().bag()[0]) return NO;
    const auto item=*_session->inventory().bag()[0];if (!item.artifact || item.room_local_id!="automatic-neck" || item.artifact->quality<1 || item.artifact->quality>3) return NO;
    const auto& definition=_content->items().at(item.definition);if (definition.armour_type!=9 || dredmor::armour_constructor_rules(definition).type!=6) return NO;
    if (item.artifact->name==definition.name) return NO;
    if (![[self inventoryRows][0][@"name"] isEqualToString:[NSString stringWithUTF8String:item.artifact->name.c_str()]]) return NO;
    for (NSView *control in _inventoryController.window.contentView.subviews) if ([control isKindOfClass:NSPopUpButton.class]) [(NSPopUpButton*)control selectItemAtIndex:9];
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController equipSelection:nil];
    if (!_session->equipment().slots()[9] || _session->inventory().bag()[0] || ![[self inventoryRows][0][@"location"] isEqualToString:@"Neck"] || ![self smokeSaveLoad]) return NO;
    [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController unequipSelection:nil];[_inventoryController close];
    const auto& restored=_session->inventory().bag()[0];return restored && restored->artifact && restored->artifact->name==item.artifact->name && primary==_session->player().primary && secondary==_session->player().secondary && [self smokeSaveLoad];
}
- (BOOL)smokeFood {
    if (!_session->regeneration()) return NO;
    const auto hp=_session->player().health,mp=_session->player().mana;const auto turn=_session->world().turn();const auto primary=_session->player().primary[0];
    const auto* drain=_content->find_spell("Fixture drain");if (!drain || !_session->cast_spell(*drain,_session->player().position,_session->player().position)) return NO;
    for (std::size_t i=0;i<_content->items().size();++i) if (_content->items()[i].name=="Fixture meal") {
        _session->create_ground_item(std::uint32_t(i),_session->player().position,2);_session->pickup();[self showInventory:nil];
        [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController useSelection:nil];
        if (_session->player().health!=hp-4 || _session->player().mana!=mp-5 || _session->regeneration()->food!=2 || _session->regeneration()->booze!=1 || _session->player().primary[0]!=primary+1 || _session->player_buffs().size()!=1 || ![self smokeSaveLoad]) return NO;
        [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController useSelection:nil];[_inventoryController close];
        if ([self inventoryRows].count || _session->regeneration()->food!=4 || _session->regeneration()->booze!=2 || _session->player().primary[0]!=primary+2 || _session->player_buffs().size()!=1 || ![self smokeSaveLoad]) return NO;
        for (int n=0;n<4;++n) {_session->step(dredmor::Action::wait);if (_session->player().primary[0]!=primary+(n==0?1:0)) return NO;}
        [self display];return _session->player().health==hp && _session->player().mana==mp-3 && !_session->regeneration()->food && !_session->regeneration()->booze && _session->player_buffs().empty() && _session->world().turn()==turn+6 && [self smokeSaveLoad];
    }
    return NO;
}
- (BOOL)smokeRegeneration {
    if (!_session->regeneration()) return NO;
    const auto hp=_session->player().health,mp=_session->player().mana;
    const auto* spell=_content->find_spell("Fixture drain");if (!spell || !_session->cast_spell(*spell,_session->player().position,_session->player().position)) return NO;
    if (_session->player().health!=hp-5 || _session->player().mana!=mp-5) return NO;
    NSEvent *wait=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
        windowNumber:self.window.windowNumber context:nil characters:@"." charactersIgnoringModifiers:@"." isARepeat:NO keyCode:47];
    [self keyDown:wait];if (![self smokeSaveLoad]) return NO;
    for (int i=0;i<30 && (_session->player().health<hp || _session->player().mana<mp);++i) [self keyDown:wait];
    return _session->player().health==hp && _session->player().mana==mp && [self smokeSaveLoad];
}
- (BOOL)smokeCasting {
    NSMenu *menu=[self spellMenu];if (menu.numberOfItems!=3) return NO;
    const auto mana=_session->player().mana;const auto turn=_session->world().turn();
    [self selectSpell:[menu itemAtIndex:0]];
    if (_targetSpell || _session->world().turn()!=turn+1 || _session->player().mana!=mana-3) return NO;
    [self selectSpell:[menu itemAtIndex:1]];
    const auto cooldown=[[menu itemAtIndex:1].representedObject unsignedLongLongValue];
    if (_targetSpell || _session->spell_cooldown(cooldown)!=3 || _session->player().mana!=mana-5 || ![self smokeSaveLoad]) return NO;
    if ([[self spellMenu] itemAtIndex:1].enabled) return NO;
    [self selectSpell:[menu itemAtIndex:2]];if (!_targetSpell) return NO;
    const auto& world=_session->world();const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),(self.bounds.size.height-110)/world.height());
    const auto point=[self convertPoint:NSMakePoint(20+1.5*cell,20+1.5*cell) toView:nil];
    NSEvent *click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point modifierFlags:0 timestamp:0
        windowNumber:self.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1];[self mouseDown:click];
    if (_targetSpell || _session->world().turn()!=turn+3 || _session->player().mana!=mana-7 || _session->spell_cooldown(cooldown)!=2 || ![self smokeSaveLoad]) return NO;
    for (std::size_t i=0;i<_content->items().size();++i) if (_content->items()[i].name=="Fixture potion") {
        _session->create_ground_item(std::uint32_t(i),_session->player().position,2);_session->pickup();
        [self showInventory:nil];[_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
        [_inventoryController useSelection:nil];
        if (_session->world().turn()!=turn+4 || _session->player().mana!=mana-8 || [self inventoryRows].count!=1 || ![self smokeSaveLoad]) return NO;
        [_inventoryController.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_inventoryController useSelection:nil];[_inventoryController close];
        return _session->world().turn()==turn+5 && _session->player().mana==mana-9 && [self inventoryRows].count==0 && [self smokeSaveLoad];
    }
    return NO;
}
- (void)keyDown:(NSEvent*)event {
    if (event.keyCode==53 && _targetSpell) { _targetSpell.reset();self.needsDisplay=YES;return; }
    using dredmor::Action;
    Action action;
    switch (event.keyCode) {
        case 123: action=Action::west; break;
        case 124: action=Action::east; break;
        case 125: action=Action::south; break;
        case 126: action=Action::north; break;
        default:
            if ([event.charactersIgnoringModifiers.lowercaseString isEqualToString:@"g"]) { [self pickUpItems:nil];return; }
            else if ([event.charactersIgnoringModifiers.lowercaseString isEqualToString:@"i"]) { [self showInventory:nil];return; }
            else if ([event.charactersIgnoringModifiers.lowercaseString isEqualToString:@"a"]) { [self showSpells:nil];return; }
            else if ([event.charactersIgnoringModifiers isEqualToString:@"."]) action=Action::wait;
            else { [super keyDown:event]; return; }
    }
    try {
        auto session=*_session;auto registry=_floors;const auto outcome=registry.step(session,action);
        bool lever=false;for (const auto& event:session.events()) lever=lever || event.kind==dredmor::EventKind::lever_used || event.kind==dredmor::EventKind::door_used;
        [self commitCandidate:session refresh:(outcome==dredmor::FloorTravel::travelled || lever)];_floors=std::move(registry);
        if (outcome==dredmor::FloorTravel::missing_floor || outcome==dredmor::FloorTravel::missing_arrival) NSBeep();
        [self recordBreakableAnimations];[self presentShopNotices];
    }
    catch (const std::exception& e) { fprintf(stderr,"turn failed: %s\n",e.what()); }
    [_inventoryController refresh];
    self.needsDisplay=YES;
}
- (NSMenu*)spellMenu {
    NSMenu *menu=[[NSMenu alloc] initWithTitle:@"Spells"];menu.autoenablesItems=NO;
    for (const auto index:_session->learned_spells()) {
        const auto& spell=_session->spell_definition(index);const auto& cost=spell.casting;
        const auto cooldown=_session->spell_cooldown(index);
        NSString *name=[NSString stringWithUTF8String:spell.name.c_str()];
        NSString *detail=cost.cooldown?[NSString stringWithFormat:@"%d turns",cost.downtime]:
            [NSString stringWithFormat:@"%d mana · %d zorkmids",dredmor::spell_mana_cost(cost,_session->player().secondary[3],_session->player().primary[5]),
                dredmor::spell_gold_cost(cost,_session->player().primary[5],_session->depth_context())];
        NSMenuItem *item=[menu addItemWithTitle:[NSString stringWithFormat:@"%@ — %@%@",name,detail,
            cooldown>0?[NSString stringWithFormat:@" · ready in %d",cooldown]:@""] action:@selector(selectSpell:) keyEquivalent:@""];
        item.target=self;item.representedObject=@(index);item.enabled=cooldown<=0 && _session->player().alive();
    }
    if (!menu.numberOfItems) {NSMenuItem *empty=[menu addItemWithTitle:@"No learned spells" action:nil keyEquivalent:@""];empty.enabled=NO;}
    return menu;
}
- (void)showSpells:(id)sender {
    NSMenu *menu=[self spellMenu];
    [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(20,self.bounds.size.height-80) inView:self];
}
- (void)selectSpell:(id)sender {
    _targetSpell=[[sender representedObject] unsignedLongLongValue];
    if (_session->spell_definition(*_targetSpell).type=="self") [self castSelectedSpellAt:_session->player().position];
    self.needsDisplay=YES;
}
- (void)castSelectedSpellAt:(dredmor::Position)target {
    if (!_targetSpell) return;
    try {
        auto next=*_session;const auto result=next.use_spell(*_targetSpell,target);
        if (result==dredmor::SpellUse::cast) {[self commitCandidate:next refresh:NO];_targetSpell.reset();[self recordBreakableAnimations];}
        else {
            NSString *message=@"That spell cannot be used here.";
            switch (result) {
                case dredmor::SpellUse::insufficient_gold:message=@"You need more zorkmids.";break;
                case dredmor::SpellUse::insufficient_mana:message=@"You need more mana.";break;
                case dredmor::SpellUse::muted:message=@"You are prevented from casting spells.";break;
                case dredmor::SpellUse::equipment_required:message=@"Equip the required weapon or shield.";break;
                default:break;
            }
            NSAlert *alert=[NSAlert new];alert.messageText=message;[alert runModal];
        }
    } catch (const std::exception& error) {
        NSAlert *alert=[NSAlert new];alert.messageText=@"This spell is not available yet.";
        fprintf(stderr,"player spell failed: %s\n",error.what());[alert runModal];_targetSpell.reset();
    }
    [_inventoryController refresh];self.needsDisplay=YES;
}
- (void)presentShopNotices {
    for (const auto& event:_session->events()) if (event.kind==dredmor::EventKind::shop_presence && event.amount==int(dredmor::ShopPresenceEvent::theft_dialog)) {
        NSAlert *alert=[NSAlert new];alert.messageText=@"You have stolen merchandise from a shop! You're in trouble now...";[alert addButtonWithTitle:@"OK"];[alert runModal];
    }
}
- (void)showShopKeeper:(NSUInteger)index {
    const auto& shop=_session->shops().at(index);if (shop.angry || !shop.keeper.alive() || !_session->player().alive()) return;
    const float scaler=_content->tweaks().real(dredmor::Difficulty::medium,"store buy price scaler");int choice=0;
    if (_session->has_unpaid_goods()) {
        NSAlert *alert=[NSAlert new];alert.messageText=[NSString stringWithFormat:@"That'll be %d zorkmids. You have %d zorkmids. You want it or don'tcha?",_session->shop_debt(scaler),_session->player().zorkmids];
        [alert addButtonWithTitle:@"Don't Buy"];[alert addButtonWithTitle:@"Buy"];choice=[alert runModal]==NSAlertSecondButtonReturn?1:0;
    }
    auto next=*_session;const auto action=next.settle_shop_debt(index,choice,scaler);NSDictionary *presentation=[self preparePresentation:next];*_session=std::move(next);[self applyPresentation:presentation];[_inventoryController refresh];self.needsDisplay=YES;
    if (action.insufficient_funds) {NSAlert *alert=[NSAlert new];alert.messageText=@"You don't have the money, punk!";[alert addButtonWithTitle:@"OK"];[alert runModal];}
    else if (!action.debt_dialog) {NSAlert *alert=[NSAlert new];alert.messageText=@"You have no unpaid items.";[alert addButtonWithTitle:@"OK"];[alert runModal];}
}
- (void)showShopPedestal:(NSUInteger)shopIndex index:(NSUInteger)pedestalIndex {
    const auto& pedestal=_session->shops().at(shopIndex).pedestals.at(pedestalIndex);if (!pedestal.item) return;
    const auto& item=*pedestal.item;const auto& definition=_content->items().at(item.definition);
    const float scaler=_content->tweaks().real(dredmor::Difficulty::medium,"store buy price scaler");
    const auto quote=dredmor::resolve_shop_pedestal(true,dredmor::item_shop_cost(&definition,item),item.stackable,item.quantity,scaler,0,_session->player().zorkmids);
    NSString *name=[NSString stringWithUTF8String:(item.artifact?item.artifact->name:definition.name).c_str()];
    NSAlert *alert=[NSAlert new];alert.messageText=name;alert.informativeText=[NSString stringWithFormat:@"Price: %d zorkmids. You have %d.",quote.total_cost,_session->player().zorkmids];
    [alert addButtonWithTitle:@"Buy"];[alert addButtonWithTitle:@"Don't Buy"];[alert addButtonWithTitle:@"Take unpaid"];
    const auto response=[alert runModal];const int choice=response==NSAlertFirstButtonReturn?0:response==NSAlertThirdButtonReturn?2:1;
    auto next=*_session;const auto action=next.use_shop_pedestal(shopIndex,pedestalIndex,choice,scaler);
    NSDictionary *presentation=[self preparePresentation:next];*_session=std::move(next);[self applyPresentation:presentation];[_inventoryController refresh];self.needsDisplay=YES;
    if (action.insufficient_funds) {NSAlert *funds=[NSAlert new];funds.messageText=@"You don't have the money, punk!";[funds addButtonWithTitle:@"OK"];[funds runModal];}
}
- (void)mouseDown:(NSEvent*)event {
    const auto point=[self convertPoint:event.locationInWindow fromView:nil];const auto& world=_session->world();
    const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),(self.bounds.size.height-110)/world.height());
    if (cell<=0 || point.x<20 || point.y<20 || point.x>=20+cell*world.width() || point.y>=20+cell*world.height()) return;
    const dredmor::Position target{int((point.x-20)/cell),int((point.y-20)/cell)};
    if (_targetSpell) {[self castSelectedSpellAt:target];return;}
    try {
        if (_session->doors()) for (std::size_t i=0;i<_session->doors()->doors.size();++i) {
            const auto& door=_session->doors()->doors[i];if (door.destroyed) continue;
            for (std::size_t part=0;part<3;++part) if (door.assembly.parts[part].x==target.x && door.assembly.parts[part].y==target.y) {
                auto next=*_session;next.use_door(i,part);[self commitCandidate:next refresh:YES];
                [_inventoryController refresh];self.needsDisplay=YES;return;
            }
        }
        for (std::size_t i=0;i<_session->shops().size();++i) {
            const auto point=_session->shops()[i].keeper.position;
            if (point.x==target.x && point.y==target.y && std::abs(point.x-_session->player().position.x)+std::abs(point.y-_session->player().position.y)==1) {[self showShopKeeper:i];return;}
        }
        for (std::size_t i=0;i<_session->shops().size();++i) for (std::size_t j=0;j<_session->shops()[i].pedestals.size();++j) {
            const auto point=_session->shops()[i].pedestals[j].position;
            if (point.x==target.x && point.y==target.y && std::abs(point.x-_session->player().position.x)+std::abs(point.y-_session->player().position.y)==1) {[self showShopPedestal:i index:j];return;}
        }
        for (std::size_t i=0;i<_session->script_levers().size();++i) {
            const auto p=_session->script_levers()[i].object.position;if (p.x!=target.x || p.y!=target.y) continue;
            auto next=*_session;if (!next.use_script_lever(i)) return;
            [self commitCandidate:next refresh:YES];[_inventoryController refresh];self.needsDisplay=YES;return;
        }
        for (std::size_t i=0;i<_session->secret_levers().size();++i) {
            const auto p=_session->secret_levers()[i].position;if (p.x!=target.x || p.y!=target.y) continue;
            auto next=*_session;if (!next.use_secret_lever(i)) return;[self commitCandidate:next refresh:YES];
            [_inventoryController refresh];self.needsDisplay=YES;return;
        }
    } catch(const std::exception& error) {fprintf(stderr,"interaction failed: %s\n",error.what());}
    [super mouseDown:event];
}
- (void)recordBreakableAnimations {
    for (const auto& event:_session->events()) if (event.kind==dredmor::EventKind::object_broken) {
        const auto index=event.target-1;
        if (index>=_session->breakables().size() || index>=_breakableImages.count) throw std::runtime_error("breakable animation index invalid");
        const auto position=_session->breakables()[index].object.position;
        [_breakAnimations addObject:@{@"sprite":_breakableImages[index][1],@"x":@(position.x),@"y":@(position.y),@"start":@(NSProcessInfo.processInfo.systemUptime)}];
    }
}
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor colorWithWhite:0.06 alpha:1] setFill]; NSRectFill(self.bounds);
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];
    const auto& world=_session->world();
    const uint64_t animationTime=static_cast<uint64_t>((NSProcessInfo.processInfo.systemUptime-_animationStart)*1000);
    const CGFloat cell=MIN((self.bounds.size.width-40)/world.width(),
                           (self.bounds.size.height-110)/world.height());
    for (int y=0;y<world.height();++y) for (int x=0;x<world.width();++x) {
        unichar tile=[_rows[y] characterAtIndex:x];
        if (tile==' ') continue;
        BOOL wall=tile=='#';
        NSRect crop=wall?_wallCrop:_floorCrop;
        NSRect source=NSMakeRect(crop.origin.x,_atlas.size.height-crop.origin.y-crop.size.height,crop.size.width,crop.size.height);
        [_atlas drawInRect:NSMakeRect(20+x*cell,20+y*cell,cell,cell)
                 fromRect:source operation:NSCompositingOperationSourceOver fraction:1
           respectFlipped:YES hints:nil];
    }
    auto drawObject=[&](dredmor::Position position,NSImage *sprite) {
        const CGFloat width=sprite.size.width*cell/32,height=sprite.size.height*cell/32;
        [sprite drawInRect:NSMakeRect(20+position.x*cell+(cell-width)/2,20+(position.y+1)*cell-height,width,height)
            fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
    };
    for (std::size_t i=0;i<_session->floor_stairs().size();++i) drawObject(_session->floor_stairs()[i].position,[_stairImages[i] frameAt:animationTime]);
    for (std::size_t i=0;i<_session->script_levers().size();++i) drawObject(_session->script_levers()[i].object.position,[_scriptLeverImages[i] frameAtIndex:(_session->script_levers()[i].activation.activated()?1:0)]);
    for (std::size_t i=0;i<_session->secret_levers().size();++i) drawObject(_session->secret_levers()[i].position,[_leverImages[i] frameAtIndex:(_session->secret_levers()[i].activated?1:0)]);
    if (_session->doors()) for (std::size_t i=0;i<_session->doors()->doors.size();++i) {
        const auto& door=_session->doors()->doors[i];if (door.destroyed) continue;
        const auto frame=_session->doors()->frames[i];
        drawObject(door.assembly.parts[0],[_doorImages[i][0] frameAtIndex:frame]);
        drawObject(door.assembly.parts[2],[_doorImages[i][1] frameAtIndex:frame]);
    }
    for (std::size_t i=0;i<_session->shops().size();++i) {
        const auto& shop=_session->shops()[i];NSDictionary *images=_shopImages[i];
        drawObject(shop.keeper.position,[images[@"keeper"] frameAt:animationTime]);
        for (std::size_t j=0;j<shop.pedestals.size();++j) {
            drawObject(shop.pedestals[j].position,[images[@"pedestal"] frameAtIndex:0]);
            id image=images[@"stock"][j];if (image!=NSNull.null) drawObject(shop.pedestals[j].position,[image frameAtIndex:0]);
        }
    }
    for (std::size_t i=0;i<_session->custom_objects().size();++i) {
        const auto& object=_session->custom_objects()[i];
        drawObject(object.position,object.playing?[_objectImages[i] frameAt:animationTime]:[_objectImages[i] frameAtIndex:object.frame]);
    }
    for (std::size_t i=0;i<_session->breakables().size();++i) {
        const auto& object=_session->breakables()[i];const NSUInteger sprite=object.hits_taken>=object.hits_needed?2:0;
        drawObject(object.object.position,[_breakableImages[i][sprite] frameAtIndex:object.object.frame]);
    }
    for (std::size_t i=0;i<_session->traps().size();++i) {
        const auto& trap=_session->traps()[i];if (trap.has_origin_object) drawObject(trap.origin,[_trapImages[i][2] frameAt:animationTime]);
    }
    for (auto i:_session->detect_traps()) {
        const auto& trap=_session->traps()[i];drawObject(trap.position,[_trapImages[i][trap.sprite_revealed?1:0] frameAtIndex:0]);
    }
    for (NSDictionary *animation in [_breakAnimations copy]) {
        SpriteImage *sprite=animation[@"sprite"];
        const auto elapsed=static_cast<uint64_t>((NSProcessInfo.processInfo.systemUptime-[animation[@"start"] doubleValue])*1000);
        if (elapsed>=[sprite duration]) [_breakAnimations removeObjectIdenticalTo:animation];
        else drawObject({[animation[@"x"] intValue],[animation[@"y"] intValue]},[sprite frameAt:elapsed]);
    }
    // Item icon rendering and original item placement remain separate work.
    // This marker exposes explicit developer-scene drops to the pickup control.
    [[NSColor colorWithRed:.95 green:.7 blue:.15 alpha:1] setFill];
    for (const auto& ground:_session->ground_items())
        [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(20+(ground.position.x+.35)*cell,
            20+(ground.position.y+.35)*cell,cell*.3,cell*.3)] fill];
    for (const auto& enemy:_session->enemies()) if (enemy.alive()) {
        NSImage *sprite=[_enemyImages[@(enemy.id)] frameAt:animationTime];
        const CGFloat width=sprite.size.width*cell/32,height=sprite.size.height*cell/32;
        [sprite drawInRect:NSMakeRect(20+enemy.position.x*cell+(cell-width)/2,20+(enemy.position.y+1)*cell-height,width,height)
            fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
        [[NSColor colorWithRed:.7 green:.1 blue:.12 alpha:1] setFill];
        NSRectFill(NSMakeRect(20+enemy.position.x*cell,20+enemy.position.y*cell,cell*enemy.health/MAX(1,enemy.secondary[0]),3));
    }
    auto p=world.player();
    NSImage *hero=[_hero frameAt:animationTime];
    [hero drawInRect:NSMakeRect(20+(p.x-.5)*cell,20+(p.y-1)*cell,cell*2,cell*2)
             fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1
       respectFlipped:YES hints:nil];
    NSString *status=[NSString stringWithFormat:@"Floor %lld · Turn %llu · HP %d/%d · Mana %d/%d · Zorkmids %d\nArrows move/attack · . waits · G picks up · I inventory · A spells · ⌘S/⌘O%@",
                       static_cast<long long>(_session->depth_context().depth)+1,(unsigned long long)world.turn(),_session->player().health,_session->player().secondary[0],
                       _session->player().mana,_session->player().secondary[1],_session->player().zorkmids,
                       _session->player().alive()?@"":@" · You have died"];
    [status drawAtPoint:NSMakePoint(20,self.bounds.size.height-70)
        withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor,
                         NSFontAttributeName:[NSFont systemFontOfSize:13]}];
    if (_scriptMessages.count) [[_scriptMessages componentsJoinedByString:@" · "] drawAtPoint:NSMakePoint(20,self.bounds.size.height-44)
        withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor,NSFontAttributeName:[NSFont systemFontOfSize:11]}];
    NSMutableArray<NSString*> *buffLabels=[NSMutableArray array];
    if (_content) for (const auto& active:_session->player_buffs()) {
        if (buffLabels.count==3) break;
        const auto& spell=_content->spells().at(active.spell);const auto& definition=spell.buffs.at(active.buff);
        NSString *name=[NSString stringWithUTF8String:spell.name.c_str()]?:@"Buff";
        if (definition.use_timer) [buffLabels addObject:[NSString stringWithFormat:@"%@ (%lld turns)",name,static_cast<long long>(std::max<int64_t>(1,int64_t(active.remaining)+1))]];
        else [buffLabels addObject:name];
    }
    if (_session->player_buffs().size()>buffLabels.count) [buffLabels addObject:[NSString stringWithFormat:@"+%zu more",_session->player_buffs().size()-buffLabels.count]];
    NSString *buffStatus=_targetSpell?[NSString stringWithFormat:@"Click a target for %@ · Esc cancels",[NSString stringWithUTF8String:_session->spell_definition(*_targetSpell).name.c_str()]]:buffLabels.count?[NSString stringWithFormat:@"Buffs: %@",[buffLabels componentsJoinedByString:@" · "]]:@"Buffs: none";
    if (!_targetSpell && _session->regeneration()) {
        const auto& r=*_session->regeneration();
        if (r.food>0 || r.booze>0) buffStatus=[NSString stringWithFormat:@"Food: %d · Booze: %d · %@",std::max(0,r.food),std::max(0,r.booze),buffStatus];
    }
    if (!_targetSpell && _session->last_door_click() && !_session->last_door_click()->messages.empty()) buffStatus=[NSString stringWithUTF8String:_session->last_door_click()->messages.back().c_str()];
    NSMutableParagraphStyle *buffStyle=[[NSMutableParagraphStyle alloc] init];buffStyle.lineBreakMode=NSLineBreakByTruncatingTail;
    [buffStatus drawInRect:NSMakeRect(20,self.bounds.size.height-28,MAX(0,self.bounds.size.width-40),18)
        withAttributes:@{NSForegroundColorAttributeName:NSColor.lightGrayColor,NSFontAttributeName:[NSFont systemFontOfSize:12],NSParagraphStyleAttributeName:buffStyle}];
}
@end

@interface AppDelegate : NSObject<NSApplicationDelegate,NSWindowDelegate>
@property(strong) NSWindow *window;
@end
@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender { return YES; }
- (void)windowWillClose:(NSNotification*)notification {
    if (notification.object==self.window) [NSApp terminate:nil];
}
@end

static NSDictionary *sceneFromInstallation(const std::filesystem::path& selected,BOOL selectSkills,
                                          std::chrono::steady_clock::time_point startup) {
    auto root=std::filesystem::canonical(selected);
    if (std::filesystem::is_directory(root/"Contents/MacOS")) root/="Contents/MacOS";
    const auto content=dredmor::GameContent::load(root);
    // Both streams share an elapsed-millisecond seed, as in DoRandomize. Exact
    // original startup timing is external input; smoke uses a fixed seed.
    const auto seed=selectSkills?static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-startup).count()):0xD0ED;
    const auto& room=content.room("Starting Room");
    const auto world=room.instantiate(false);
    NSMutableArray<NSString*> *rows=[NSMutableArray array];
    for (const auto& row:room.rows) [rows addObject:[NSString stringWithUTF8String:row.c_str()]];
    auto path=[&](const std::filesystem::path& value) { return [NSString stringWithUTF8String:value.c_str()]; };
    NSMutableDictionary *scene=[@{@"name":@"Starting Room",@"policy":@"provisional-preview",@"rows":rows,
        @"collision":[NSString stringWithUTF8String:world.room_text().c_str()],
        @"atlas":path(roomResource(root,"tilesets/basic.png")),@"hero":path(roomResource(root,"sprites/hero/hero_idle_d.spr")),
        @"data_root":path(root),@"seed":@(seed),@"starting_loadout":@YES,@"room_objects":@"Starting Room",
        @"floor_crop":@[@0,@192,@32,@32],@"wall_crop":@[@0,@96,@32,@32]} mutableCopy];
    if (selectSkills) {
        DredmorSkillSelectionController *controller=[[DredmorSkillSelectionController alloc] initWithSkills:content.skills()];
        NSArray<NSNumber*> *skills=[controller runSelection];
        if (!skills) return nil;
        scene[@"skills"]=skills;
    }
    return scene;
}

int main(int argc,const char**argv) {
    const auto startup=std::chrono::steady_clock::now();
    @autoreleasepool {
        const bool smoke=argc==3 && std::strcmp(argv[2],"--smoke-test")==0;
        if (argc!=1 && argc!=2 && !smoke) {
            fprintf(stderr,"Usage: DredmorMac [INSTALLATION_OR_SCENE [--smoke-test]]\n"); return 2;
        }
        NSApplication *app=[NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        RoomView *view=nil;NSDictionary *scene=nil;
        while (!view) {
            NSString *selected=nil;
            if (argc==1) {
                [app activateIgnoringOtherApps:YES];
                NSOpenPanel *panel=[NSOpenPanel openPanel];
                panel.title=@"Open Dungeons of Dredmor Installation";
                panel.message=@"Choose Dungeons of Dredmor.app or its Contents/MacOS folder to open the development room view.";
                panel.canChooseFiles=YES;panel.canChooseDirectories=YES;panel.allowsMultipleSelection=NO;
                if ([panel runModal]!=NSModalResponseOK) return 0;
                selected=panel.URL.path;
            } else selected=[NSString stringWithUTF8String:argv[1]];
            try {
                const std::filesystem::path path(selected.UTF8String);
                if (std::filesystem::is_directory(path)) {
                    scene=sceneFromInstallation(path,!smoke,startup);
                    if (!scene) return 0;
                }
                else {
                    NSData *data=[NSData dataWithContentsOfFile:selected];
                    scene=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nullptr]:nil;
                }
                if (![scene isKindOfClass:NSDictionary.class] || ![scene[@"collision"] isKindOfClass:NSString.class]
                    || ![scene[@"rows"] isKindOfClass:NSArray.class] || ![scene[@"atlas"] isKindOfClass:NSString.class]
                    || ![scene[@"hero"] isKindOfClass:NSString.class]) throw std::runtime_error("invalid installation or scene file");
                view=[[RoomView alloc] initWithScene:scene];
                if (!view) throw std::runtime_error("cannot load room layout or scene images");
            } catch (const std::exception& error) {
                fprintf(stderr,"%s\n",error.what());
                if (argc!=1) return 1;
                NSAlert *alert=[NSAlert new];alert.messageText=@"Cannot open installation";
                alert.informativeText=[NSString stringWithUTF8String:error.what()];[alert runModal];
            }
        }
        NSString *expectedSnapshot=scene[@"smoke_expected"];
        if (smoke && ![expectedSnapshot isKindOfClass:NSString.class]) {
            std::istringstream input([scene[@"collision"] UTF8String]);auto expected=dredmor::World::read(input);
            expected.step(dredmor::Action::east);expected.step(dredmor::Action::wait);
            expectedSnapshot=[NSString stringWithUTF8String:expected.snapshot().c_str()];
        }
        NSMenu *menu=[NSMenu new];
        NSMenuItem *appMenuItem=[NSMenuItem new];[menu addItem:appMenuItem];
        NSMenu *appMenu=[[NSMenu alloc] initWithTitle:@"DredmorMac"];
        [appMenu addItemWithTitle:@"Quit DredmorMac" action:@selector(terminate:) keyEquivalent:@"q"];
        appMenuItem.submenu=appMenu;
        NSMenuItem *fileMenuItem=[NSMenuItem new];[menu addItem:fileMenuItem];
        NSMenu *fileMenu=[[NSMenu alloc] initWithTitle:@"File"];
        NSMenuItem *open=[fileMenu addItemWithTitle:@"Load Session…" action:@selector(openDocument:) keyEquivalent:@"o"];
        NSMenuItem *save=[fileMenu addItemWithTitle:@"Save Session…" action:@selector(saveDocument:) keyEquivalent:@"s"];
        open.target=view;save.target=view;fileMenuItem.submenu=fileMenu;app.mainMenu=menu;
        NSMenuItem *gameMenuItem=[NSMenuItem new];[menu addItem:gameMenuItem];
        NSMenu *gameMenu=[[NSMenu alloc] initWithTitle:@"Character"];
        NSMenuItem *inventory=[gameMenu addItemWithTitle:@"Inventory…" action:@selector(showInventory:) keyEquivalent:@"i"];
        inventory.target=view;gameMenuItem.submenu=gameMenu;
        NSMenuItem *spells=[gameMenu addItemWithTitle:@"Spells…" action:@selector(showSpells:) keyEquivalent:@"a"];spells.target=view;
        static AppDelegate *delegate;
        delegate=[AppDelegate new]; app.delegate=delegate;
        delegate.window=[[NSWindow alloc] initWithContentRect:view.frame
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered defer:NO];
        delegate.window.title=@"DredmorMac — Room preview";
        delegate.window.delegate=delegate;
        delegate.window.contentMinSize=NSMakeSize(300,360);
        delegate.window.contentView=view;
        [delegate.window center]; [delegate.window makeKeyAndOrderFront:nil];
        [delegate.window makeFirstResponder:view];
        [app activateIgnoringOtherApps:YES];
        if (smoke) {
            // This mode is for the generated synthetic fixture, not game parity.
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{
                NSEvent *right=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
                    modifierFlags:0 timestamp:0 windowNumber:delegate.window.windowNumber context:nil
                    characters:@"\uf703" charactersIgnoringModifiers:@"\uf703" isARepeat:NO keyCode:124];
                NSEvent *wait=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
                    modifierFlags:0 timestamp:0 windowNumber:delegate.window.windowNumber context:nil
                    characters:@"." charactersIgnoringModifiers:@"." isARepeat:NO keyCode:47];
                [view keyDown:right]; [view keyDown:wait]; [view displayIfNeeded];
                if (![[view snapshot] isEqualToString:expectedSnapshot] || ([scene[@"smoke_combat"] boolValue] && [view hasLivingEnemies]) ||
                    ([scene[@"smoke_animation"] boolValue] && ![view hasAnimatedHero]) ||
                    ([scene[@"smoke_inventory"] boolValue] && ![view smokeInventory]) ||
                    ([scene[@"smoke_artifact"] boolValue] && ![view smokeArtifact]) ||
                    ([scene[@"smoke_room_monsters"] boolValue] && ![view smokeRoomMonsters]) ||
                    ([scene[@"smoke_hordes"] boolValue] && ![view smokeHordes]) ||
                    ([scene[@"smoke_script_hordes"] boolValue] && ![view hasScriptedHordes]) ||
                    ([scene[@"smoke_floors"] boolValue] && ![view smokeFloors]) ||
                    ([scene[@"smoke_secret_levers"] boolValue] && ![view smokeSecretLevers]) ||
                    ([scene[@"smoke_script_levers"] boolValue] && ![view smokeScriptLevers]) ||
                    ([scene[@"smoke_room_scripts"] boolValue] && ![view smokeRoomScripts]) ||
                    ([scene[@"smoke_doors"] boolValue] && ![view smokeDoors]) ||
                    ([scene[@"smoke_decorations"] boolValue] && ![view smokeDecorations]) ||
                    ([scene[@"smoke_automatic_armour"] boolValue] && ![view smokeAutomaticArmour]) ||
                    ([scene[@"smoke_shops"] boolValue] && ![view smokeShops]) ||
                    ([scene[@"smoke_food"] boolValue] && ![view smokeFood]) ||
                    ([scene[@"smoke_regeneration"] boolValue] && ![view smokeRegeneration]) ||
                    ([scene[@"smoke_casting"] boolValue] && ![view smokeCasting]) ||
                    ([scene[@"smoke_traps"] boolValue] && ![view smokeTraps]) ||
                    ([scene[@"smoke_objects"] boolValue] && ![view smokeCustomObjects]) ||
                    ([scene[@"smoke_breakables"] boolValue] && ![view smokeBreakables]) ||
                    ![view smokeSaveLoad] || ![view smokeSkillSelection]) {
                    fprintf(stderr,"native smoke failed: %s\n",[[view snapshot] UTF8String]); std::exit(1);
                }
                puts("native smoke: animated SPR, draw, movement, combat and atomic save/load passed");
                [app terminate:nil];
            });
        }
        [app run];
    }
    return 0;
}
