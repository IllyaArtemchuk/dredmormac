#import <Cocoa/Cocoa.h>
#include <dispatch/dispatch.h>
#include <cstring>
#include <cstdlib>
#include "dredmor/content.hpp"
#include <memory>
#include <sstream>
#include <stdexcept>

@interface RoomView : NSView {
    std::unique_ptr<dredmor::Session> _session;
    std::unique_ptr<dredmor::GameContent> _content;
    NSArray<NSString*> *_rows;
    NSImage *_atlas;
    NSImage *_hero;
    NSMutableDictionary<NSNumber*,NSImage*> *_enemyImages;
    NSRect _floorCrop,_wallCrop;
}
- (instancetype)initWithScene:(NSDictionary*)scene;
- (NSString*)snapshot;
- (BOOL)hasLivingEnemies;
@end

@implementation RoomView
- (instancetype)initWithScene:(NSDictionary*)scene {
    self = [super initWithFrame:NSMakeRect(0,0,600,650)];
    if (self) {
        std::istringstream input([scene[@"collision"] UTF8String]);
        auto world=dredmor::World::read(input);
        dredmor::ClassRanks ranks={3,2,2};
        int experience=190;
        if ([scene[@"data_root"] isKindOfClass:NSString.class]) {
            _content=std::make_unique<dredmor::GameContent>(dredmor::GameContent::load([scene[@"data_root"] UTF8String]));
            std::vector<int> selected;
            if ([scene[@"skills"] isKindOfClass:NSArray.class]) {
                for (id value in scene[@"skills"]) {
                    if (![value isKindOfClass:NSNumber.class]) throw std::runtime_error("invalid selected skill ID");
                    selected.push_back([value intValue]);
                }
            } else for (const auto& skill:_content->skills()) if (selected.size()<7) selected.push_back(skill.id);
            ranks=_content->starting_ranks(selected);
            experience=_content->tweaks().integer(dredmor::Difficulty::medium,"experience n");
        }
        _session=std::make_unique<dredmor::Session>(std::move(world),dredmor::make_player(ranks),0xD0ED,experience);
        if (_content) _session->equipment_profile(ranks,std::make_shared<const std::vector<dredmor::ItemDefinition>>(_content->items()));
        _rows = scene[@"rows"];
        if (_rows.count != (NSUInteger)_session->world().height()) return nil;
        for (id row in _rows)
            if (![row isKindOfClass:NSString.class] || [row length] != (NSUInteger)_session->world().width()) return nil;
        _atlas = [[NSImage alloc] initWithContentsOfFile:scene[@"atlas"]];
        _hero = [[NSImage alloc] initWithContentsOfFile:scene[@"hero"]];
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
            NSImage *sprite=[[NSImage alloc] initWithContentsOfFile:value[@"image"]];
            if (!sprite) return nil;
            _enemyImages[@(identifier)]=sprite;
        }
    }
    return self;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isFlipped { return YES; }
- (NSString*)snapshot { return [NSString stringWithUTF8String:_session->world().snapshot().c_str()]; }
- (BOOL)hasLivingEnemies {
    for (const auto& enemy:_session->enemies()) if (enemy.alive()) return YES;
    return NO;
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
            if ([event.charactersIgnoringModifiers isEqualToString:@"."]) action=Action::wait;
            else { [super keyDown:event]; return; }
    }
    try { _session->step(action); }
    catch (const std::exception& e) { fprintf(stderr,"turn failed: %s\n",e.what()); }
    self.needsDisplay=YES;
}
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor colorWithWhite:0.06 alpha:1] setFill]; NSRectFill(self.bounds);
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];
    const auto& world=_session->world();
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
    for (const auto& enemy:_session->enemies()) if (enemy.alive()) {
        NSImage *sprite=_enemyImages[@(enemy.id)];
        const CGFloat width=sprite.size.width*cell/32,height=sprite.size.height*cell/32;
        [sprite drawInRect:NSMakeRect(20+enemy.position.x*cell+(cell-width)/2,20+(enemy.position.y+1)*cell-height,width,height)
            fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
        [[NSColor colorWithRed:.7 green:.1 blue:.12 alpha:1] setFill];
        NSRectFill(NSMakeRect(20+enemy.position.x*cell,20+enemy.position.y*cell,cell*enemy.health/MAX(1,enemy.secondary[0]),3));
    }
    auto p=world.player();
    [_hero drawInRect:NSMakeRect(20+(p.x-.5)*cell,20+(p.y-1)*cell,cell*2,cell*2)
             fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1
       respectFlipped:YES hints:nil];
    NSString *status=[NSString stringWithFormat:@"Turn %llu · HP %d/%d · Mana %d/%d\nArrow keys move or attack · . waits%@",
                       (unsigned long long)world.turn(),_session->player().health,_session->player().secondary[0],
                       _session->player().mana,_session->player().secondary[1],_session->player().alive()?@"":@" · You have died"];
    [status drawAtPoint:NSMakePoint(20,self.bounds.size.height-55)
        withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor,
                         NSFontAttributeName:[NSFont systemFontOfSize:13]}];
}
@end

@interface AppDelegate : NSObject<NSApplicationDelegate>
@property(strong) NSWindow *window;
@end
@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender { return YES; }
@end

int main(int argc,const char**argv) {
    @autoreleasepool {
        const bool smoke=argc==3 && std::strcmp(argv[2],"--smoke-test")==0;
        if (argc!=2 && !smoke) { fprintf(stderr,"Usage: DredmorMac scene.json [--smoke-test]\n"); return 2; }
        NSError *error=nil;
        NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
        NSDictionary *scene=data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:&error] : nil;
        if (![scene isKindOfClass:NSDictionary.class] || ![scene[@"collision"] isKindOfClass:NSString.class]
            || ![scene[@"rows"] isKindOfClass:NSArray.class]
            || ![scene[@"atlas"] isKindOfClass:NSString.class] || ![scene[@"hero"] isKindOfClass:NSString.class]) {
            fprintf(stderr,"Invalid scene; run scripts/prepare_scene.py first\n"); return 1;
        }
        NSApplication *app=[NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        RoomView *view=nil;
        try { view=[[RoomView alloc] initWithScene:scene]; }
        catch (const std::exception& e) { fprintf(stderr,"%s\n",e.what()); return 1; }
        if (!view) { fprintf(stderr,"Cannot load scene images\n"); return 1; }
        static AppDelegate *delegate;
        delegate=[AppDelegate new]; app.delegate=delegate;
        delegate.window=[[NSWindow alloc] initWithContentRect:view.frame
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered defer:NO];
        delegate.window.title=@"DredmorMac — Room preview";
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
                NSString *expected=[scene[@"smoke_expected"] isKindOfClass:NSString.class]?scene[@"smoke_expected"]:@"{\"turn\":2,\"x\":2,\"y\":1}";
                if (![[view snapshot] isEqualToString:expected] || ([scene[@"smoke_combat"] boolValue] && [view hasLivingEnemies])) {
                    fprintf(stderr,"native smoke failed: %s\n",[[view snapshot] UTF8String]); std::exit(1);
                }
                puts("native smoke: window, draw, movement and wait passed");
                [app terminate:nil];
            });
        }
        [app run];
    }
    return 0;
}
