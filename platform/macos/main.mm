#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "InventoryController.h"
#import "SkillSelectionController.h"
#include <dispatch/dispatch.h>
#include <cstring>
#include <cstdlib>
#include "dredmor/content.hpp"
#include "dredmor/character.hpp"
#include "dredmor/sprite.hpp"
#include "dredmor/resources.hpp"
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
- (instancetype)initWithRoomSprite:(const dredmor::RoomSprite&)sprite root:(const std::filesystem::path&)root;
- (NSImage*)frameAt:(uint64_t)milliseconds;
- (NSImage*)frameAtIndex:(uint32_t)index;
- (NSUInteger)frameCount;
- (uint64_t)duration;
- (void)setSingleFrameDelay:(uint16_t)delay;
@end
@implementation SpriteImage
- (instancetype)initWithPath:(NSString*)path {
    self=[super init];
    if (self) {
        if ([[path.pathExtension lowercaseString] isEqualToString:@"spr"]) {
            _sprite=dredmor::Sprite::read(path.UTF8String);
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
- (uint64_t)duration {
    if (_sprite.frames.empty()) return MAX(uint64_t(1),_seriesDelay)*_images.count;
    uint64_t result=0;for (const auto& frame:_sprite.frames) result+=MAX(uint64_t(1),uint64_t(frame.delay_ms));return result;
}
- (void)setSingleFrameDelay:(uint16_t)delay {
    if (_images.count!=1) throw std::runtime_error("single-frame delay requires one frame");
    if (_sprite.frames.empty()) _seriesDelay=delay;else _sprite.frames[0].delay_ms=delay;
}
@end

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

@interface RoomView : NSView<DredmorInventoryOwner> {
    std::unique_ptr<dredmor::Session> _session;
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
    NSRect _floorCrop,_wallCrop;
}
- (instancetype)initWithScene:(NSDictionary*)scene;
- (NSString*)snapshot;
- (BOOL)hasLivingEnemies;
- (BOOL)hasAnimatedHero;
- (void)saveToURL:(NSURL*)url;
- (void)loadFromURL:(NSURL*)url;
- (BOOL)smokeSaveLoad;
- (void)saveDocument:(id)sender;
- (void)openDocument:(id)sender;
- (void)showInventory:(id)sender;
- (void)pickUpItems:(id)sender;
- (BOOL)smokeInventory;
- (BOOL)smokeRoomMonsters;
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
        if (_content) _session->spell_profile(std::make_shared<const std::vector<dredmor::SpellDefinition>>(_content->spells()));
        NSMutableArray<NSArray<SpriteImage*>*> *breakables=[NSMutableArray array];
        if (scene[@"room_objects"]) {
            if (!_content || ![scene[@"room_objects"] isKindOfClass:NSString.class]) throw std::runtime_error("room objects need a content room name");
            const auto& room=_content->room([scene[@"room_objects"] UTF8String]);
            const auto numeric=room.numeric_template();dredmor::TrapMap trapMap{numeric.width,numeric.height,{}};
            for (const auto& cell:numeric.cells) trapMap.tiles.push_back(cell.kind);
            // Preview builds supported objects/loot in relative source order.
            // Other constructors/RNG and full population still belong to level generation.
            for (const auto& record:room.records()) std::visit([&](const auto& definition) {
                using T=std::decay_t<decltype(definition)>;
                if constexpr (std::is_same_v<T,dredmor::CustomRoomDefinition>) _session->create_custom_object(definition);
                else if constexpr (std::is_same_v<T,dredmor::RoomPedestalRecord>) _session->create_pedestal(definition);
                else if constexpr (std::is_same_v<T,dredmor::RoomLootRecord>)
                    _session->create_room_loot(definition,_content->item_factory_tables(),_content->loot_context());
                else if constexpr (std::is_same_v<T,dredmor::RoomMonsterRecord>) {
                    _session->create_room_monster(definition,[&](const auto& plan,auto& random,auto& state) {
                        return _content->create_room_monster(plan,{},dredmor::Difficulty::medium,random,state);
                    });
                }
                else if constexpr (std::is_same_v<T,dredmor::RoomTrapRecord>)
                    _session->create_room_trap(definition,_content->item_factory_tables(),_content->loot_context(),trapMap);
                else if constexpr (std::is_same_v<T,dredmor::BreakableRoomDefinition>) {
                    NSArray<SpriteImage*> *prepared=nil;
                    const auto created=_session->create_breakable(definition,{0,0},[&](const auto& value) {
                        prepared=breakableSprites(value.intact,value.breaking.value_or(value.intact),value.broken,_dataRoot);return breakableFrames(prepared);
                    });
                    if (created) {
                        const auto delay=_session->breakables()[*created].single_breaking_delay;
                        if (delay) [prepared[1] setSingleFrameDelay:delay];[breakables addObject:prepared];
                    }
                }
            },record);
        }
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
            const auto path=roomResource(_dataRoot,enemy.sprite);
            SpriteImage *image=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
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
            const auto identifier=_session->spawn_enemy(std::move(enemy));
            NSString *spritePath=[value[@"animation"] isKindOfClass:NSString.class]?value[@"animation"]:value[@"image"];
            SpriteImage *sprite=[[SpriteImage alloc] initWithPath:spritePath];
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
    std::ostringstream output;_session->save(output);
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
    auto loaded=std::make_unique<dredmor::Session>(dredmor::Session::load(input,definitions,abilities,spells));
    // Current renderer supports a single prepared room. Reject another layout
    // before replacing either the session or its images.
    const auto& old=_session->world();const auto& world=loaded->world();
    if (old.width()!=world.width() || old.height()!=world.height())
        throw std::runtime_error("open the save's prepared room before loading this file");
    for (int y=0;y<old.height();++y) for (int x=0;x<old.width();++x)
        if (old.tile(x,y)!=world.tile(x,y)) throw std::runtime_error("save belongs to another room layout");
    NSMutableDictionary<NSNumber*,SpriteImage*> *images=[NSMutableDictionary dictionary];
    for (const auto& enemy:loaded->enemies()) {
        if (_content) {
            const auto definition=_content->make_monster(enemy.name,enemy.position,dredmor::Difficulty::medium);
            if (definition.sprite.empty()) throw std::runtime_error("saved monster has no idle sprite");
            const auto path=roomResource(_dataRoot,definition.sprite);
            images[@(enemy.id)]=[[SpriteImage alloc] initWithPath:[NSString stringWithUTF8String:path.c_str()]];
        } else {
            SpriteImage *image=_enemyImages[@(enemy.id)];
            if (!image) throw std::runtime_error("saved actor image is unavailable in this scene");
            images[@(enemy.id)]=image;
        }
    }
    NSMutableArray<SpriteImage*> *objects=[NSMutableArray array];
    for (const auto& object:loaded->custom_objects()) {
        SpriteImage *image=[[SpriteImage alloc] initWithRoomSprite:object.sprite root:_dataRoot];
        if (!image) throw std::runtime_error("cannot prepare saved room object sprite");
        [objects addObject:image];
    }
    NSMutableArray<NSArray<SpriteImage*>*> *breakables=[NSMutableArray array];
    for (const auto& object:loaded->breakables()) {
        NSArray<SpriteImage*> *prepared=breakableSprites(object.intact,object.breaking,object.broken,_dataRoot);
        const auto frames=breakableFrames(prepared);
        if (frames.intact!=object.frames.intact || frames.breaking!=object.frames.breaking || frames.broken!=object.frames.broken)
            throw std::runtime_error("saved breakable frames do not match resources");
        if (object.single_breaking_delay) [prepared[1] setSingleFrameDelay:object.single_breaking_delay];[breakables addObject:prepared];
    }
    NSMutableArray<NSArray<SpriteImage*>*> *traps=[NSMutableArray array];
    for (const auto& trap:loaded->traps()) [traps addObject:trapSprites(trap,_content->items().at(trap.definition),_dataRoot)];
    _session=std::move(loaded);_enemyImages=images;_objectImages=objects;_breakableImages=breakables;_trapImages=traps;[_breakAnimations removeAllObjects];
    [_inventoryController refresh];self.needsDisplay=YES;
}
- (void)showFailure:(const std::exception&)error {
    NSAlert *alert=[NSAlert new];alert.messageText=@"Cannot complete action";
    alert.informativeText=[NSString stringWithUTF8String:error.what()];[alert runModal];
}
- (void)saveDocument:(id)sender {
    NSSavePanel *panel=[NSSavePanel savePanel];
    panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"dredmor-save" conformingToType:UTTypeXML]];
    panel.nameFieldStringValue=@"Encounter.dredmor-save";
    panel.message=@"Save this port's session. Original game saves use a different format.";
    if ([panel runModal]!=NSModalResponseOK) return;
    try { [self saveToURL:panel.URL]; } catch (const std::exception& error) { [self showFailure:error]; }
}
- (void)openDocument:(id)sender {
    NSOpenPanel *panel=[NSOpenPanel openPanel];
    panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"dredmor-save" conformingToType:UTTypeXML]];
    panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;
    panel.message=@"Load a port session for the currently prepared room.";
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
- (NSArray<NSDictionary*>*)inventoryRows {
    NSMutableArray<NSDictionary*> *rows=[NSMutableArray array];
    auto append=[&](const dredmor::InventoryItem& item,NSString *location,BOOL equipped,NSUInteger slot) {
        NSString *name=item.artifact?[NSString stringWithUTF8String:item.artifact->name.c_str()]:_content && item.definition<_content->items().size()?
            [NSString stringWithUTF8String:_content->items()[item.definition].name.c_str()]:
            [NSString stringWithFormat:@"Item %u",item.definition];
        if (item.charges) name=[NSString stringWithFormat:@"%@ (%d charges)",name,*item.charges];
        [rows addObject:@{@"name":name,@"quantity":@(item.quantity),@"location":location,@"equipped":@(equipped),
            @"instance":@(item.instance),@"slot":@(slot)}];
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
- (void)pickUpItems:(id)sender {
    try { _session->pickup();[_inventoryController refresh];self.needsDisplay=YES; }
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
    if (_session->enemies().size()!=2 || _enemyImages.count!=2) return NO;
    const auto& named=_session->enemies()[0];const auto& random=_session->enemies()[1];
    const auto count=_session->monster_spawn_state().counts.find("Fixture variant");
    return named.room_spawned && named.room_local_id=="fixture-monster" && named.can_push==0 && !named.sprite.empty() &&
        random.room_spawned && random.room_local_id=="fixture-random" && random.can_push==255 && !random.sprite.empty() &&
        !(random.monster_flags&~10U) && count!=_session->monster_spawn_state().counts.end() && count->second==1;
}
- (BOOL)smokeTraps {
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:
        [NSUUID.UUID.UUIDString stringByAppendingString:@".dredmor-save"]]];
    try {
        if (_session->traps().size()!=2 || _trapImages.count!=2) return NO;
        const auto& ceiling=_session->traps()[0];const auto& floor=_session->traps()[1];
        if (ceiling.charges!=1 || !ceiling.discovered || ceiling.pending_destruction || ceiling.room_local_id!="fixture-trap" || ceiling.can_push!=0 ||
            _session->player().health!=_session->player().secondary[0]-7 || !floor.has_origin_object || floor.origin.x!=1 || floor.origin.y!=2 ||
            _session->passable(floor.origin) || !_session->world().passable(floor.origin) || [_trapImages[0][1] frameCount]!=2 || [_trapImages[1][2] frameCount]!=2) return NO;
        if (![self smokeSaveLoad]) return NO;
        std::ostringstream before;_session->save(before);NSArray *images=_trapImages;std::vector<std::string> invalid;
        auto wrongSpell=before.str();auto start=wrongSpell.find("<spell-rules content=\"");
        if (start==std::string::npos) return NO;start+=std::string("<spell-rules content=\"").size();const auto end=wrongSpell.find('"',start);
        wrongSpell.replace(start,end-start,"1");invalid.push_back(std::move(wrongSpell));
        auto badOrigin=before.str();start=badOrigin.find("origin-object=\"0\"");if (start==std::string::npos) return NO;
        badOrigin.replace(start,std::string("origin-object=\"0\"").size(),"origin-object=\"1\"");invalid.push_back(std::move(badOrigin));
        for (const auto& bytes:invalid) {
            NSData *data=[NSData dataWithBytes:bytes.data() length:bytes.size()];if (![data writeToURL:url options:NSDataWritingAtomic error:nullptr]) return NO;
            bool failed=false;try {[self loadFromURL:url];} catch(const std::exception&) {failed=true;}
            std::ostringstream after;_session->save(after);if (!failed || after.str()!=before.str() || images!=_trapImages) return NO;
        }
        [[NSFileManager defaultManager] removeItemAtURL:url error:nullptr];
        for (NSString *key in @[@"\uf702",@"\uf703"]) {
            NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0
                windowNumber:self.window.windowNumber context:nil characters:key charactersIgnoringModifiers:key isARepeat:NO keyCode:0];[self keyDown:event];
        }
        return _session->traps()[0].charges==0 && _session->traps()[0].pending_destruction &&
            _session->player().health==_session->player().secondary[0]-14 && !_session->passable(_session->traps()[1].origin);
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
- (void)keyDown:(NSEvent*)event {
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
            else if ([event.charactersIgnoringModifiers isEqualToString:@"."]) action=Action::wait;
            else { [super keyDown:event]; return; }
    }
    try { _session->step(action);[self recordBreakableAnimations]; }
    catch (const std::exception& e) { fprintf(stderr,"turn failed: %s\n",e.what()); }
    [_inventoryController refresh];
    self.needsDisplay=YES;
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
                           (self.bounds.size.height-90)/world.height());
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
    NSString *status=[NSString stringWithFormat:@"Turn %llu · HP %d/%d · Mana %d/%d · Zorkmids %d\nArrows move/attack · . waits · G picks up · I inventory · ⌘S/⌘O%@",
                       (unsigned long long)world.turn(),_session->player().health,_session->player().secondary[0],
                       _session->player().mana,_session->player().secondary[1],_session->player().zorkmids,
                       _session->player().alive()?@"":@" · You have died"];
    [status drawAtPoint:NSMakePoint(20,self.bounds.size.height-55)
        withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor,
                         NSFontAttributeName:[NSFont systemFontOfSize:13]}];
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
