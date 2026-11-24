#import <Cocoa/Cocoa.h>
@protocol DredmorInventoryOwner <NSObject>
- (NSArray<NSDictionary*>*)inventoryRows;
- (BOOL)equipInventoryInstance:(uint64_t)instance slot:(NSInteger)slot;
- (BOOL)unequipInventorySlot:(NSInteger)slot;
- (BOOL)useInventoryConsumable:(uint64_t)instance;
- (BOOL)sellInventoryInstance:(uint64_t)instance;
- (BOOL)quickSellInventoryInstance:(uint64_t)instance;
@end

@interface DredmorInventoryController : NSWindowController<NSTableViewDataSource>
@property(weak) id<DredmorInventoryOwner> inventoryOwner;
@property(strong,readonly) NSTableView *tableView;
- (instancetype)initWithOwner:(id<DredmorInventoryOwner>)owner;
- (void)refresh;
- (void)equipSelection:(id)sender;
- (void)unequipSelection:(id)sender;
- (void)useSelection:(id)sender;
- (void)sellSelection:(id)sender;
- (void)quickSellSelection:(id)sender;
@end
