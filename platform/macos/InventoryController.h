#import <Cocoa/Cocoa.h>
@protocol DredmorInventoryOwner <NSObject>
- (NSArray<NSDictionary*>*)inventoryRows;
- (BOOL)equipInventoryInstance:(uint64_t)instance slot:(NSInteger)slot;
- (BOOL)unequipInventorySlot:(NSInteger)slot;
@end

@interface DredmorInventoryController : NSWindowController<NSTableViewDataSource>
@property(weak) id<DredmorInventoryOwner> owner;
@property(strong,readonly) NSTableView *tableView;
- (instancetype)initWithOwner:(id<DredmorInventoryOwner>)owner;
- (void)refresh;
- (void)equipSelection:(id)sender;
- (void)unequipSelection:(id)sender;
@end
