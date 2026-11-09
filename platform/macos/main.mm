#import <Cocoa/Cocoa.h>
#include "dredmor/world.hpp"
#include <memory>
#include <sstream>

@interface RoomView : NSView {
    std::unique_ptr<dredmor::World> _world;
    NSArray<NSString*> *_rows;
    NSImage *_atlas;
    NSImage *_hero;
}
- (instancetype)initWithScene:(NSDictionary*)scene;
@end

@implementation RoomView
- (instancetype)initWithScene:(NSDictionary*)scene {
    self = [super initWithFrame:NSMakeRect(0,0,600,650)];
    if (self) {
        std::istringstream input([scene[@"collision"] UTF8String]);
        _world = std::make_unique<dredmor::World>(dredmor::World::read(input));
        _rows = scene[@"rows"];
        if (_rows.count != (NSUInteger)_world->height()) return nil;
        for (id row in _rows)
            if (![row isKindOfClass:NSString.class] || [row length] != (NSUInteger)_world->width()) return nil;
        _atlas = [[NSImage alloc] initWithContentsOfFile:scene[@"atlas"]];
        _hero = [[NSImage alloc] initWithContentsOfFile:scene[@"hero"]];
        if (!_atlas || !_hero) return nil;
    }
    return self;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isFlipped { return YES; }
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
    _world->step(action);
    self.needsDisplay=YES;
}
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor colorWithWhite:0.06 alpha:1] setFill]; NSRectFill(self.bounds);
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];
    const CGFloat cell=MIN((self.bounds.size.width-40)/_world->width(),
                           (self.bounds.size.height-90)/_world->height());
    for (int y=0;y<_world->height();++y) for (int x=0;x<_world->width();++x) {
        unichar tile=[_rows[y] characterAtIndex:x];
        if (tile==' ') continue;
        BOOL wall=tile=='#';
        NSRect source=NSMakeRect(0,_atlas.size.height-(wall?96:192)-32,32,32);
        [_atlas drawInRect:NSMakeRect(20+x*cell,20+y*cell,cell,cell)
                 fromRect:source operation:NSCompositingOperationSourceOver fraction:1
           respectFlipped:YES hints:nil];
    }
    auto p=_world->player();
    [_hero drawInRect:NSMakeRect(20+(p.x-.5)*cell,20+(p.y-1)*cell,cell*2,cell*2)
             fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1
       respectFlipped:YES hints:nil];
    NSString *status=[NSString stringWithFormat:@"Starting Room preview · Turn %llu\nArrow keys move · . waits · gameplay policies provisional",
                       (unsigned long long)_world->turn()];
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
        if (argc!=2) { fprintf(stderr,"Usage: DredmorMac scene.json\n"); return 2; }
        NSError *error=nil;
        NSData *data=[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
        NSDictionary *scene=data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:&error] : nil;
        if (![scene isKindOfClass:NSDictionary.class] || ![scene[@"collision"] isKindOfClass:NSString.class]
            || ![scene[@"rows"] isKindOfClass:NSArray.class]
            || ![scene[@"atlas"] isKindOfClass:NSString.class] || ![scene[@"hero"] isKindOfClass:NSString.class]) {
            fprintf(stderr,"Invalid scene; run scripts/prepare_scene.py first\n"); return 1;
        }
        RoomView *view=nil;
        try { view=[[RoomView alloc] initWithScene:scene]; }
        catch (const std::exception& e) { fprintf(stderr,"%s\n",e.what()); return 1; }
        if (!view) { fprintf(stderr,"Cannot load scene images\n"); return 1; }
        NSApplication *app=[NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate *delegate=[AppDelegate new]; app.delegate=delegate;
        delegate.window=[[NSWindow alloc] initWithContentRect:view.frame
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered defer:NO];
        delegate.window.title=@"DredmorMac — Room preview";
        delegate.window.contentView=view;
        [delegate.window center]; [delegate.window makeKeyAndOrderFront:nil];
        [delegate.window makeFirstResponder:view];
        [app activateIgnoringOtherApps:YES]; [app run];
    }
    return 0;
}
