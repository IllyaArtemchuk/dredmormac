#import <Cocoa/Cocoa.h>
#include "dredmor/content.hpp"
@interface DredmorSkillSelectionController : NSWindowController<NSWindowDelegate>
- (instancetype)initWithSkills:(const std::vector<dredmor::SkillDefinition>&)skills;
- (NSArray<NSNumber*>*)runSelection;
- (NSArray<NSNumber*>*)selectedSkills;
- (BOOL)smokeTest;
@end
