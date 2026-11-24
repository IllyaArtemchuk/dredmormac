#import "InventoryController.h"
@implementation DredmorInventoryController {
    NSTableView *_tableView;
    NSPopUpButton *_slot;
    NSArray<NSDictionary*> *_rows;
}
@synthesize tableView=_tableView;
- (instancetype)initWithOwner:(id<DredmorInventoryOwner>)owner {
    NSWindow *window=[[NSPanel alloc] initWithContentRect:NSMakeRect(0,0,540,420)
        styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    window.title=@"Inventory and Equipment";window.releasedWhenClosed=NO;window.contentMinSize=NSMakeSize(540,300);
    self=[super initWithWindow:window];
    if (self) {
        _inventoryOwner=owner;
        NSView *content=window.contentView;
        NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(16,72,508,332)];
        scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;scroll.hasVerticalScroller=YES;
        _tableView=[[NSTableView alloc] initWithFrame:scroll.bounds];_tableView.dataSource=self;
        for (NSString *identifier in @[@"name",@"quantity",@"location"]) {
            NSTableColumn *column=[[NSTableColumn alloc] initWithIdentifier:identifier];
            column.title=[identifier capitalizedString];
            column.editable=NO;
            column.width=[identifier isEqualToString:@"name"]?260:([identifier isEqualToString:@"quantity"]?70:150);
            [_tableView addTableColumn:column];
        }
        scroll.documentView=_tableView;[content addSubview:scroll];
        _slot=[[NSPopUpButton alloc] initWithFrame:NSMakeRect(16,28,184,28) pullsDown:NO];
        [_slot addItemsWithTitles:@[@"Head",@"Left hand",@"Right hand",@"Left ring",@"Right ring",@"Chest",
            @"Legs",@"Feet",@"Ranged",@"Neck",@"Hands",@"Waist"]];[content addSubview:_slot];
        NSButton *equip=[NSButton buttonWithTitle:@"Equip" target:self action:@selector(equipSelection:)];
        equip.frame=NSMakeRect(210,28,80,28);[content addSubview:equip];
        NSButton *unequip=[NSButton buttonWithTitle:@"Unequip" target:self action:@selector(unequipSelection:)];
        unequip.frame=NSMakeRect(300,28,85,28);[content addSubview:unequip];
        NSButton *drink=[NSButton buttonWithTitle:@"Use" target:self action:@selector(useSelection:)];
        drink.frame=NSMakeRect(395,28,60,28);drink.autoresizingMask=NSViewMinXMargin;[content addSubview:drink];
        NSButton *sell=[NSButton buttonWithTitle:@"Sell" target:self action:@selector(sellSelection:)];
        sell.toolTip=@"Sell to a nearby keeper. Hold Shift to sell immediately while inside a calm shop.";sell.frame=NSMakeRect(465,28,60,28);sell.autoresizingMask=NSViewMinXMargin;[content addSubview:sell];
        [self refresh];[window center];
    }
    return self;
}
- (void)refresh { _rows=[self.inventoryOwner inventoryRows];[_tableView reloadData]; }
- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView { return _rows.count; }
- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    return row>=0 && row<(NSInteger)_rows.count?_rows[row][column.identifier]:nil;
}
- (void)equipSelection:(id)sender {
    const NSInteger row=_tableView.selectedRow;
    if (row<0 || row>=(NSInteger)_rows.count || [_rows[row][@"equipped"] boolValue] ||
        ![self.inventoryOwner equipInventoryInstance:[_rows[row][@"instance"] unsignedLongLongValue] slot:_slot.indexOfSelectedItem]) NSBeep();
    [self refresh];
}
- (void)unequipSelection:(id)sender {
    const NSInteger row=_tableView.selectedRow;
    if (row<0 || row>=(NSInteger)_rows.count || ![_rows[row][@"equipped"] boolValue] ||
        ![self.inventoryOwner unequipInventorySlot:[_rows[row][@"slot"] integerValue]]) NSBeep();
    [self refresh];
}
- (void)quickSellSelection:(id)sender {
    const NSInteger row=_tableView.selectedRow;
    if (row<0 || row>=(NSInteger)_rows.count || [_rows[row][@"equipped"] boolValue] ||
        ![self.inventoryOwner quickSellInventoryInstance:[_rows[row][@"instance"] unsignedLongLongValue]]) NSBeep();
    [self refresh];
}
- (void)sellSelection:(id)sender {
    if (NSApp.currentEvent.modifierFlags&NSEventModifierFlagShift) {[self quickSellSelection:sender];return;}
    const NSInteger row=_tableView.selectedRow;
    if (row<0 || row>=(NSInteger)_rows.count || [_rows[row][@"equipped"] boolValue] ||
        ![self.inventoryOwner sellInventoryInstance:[_rows[row][@"instance"] unsignedLongLongValue]]) NSBeep();
    [self refresh];
}
- (void)useSelection:(id)sender {
    const NSInteger row=_tableView.selectedRow;
    if (row<0 || row>=(NSInteger)_rows.count || [_rows[row][@"equipped"] boolValue] ||
        ![self.inventoryOwner useInventoryConsumable:[_rows[row][@"instance"] unsignedLongLongValue]]) NSBeep();
    [self refresh];
}
@end
