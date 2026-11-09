#import "SkillSelectionController.h"
@interface SkillListView : NSView
@end
@implementation SkillListView
- (BOOL)isFlipped { return YES; }
@end

@implementation DredmorSkillSelectionController {
    std::vector<dredmor::SkillDefinition> _skills;
    NSMutableArray<NSButton*> *_choices;
    NSButton *_begin;
    NSTextField *_summary;
    BOOL _modal;
}
- (instancetype)initWithSkills:(const std::vector<dredmor::SkillDefinition>&)skills {
    NSWindow *window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,680,600)
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    window.title=@"Choose Seven Skills";window.releasedWhenClosed=NO;window.contentMinSize=NSMakeSize(680,400);
    self=[super initWithWindow:window];
    if (self) {
        for (const auto& skill:skills) if (!skill.deprecated) _skills.push_back(skill);
        _choices=[NSMutableArray array];window.delegate=self;
        NSView *content=window.contentView;
        NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(16,90,648,494)];
        scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;scroll.hasVerticalScroller=YES;
        SkillListView *list=[[SkillListView alloc] initWithFrame:NSMakeRect(0,0,628,MAX(494,((_skills.size()+2)/3)*38))];
        for (std::size_t i=0;i<_skills.size();++i) {
            NSButton *choice=[NSButton checkboxWithTitle:[NSString stringWithUTF8String:_skills[i].name.c_str()]
                target:self action:@selector(updateSelection:)];
            choice.frame=NSMakeRect((i%3)*208+4,(i/3)*38+6,200,28);choice.tag=_skills[i].id;
            choice.toolTip=[NSString stringWithUTF8String:_skills[i].source.attribute("description").c_str()];
            [list addSubview:choice];[_choices addObject:choice];
        }
        scroll.documentView=list;[content addSubview:scroll];
        _summary=[NSTextField labelWithString:@""];
        _summary.frame=NSMakeRect(16,60,648,20);[content addSubview:_summary];
        _begin=[NSButton buttonWithTitle:@"Begin" target:self action:@selector(beginSelection:)];
        _begin.frame=NSMakeRect(550,18,112,32);_begin.keyEquivalent=@"\r";[content addSubview:_begin];
        NSButton *cancel=[NSButton buttonWithTitle:@"Cancel" target:self action:@selector(cancelSelection:)];
        cancel.frame=NSMakeRect(430,18,112,32);cancel.keyEquivalent=@"\033";[content addSubview:cancel];
        [self updateSelection:nil];[window center];
    }
    return self;
}
- (NSArray<NSNumber*>*)selectedSkills {
    NSMutableArray<NSNumber*> *selected=[NSMutableArray array];
    for (NSButton *choice in _choices) if (choice.state==NSControlStateValueOn) [selected addObject:@(choice.tag)];
    return selected;
}
- (void)updateSelection:(id)sender {
    NSUInteger count=0;int classes[3]={0,0,0};
    for (NSUInteger i=0;i<_choices.count;++i) if (_choices[i].state==NSControlStateValueOn) {
        ++count;if (_skills[i].category>=0 && _skills[i].category<3) ++classes[_skills[i].category];
    }
    _begin.enabled=count==7;
    _summary.stringValue=[NSString stringWithFormat:@"%lu of 7 selected · Warrior %d · Wizard %d · Rogue %d",
        (unsigned long)count,classes[0],classes[1],classes[2]];
}
- (NSArray<NSNumber*>*)runSelection {
    _modal=YES;[NSApp activateIgnoringOtherApps:YES];[self showWindow:nil];
    const auto response=[NSApp runModalForWindow:self.window];_modal=NO;[self.window orderOut:nil];
    return response==NSModalResponseOK?[self selectedSkills]:nil;
}
- (void)beginSelection:(id)sender { if (_modal && [self selectedSkills].count==7) [NSApp stopModalWithCode:NSModalResponseOK]; }
- (void)cancelSelection:(id)sender { if (_modal) [NSApp stopModalWithCode:NSModalResponseCancel]; }
- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (_modal) {[self cancelSelection:nil];return NO;}return YES;
}
- (BOOL)smokeTest {
    if (_choices.count<7 || _begin.enabled) return NO;
    [self showWindow:nil];[self.window displayIfNeeded];
    for (NSUInteger i=0;i<7;++i) [_choices[i] performClick:nil];
    BOOL valid=_begin.enabled && [self selectedSkills].count==7;
    for (NSUInteger i=0;i<7;++i) valid=valid && [[self selectedSkills][i] intValue]==_skills[i].id;
    [_choices[0] performClick:nil];valid=valid && !_begin.enabled;
    [_choices[0] performClick:nil];valid=valid && _begin.enabled;[self close];return valid;
}
@end
