#import "DeviceConfigurationDialogs.h"
#import <QuartzCore/QuartzCore.h>
#include <vector>
#include <string>
#include <algorithm>

// MARK: - DeviceConfigurationDialog Implementation

@implementation DeviceConfigurationDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow {
    self = [super init];
    if (self) {
        self.isEditMode = NO;
        [self setupDialog:parentWindow];
        [self setupDefaultConfig];
    }
    return self;
}

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow deviceConfig:(OSCDeviceConfig)config {
    self = [super init];
    if (self) {
        self.isEditMode = YES;
        self.deviceConfig = config;
        [self setupDialog:parentWindow];
        [self populateFieldsFromConfig];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create dialog window
    NSRect frame = NSMakeRect(0, 0, 600, 700);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = self.isEditMode ? @"Edit OSC Device" : @"Add OSC Device";
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    NSView *contentView = self.dialogWindow.contentView;
    
    // Create and layout controls
    int yPos = 650;
    int spacing = 35;
    int leftMargin = 20;
    int rightMargin = 20;
    int fieldWidth = frame.size.width - leftMargin - rightMargin - 120;
    
    // Device Name
    NSTextField *deviceNameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    deviceNameLabel.stringValue = @"Device Name:";
    deviceNameLabel.editable = NO;
    deviceNameLabel.bezeled = NO;
    deviceNameLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:deviceNameLabel];
    
    self.deviceNameField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, fieldWidth, 22)];
    self.deviceNameField.placeholderString = @"Enter device name";
    [contentView addSubview:self.deviceNameField];
    yPos -= spacing;
    
    // Device ID
    NSTextField *deviceIdLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    deviceIdLabel.stringValue = @"Device ID:";
    deviceIdLabel.editable = NO;
    deviceIdLabel.bezeled = NO;
    deviceIdLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:deviceIdLabel];
    
    self.deviceIdField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, fieldWidth, 22)];
    self.deviceIdField.placeholderString = @"Unique device identifier";
    [contentView addSubview:self.deviceIdField];
    yPos -= spacing;
    
    // Protocol Type
    NSTextField *protocolLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    protocolLabel.stringValue = @"Protocol:";
    protocolLabel.editable = NO;
    protocolLabel.bezeled = NO;
    protocolLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:protocolLabel];
    
    self.protocolTypeButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 200, 25)];
    [self.protocolTypeButton addItemWithTitle:@"UDP Unicast"];
    [self.protocolTypeButton addItemWithTitle:@"UDP Multicast"];
    [self.protocolTypeButton addItemWithTitle:@"TCP"];
    [contentView addSubview:self.protocolTypeButton];
    yPos -= spacing;
    
    // Network Address
    NSTextField *addressLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    addressLabel.stringValue = @"Address:";
    addressLabel.editable = NO;
    addressLabel.bezeled = NO;
    addressLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:addressLabel];
    
    self.networkAddressField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, fieldWidth, 22)];
    self.networkAddressField.placeholderString = @"127.0.0.1";
    [contentView addSubview:self.networkAddressField];
    yPos -= spacing;
    
    // Port
    NSTextField *portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    portLabel.stringValue = @"Port:";
    portLabel.editable = NO;
    portLabel.bezeled = NO;
    portLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:portLabel];
    
    self.portField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 100, 22)];
    self.portField.placeholderString = @"9000";
    [contentView addSubview:self.portField];
    yPos -= spacing;
    
    // Local Address
    NSTextField *localAddressLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    localAddressLabel.stringValue = @"Local Address:";
    localAddressLabel.editable = NO;
    localAddressLabel.bezeled = NO;
    localAddressLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:localAddressLabel];
    
    self.localAddressField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, fieldWidth, 22)];
    self.localAddressField.placeholderString = @"0.0.0.0";
    [contentView addSubview:self.localAddressField];
    yPos -= spacing;
    
    // Local Port
    NSTextField *localPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    localPortLabel.stringValue = @"Local Port:";
    localPortLabel.editable = NO;
    localPortLabel.bezeled = NO;
    localPortLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:localPortLabel];
    
    self.localPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 100, 22)];
    self.localPortField.placeholderString = @"0 (auto)";
    [contentView addSubview:self.localPortField];
    yPos -= spacing;
    
    // OSC Address
    NSTextField *oscAddressLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    oscAddressLabel.stringValue = @"OSC Address:";
    oscAddressLabel.editable = NO;
    oscAddressLabel.bezeled = NO;
    oscAddressLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:oscAddressLabel];
    
    self.oscAddressField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, fieldWidth, 22)];
    self.oscAddressField.placeholderString = @"/channel/1";
    [contentView addSubview:self.oscAddressField];
    yPos -= spacing;
    
    // Signal Level
    NSTextField *signalLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    signalLabel.stringValue = @"Signal Level:";
    signalLabel.editable = NO;
    signalLabel.bezeled = NO;
    signalLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:signalLabel];
    
    self.signalLevelSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 200, 22)];
    self.signalLevelSlider.minValue = 0.0;
    self.signalLevelSlider.maxValue = 1.0;
    self.signalLevelSlider.doubleValue = 1.0;
    self.signalLevelSlider.target = self;
    self.signalLevelSlider.action = @selector(signalLevelChanged:);
    [contentView addSubview:self.signalLevelSlider];
    
    self.signalLevelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 330, yPos, 50, 20)];
    self.signalLevelLabel.stringValue = @"1.0";
    self.signalLevelLabel.editable = NO;
    self.signalLevelLabel.bezeled = NO;
    self.signalLevelLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:self.signalLevelLabel];
    yPos -= spacing;
    
    // Enabled checkbox
    self.enabledCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 100, 25)];
    [self.enabledCheckbox setButtonType:NSButtonTypeSwitch];
    self.enabledCheckbox.title = @"Enabled";
    self.enabledCheckbox.state = NSControlStateValueOn;
    [contentView addSubview:self.enabledCheckbox];
    yPos -= spacing;
    
    // Advanced Settings Section
    NSTextField *advancedLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 200, 20)];
    advancedLabel.stringValue = @"Advanced Settings";
    advancedLabel.editable = NO;
    advancedLabel.bezeled = NO;
    advancedLabel.backgroundColor = [NSColor clearColor];
    advancedLabel.font = [NSFont boldSystemFontOfSize:14];
    [contentView addSubview:advancedLabel];
    yPos -= spacing;
    
    // Timeout
    NSTextField *timeoutLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 100, 20)];
    timeoutLabel.stringValue = @"Timeout (ms):";
    timeoutLabel.editable = NO;
    timeoutLabel.bezeled = NO;
    timeoutLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:timeoutLabel];
    
    self.timeoutField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 100, 22)];
    self.timeoutField.placeholderString = @"5000";
    [contentView addSubview:self.timeoutField];
    yPos -= spacing;
    
    // Use Timestamps checkbox
    self.useTimestampsCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 120, yPos, 150, 25)];
    [self.useTimestampsCheckbox setButtonType:NSButtonTypeSwitch];
    self.useTimestampsCheckbox.title = @"Use Timestamps";
    [contentView addSubview:self.useTimestampsCheckbox];
    yPos -= spacing + 20;
    
    // Buttons
    self.cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 200, 20, 80, 30)];
    self.cancelButton.title = @"Cancel";
    self.cancelButton.target = self;
    self.cancelButton.action = @selector(cancelClicked:);
    [contentView addSubview:self.cancelButton];
    
    self.testConnectionButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 310, 20, 100, 30)];
    self.testConnectionButton.title = @"Test";
    self.testConnectionButton.target = self;
    self.testConnectionButton.action = @selector(testConnectionClicked:);
    [contentView addSubview:self.testConnectionButton];
    
    self.okButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 110, 20, 80, 30)];
    self.okButton.title = @"OK";
    self.okButton.target = self;
    self.okButton.action = @selector(okClicked:);
    self.okButton.keyEquivalent = @"\\r";
    [contentView addSubview:self.okButton];
}

- (void)setupDefaultConfig {
    // Initialize with default values
    OSCDeviceConfig config;
    config.deviceName = "New OSC Device";
    config.deviceId = "osc_device_" + std::to_string(arc4random() % 10000);
    config.protocolType = OSCProtocolType::UDP_UNICAST;
    config.networkAddress = "127.0.0.1";
    config.port = 9000;
    config.localAddress = "0.0.0.0";
    config.localPort = 0;
    config.oscAddress = "/channel/1";
    config.signalLevel = 1.0f;
    config.enabled = true;
    config.timeout = 5000;
    config.useTimestamps = false;
    
    self.deviceConfig = config;
}

- (void)populateFieldsFromConfig {
    self.deviceNameField.stringValue = [NSString stringWithUTF8String:self.deviceConfig.deviceName.c_str()];
    self.deviceIdField.stringValue = [NSString stringWithUTF8String:self.deviceConfig.deviceId.c_str()];
    [self.protocolTypeButton selectItemAtIndex:(NSInteger)self.deviceConfig.protocolType];
    self.networkAddressField.stringValue = [NSString stringWithUTF8String:self.deviceConfig.networkAddress.c_str()];
    self.portField.stringValue = [NSString stringWithFormat:@"%d", self.deviceConfig.port];
    self.localAddressField.stringValue = [NSString stringWithUTF8String:self.deviceConfig.localAddress.c_str()];
    self.localPortField.stringValue = [NSString stringWithFormat:@"%d", self.deviceConfig.localPort];
    self.oscAddressField.stringValue = [NSString stringWithUTF8String:self.deviceConfig.oscAddress.c_str()];
    self.signalLevelSlider.doubleValue = self.deviceConfig.signalLevel;
    self.signalLevelLabel.stringValue = [NSString stringWithFormat:@"%.2f", self.deviceConfig.signalLevel];
    self.enabledCheckbox.state = self.deviceConfig.enabled ? NSControlStateValueOn : NSControlStateValueOff;
    self.timeoutField.stringValue = [NSString stringWithFormat:@"%d", self.deviceConfig.timeout];
    self.useTimestampsCheckbox.state = self.deviceConfig.useTimestamps ? NSControlStateValueOn : NSControlStateValueOff;
}

- (OSCDeviceConfig)getDeviceConfig {
    OSCDeviceConfig config;
    config.deviceName = std::string([self.deviceNameField.stringValue UTF8String]);
    config.deviceId = std::string([self.deviceIdField.stringValue UTF8String]);
    config.protocolType = (OSCProtocolType)[self.protocolTypeButton indexOfSelectedItem];
    config.networkAddress = std::string([self.networkAddressField.stringValue UTF8String]);
    config.port = [self.portField.stringValue intValue];
    config.localAddress = std::string([self.localAddressField.stringValue UTF8String]);
    config.localPort = [self.localPortField.stringValue intValue];
    config.oscAddress = std::string([self.oscAddressField.stringValue UTF8String]);
    config.signalLevel = self.signalLevelSlider.floatValue;
    config.enabled = self.enabledCheckbox.state == NSControlStateValueOn;
    config.timeout = [self.timeoutField.stringValue intValue];
    config.useTimestamps = self.useTimestampsCheckbox.state == NSControlStateValueOn;
    return config;
}

- (void)showDialog:(void (^)(OSCDeviceConfig config, BOOL cancelled))completion {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
    
    if (completion) {
        completion([self getDeviceConfig], self.cancelled);
    }
}

- (void)signalLevelChanged:(NSSlider *)sender {
    self.signalLevelLabel.stringValue = [NSString stringWithFormat:@"%.2f", sender.floatValue];
}

- (void)okClicked:(NSButton *)sender {
    // Validate input
    if (self.deviceNameField.stringValue.length == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Input";
        alert.informativeText = @"Device name cannot be empty.";
        [alert runModal];
        return;
    }
    
    if (self.deviceIdField.stringValue.length == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Input";
        alert.informativeText = @"Device ID cannot be empty.";
        [alert runModal];
        return;
    }
    
    if (self.networkAddressField.stringValue.length == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Input";
        alert.informativeText = @"Network address cannot be empty.";
        [alert runModal];
        return;
    }
    
    int port = [self.portField.stringValue intValue];
    if (port <= 0 || port > 65535) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Input";
        alert.informativeText = @"Port must be between 1 and 65535.";
        [alert runModal];
        return;
    }
    
    self.cancelled = NO;
    [NSApp stopModal];
}

- (void)cancelClicked:(NSButton *)sender {
    self.cancelled = YES;
    [NSApp stopModal];
}

- (void)testConnectionClicked:(NSButton *)sender {
    // Get current configuration
    OSCDeviceConfig config = [self getDeviceConfig];
    
    // Validate configuration first
    if (config.networkAddress.empty() || config.port <= 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Configuration";
        alert.informativeText = @"Please enter valid network address and port before testing.";
        [alert runModal];
        return;
    }
    
    // Create test OSC sender
    NSString *address = [NSString stringWithUTF8String:config.networkAddress.c_str()];
    NSString *port = [NSString stringWithFormat:@"%d", config.port];
    
    // Test the connection by trying to send a test message
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        @try {
            // Simple connection test using liblo
            lo_address target = lo_address_new([address UTF8String], [port UTF8String]);
            
            if (target) {
                // Send a test ping message
                int result = lo_send(target, "/test/ping", "s", "cv_to_osc_test");
                lo_address_free(target);
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSAlert *alert = [[NSAlert alloc] init];
                    if (result == 0) {
                        alert.messageText = @"Connection Test Successful";
                        alert.informativeText = [NSString stringWithFormat:@"Successfully sent test message to %@:%@\n\nNote: This only tests message sending, not reception.", address, port];
                        alert.alertStyle = NSAlertStyleInformational;
                    } else {
                        alert.messageText = @"Connection Test Failed";
                        alert.informativeText = [NSString stringWithFormat:@"Failed to send test message to %@:%@\n\nPlease check the address and port.", address, port];
                        alert.alertStyle = NSAlertStyleWarning;
                    }
                    [alert runModal];
                });
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Connection Test Failed";
                    alert.informativeText = @"Could not create OSC target. Please check the address format.";
                    alert.alertStyle = NSAlertStyleCritical;
                    [alert runModal];
                });
            }
        } @catch (NSException *exception) {
            dispatch_async(dispatch_get_main_queue(), ^{
                NSAlert *alert = [[NSAlert alloc] init];
                alert.messageText = @"Connection Test Error";
                alert.informativeText = [NSString stringWithFormat:@"Error during test: %@", exception.reason];
                alert.alertStyle = NSAlertStyleCritical;
                [alert runModal];
            });
        }
    });
    
    // Show immediate feedback
    NSAlert *testingAlert = [[NSAlert alloc] init];
    testingAlert.messageText = @"Testing Connection";
    testingAlert.informativeText = @"Sending test message to device...";
    testingAlert.alertStyle = NSAlertStyleInformational;
    
    // Use a timer to auto-close this alert
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [[testingAlert window] close];
    });
    
    [testingAlert runModal];
}

@end

// MARK: - ChannelConfigurationDialog Implementation

@implementation ChannelConfigurationDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                           channelId:(int)channelId 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        self.channelId = channelId;
        self.mixerEngine = engine;
        self.inputDevices = [[NSMutableArray alloc] init];
        self.outputDevices = [[NSMutableArray alloc] init];
        [self setupDialog:parentWindow];
        [self loadChannelData];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create dialog window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = [NSString stringWithFormat:@"Channel %d Configuration", self.channelId + 1];
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    NSView *contentView = self.dialogWindow.contentView;
    
    // Create tab view for better organization
    NSTabView *tabView = [[NSTabView alloc] initWithFrame:NSMakeRect(10, 50, frame.size.width - 20, frame.size.height - 100)];
    [contentView addSubview:tabView];
    
    // General Tab
    NSTabViewItem *generalTab = [[NSTabViewItem alloc] init];
    generalTab.label = @"General";
    NSView *generalView = [[NSView alloc] initWithFrame:tabView.contentRect];
    generalTab.view = generalView;
    [tabView addTabViewItem:generalTab];
    
    [self setupGeneralTab:generalView];
    
    // Input Devices Tab
    NSTabViewItem *inputTab = [[NSTabViewItem alloc] init];
    inputTab.label = @"Input Devices";
    NSView *inputView = [[NSView alloc] initWithFrame:tabView.contentRect];
    inputTab.view = inputView;
    [tabView addTabViewItem:inputTab];
    
    [self setupInputDevicesTab:inputView];
    
    // Output Devices Tab
    NSTabViewItem *outputTab = [[NSTabViewItem alloc] init];
    outputTab.label = @"Output Devices";
    NSView *outputView = [[NSView alloc] initWithFrame:tabView.contentRect];
    outputTab.view = outputView;
    [tabView addTabViewItem:outputTab];
    
    [self setupOutputDevicesTab:outputView];
    
    // Buttons
    self.cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 200, 10, 80, 30)];
    self.cancelButton.title = @"Cancel";
    self.cancelButton.target = self;
    self.cancelButton.action = @selector(cancelClicked:);
    [contentView addSubview:self.cancelButton];
    
    self.okButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 110, 10, 80, 30)];
    self.okButton.title = @"OK";
    self.okButton.target = self;
    self.okButton.action = @selector(okClicked:);
    self.okButton.keyEquivalent = @"\\r";
    [contentView addSubview:self.okButton];
}

- (void)setupGeneralTab:(NSView *)view {
    int yPos = view.frame.size.height - 50;
    int spacing = 35;
    int leftMargin = 20;
    
    // Channel Name
    NSTextField *nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    nameLabel.stringValue = @"Channel Name:";
    nameLabel.editable = NO;
    nameLabel.bezeled = NO;
    nameLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:nameLabel];
    
    self.channelNameField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 300, 22)];
    [view addSubview:self.channelNameField];
    yPos -= spacing;
    
    // Level Volts
    NSTextField *levelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    levelLabel.stringValue = @"Level (Volts):";
    levelLabel.editable = NO;
    levelLabel.bezeled = NO;
    levelLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:levelLabel];
    
    self.levelVoltsSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 200, 22)];
    self.levelVoltsSlider.minValue = -10.0;
    self.levelVoltsSlider.maxValue = 10.0;
    self.levelVoltsSlider.target = self;
    self.levelVoltsSlider.action = @selector(levelChanged:);
    [view addSubview:self.levelVoltsSlider];
    
    self.levelVoltsLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 340, yPos, 50, 20)];
    self.levelVoltsLabel.editable = NO;
    self.levelVoltsLabel.bezeled = NO;
    self.levelVoltsLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:self.levelVoltsLabel];
    yPos -= spacing;
    
    // Min Range
    NSTextField *minLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    minLabel.stringValue = @"Min Range:";
    minLabel.editable = NO;
    minLabel.bezeled = NO;
    minLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:minLabel];
    
    self.minRangeField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 100, 22)];
    [view addSubview:self.minRangeField];
    yPos -= spacing;
    
    // Max Range
    NSTextField *maxLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    maxLabel.stringValue = @"Max Range:";
    maxLabel.editable = NO;
    maxLabel.bezeled = NO;
    maxLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:maxLabel];
    
    self.maxRangeField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 100, 22)];
    [view addSubview:self.maxRangeField];
    yPos -= spacing;
    
    // Channel Color
    NSTextField *colorLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    colorLabel.stringValue = @"Channel Color:";
    colorLabel.editable = NO;
    colorLabel.bezeled = NO;
    colorLabel.backgroundColor = [NSColor clearColor];
    [view addSubview:colorLabel];
    
    self.channelColorWell = [[NSColorWell alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 50, 25)];
    [view addSubview:self.channelColorWell];
    yPos -= spacing;
    
    // Show in Master
    self.showInMasterCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 200, 25)];
    [self.showInMasterCheckbox setButtonType:NSButtonTypeSwitch];
    self.showInMasterCheckbox.title = @"Show in Master Mix";
    [view addSubview:self.showInMasterCheckbox];
}

- (void)setupInputDevicesTab:(NSView *)view {
    // Input devices table
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 50, view.frame.size.width - 20, view.frame.size.height - 100)];
    self.inputDevicesTable = [[NSTableView alloc] init];
    
    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"Device Name";
    nameColumn.width = 200;
    [self.inputDevicesTable addTableColumn:nameColumn];
    
    NSTableColumn *addressColumn = [[NSTableColumn alloc] initWithIdentifier:@"address"];
    addressColumn.title = @"Address";
    addressColumn.width = 150;
    [self.inputDevicesTable addTableColumn:addressColumn];
    
    NSTableColumn *portColumn = [[NSTableColumn alloc] initWithIdentifier:@"port"];
    portColumn.title = @"Port";
    portColumn.width = 80;
    [self.inputDevicesTable addTableColumn:portColumn];
    
    NSTableColumn *statusColumn = [[NSTableColumn alloc] initWithIdentifier:@"status"];
    statusColumn.title = @"Status";
    statusColumn.width = 100;
    [self.inputDevicesTable addTableColumn:statusColumn];
    
    scrollView.documentView = self.inputDevicesTable;
    [view addSubview:scrollView];
    
    // Buttons
    self.addInputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, 10, 80, 30)];
    self.addInputDeviceButton.title = @"Add";
    self.addInputDeviceButton.target = self;
    self.addInputDeviceButton.action = @selector(addInputDevice:);
    [view addSubview:self.addInputDeviceButton];
    
    self.editInputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(100, 10, 80, 30)];
    self.editInputDeviceButton.title = @"Edit";
    self.editInputDeviceButton.target = self;
    self.editInputDeviceButton.action = @selector(editInputDevice:);
    [view addSubview:self.editInputDeviceButton];
    
    self.removeInputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(190, 10, 80, 30)];
    self.removeInputDeviceButton.title = @"Remove";
    self.removeInputDeviceButton.target = self;
    self.removeInputDeviceButton.action = @selector(removeInputDevice:);
    [view addSubview:self.removeInputDeviceButton];
}

- (void)setupOutputDevicesTab:(NSView *)view {
    // Similar to input devices tab
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 50, view.frame.size.width - 20, view.frame.size.height - 100)];
    self.outputDevicesTable = [[NSTableView alloc] init];
    
    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"Device Name";
    nameColumn.width = 200;
    [self.outputDevicesTable addTableColumn:nameColumn];
    
    NSTableColumn *addressColumn = [[NSTableColumn alloc] initWithIdentifier:@"address"];
    addressColumn.title = @"Address";
    addressColumn.width = 150;
    [self.outputDevicesTable addTableColumn:addressColumn];
    
    NSTableColumn *portColumn = [[NSTableColumn alloc] initWithIdentifier:@"port"];
    portColumn.title = @"Port";
    portColumn.width = 80;
    [self.outputDevicesTable addTableColumn:portColumn];
    
    NSTableColumn *statusColumn = [[NSTableColumn alloc] initWithIdentifier:@"status"];
    statusColumn.title = @"Status";
    statusColumn.width = 100;
    [self.outputDevicesTable addTableColumn:statusColumn];
    
    scrollView.documentView = self.outputDevicesTable;
    [view addSubview:scrollView];
    
    // Buttons
    self.addOutputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, 10, 80, 30)];
    self.addOutputDeviceButton.title = @"Add";
    self.addOutputDeviceButton.target = self;
    self.addOutputDeviceButton.action = @selector(addOutputDevice:);
    [view addSubview:self.addOutputDeviceButton];
    
    self.editOutputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(100, 10, 80, 30)];
    self.editOutputDeviceButton.title = @"Edit";
    self.editOutputDeviceButton.target = self;
    self.editOutputDeviceButton.action = @selector(editOutputDevice:);
    [view addSubview:self.editOutputDeviceButton];
    
    self.removeOutputDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(190, 10, 80, 30)];
    self.removeOutputDeviceButton.title = @"Remove";
    self.removeOutputDeviceButton.target = self;
    self.removeOutputDeviceButton.action = @selector(removeOutputDevice:);
    [view addSubview:self.removeOutputDeviceButton];
}

- (void)loadChannelData {
    if (self.mixerEngine) {
        // TODO: Load actual channel data from mixer engine
        self.channelNameField.stringValue = [NSString stringWithFormat:@"Channel %d", self.channelId + 1];
        self.levelVoltsSlider.doubleValue = 0.0;
        self.levelVoltsLabel.stringValue = @"0.0V";
        self.minRangeField.stringValue = @"-10.0";
        self.maxRangeField.stringValue = @"10.0";
        self.channelColorWell.color = [NSColor greenColor];
        self.showInMasterCheckbox.state = NSControlStateValueOn;
    }
}

- (void)showDialog:(void (^)(BOOL cancelled))completion {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
    
    if (completion) {
        completion(self.cancelled);
    }
}

- (void)levelChanged:(NSSlider *)sender {
    self.levelVoltsLabel.stringValue = [NSString stringWithFormat:@"%.1fV", sender.floatValue];
}

- (void)addInputDevice:(NSButton *)sender {
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.dialogWindow];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            // Add device to input devices array
            NSDictionary *deviceDict = convertFromDeviceConfigToObjC(config);
            [self.inputDevices addObject:deviceDict];
            [self.inputDevicesTable reloadData];
        }
    }];
}

- (void)editInputDevice:(NSButton *)sender {
    // TODO: Implement edit input device
}

- (void)removeInputDevice:(NSButton *)sender {
    NSInteger selectedRow = [self.inputDevicesTable selectedRow];
    if (selectedRow >= 0) {
        [self.inputDevices removeObjectAtIndex:selectedRow];
        [self.inputDevicesTable reloadData];
    }
}

- (void)addOutputDevice:(NSButton *)sender {
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.dialogWindow];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            // Add device to output devices array
            NSDictionary *deviceDict = convertFromDeviceConfigToObjC(config);
            [self.outputDevices addObject:deviceDict];
            [self.outputDevicesTable reloadData];
        }
    }];
}

- (void)editOutputDevice:(NSButton *)sender {
    // TODO: Implement edit output device
}

- (void)removeOutputDevice:(NSButton *)sender {
    NSInteger selectedRow = [self.outputDevicesTable selectedRow];
    if (selectedRow >= 0) {
        [self.outputDevices removeObjectAtIndex:selectedRow];
        [self.outputDevicesTable reloadData];
    }
}

- (void)okClicked:(NSButton *)sender {
    // TODO: Apply channel configuration to mixer engine
    self.cancelled = NO;
    [NSApp stopModal];
}

- (void)cancelClicked:(NSButton *)sender {
    self.cancelled = YES;
    [NSApp stopModal];
}

@end

// MARK: - GlobalSettingsDialog Implementation

@implementation GlobalSettingsDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        self.mixerEngine = engine;
        [self setupDialog:parentWindow];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create dialog window
    NSRect frame = NSMakeRect(0, 0, 500, 400);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = @"Global Mixer Settings";
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    NSView *contentView = self.dialogWindow.contentView;
    
    // Create a basic settings interface
    int yPos = 350;
    int spacing = 35;
    int leftMargin = 20;
    
    // Master Level
    NSTextField *masterLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin, yPos, 120, 20)];
    masterLabel.stringValue = @"Master Level:";
    masterLabel.editable = NO;
    masterLabel.bezeled = NO;
    masterLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:masterLabel];
    
    self.masterLevelSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 200, 22)];
    self.masterLevelSlider.minValue = 0.0;
    self.masterLevelSlider.maxValue = 1.0;
    self.masterLevelSlider.doubleValue = 0.8;
    [contentView addSubview:self.masterLevelSlider];
    
    self.masterLevelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(leftMargin + 340, yPos, 50, 20)];
    self.masterLevelLabel.stringValue = @"0.8";
    self.masterLevelLabel.editable = NO;
    self.masterLevelLabel.bezeled = NO;
    self.masterLevelLabel.backgroundColor = [NSColor clearColor];
    [contentView addSubview:self.masterLevelLabel];
    yPos -= spacing;
    
    // Master Mute
    self.masterMuteCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 150, 25)];
    [self.masterMuteCheckbox setButtonType:NSButtonTypeSwitch];
    self.masterMuteCheckbox.title = @"Master Mute";
    [contentView addSubview:self.masterMuteCheckbox];
    yPos -= spacing;
    
    // Learning Mode
    self.enableLearningModeCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(leftMargin + 130, yPos, 200, 25)];
    [self.enableLearningModeCheckbox setButtonType:NSButtonTypeSwitch];
    self.enableLearningModeCheckbox.title = @"Enable Learning Mode";
    [contentView addSubview:self.enableLearningModeCheckbox];
    yPos -= spacing + 20;
    
    // Buttons
    self.cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 200, 20, 80, 30)];
    self.cancelButton.title = @"Cancel";
    self.cancelButton.target = self;
    self.cancelButton.action = @selector(cancelClicked:);
    [contentView addSubview:self.cancelButton];
    
    self.applyButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 310, 20, 80, 30)];
    self.applyButton.title = @"Apply";
    self.applyButton.target = self;
    self.applyButton.action = @selector(applyClicked:);
    [contentView addSubview:self.applyButton];
    
    self.okButton = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 110, 20, 80, 30)];
    self.okButton.title = @"OK";
    self.okButton.target = self;
    self.okButton.action = @selector(okClicked:);
    self.okButton.keyEquivalent = @"\r";
    [contentView addSubview:self.okButton];
}

- (void)showDialog:(void (^)(BOOL cancelled))completion {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
    
    if (completion) {
        completion(self.cancelled);
    }
}

- (void)applyClicked:(NSButton *)sender {
    // Apply settings without closing dialog
    [self applySettings];
}

- (void)okClicked:(NSButton *)sender {
    [self applySettings];
    self.cancelled = NO;
    [NSApp stopModal];
}

- (void)cancelClicked:(NSButton *)sender {
    self.cancelled = YES;
    [NSApp stopModal];
}

- (void)applySettings {
    if (self.mixerEngine) {
        self.mixerEngine->setMasterVolume([self.masterLevelSlider floatValue]);
        self.mixerEngine->enableLearningMode([self.enableLearningModeCheckbox state] == NSControlStateValueOn);
        // TODO: Apply other settings
    }
}

@end

// MARK: - DeviceDiscoveryDialog Implementation

@implementation DeviceDiscoveryDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        self.mixerEngine = engine;
        self.discoveredDevices = [[NSMutableArray alloc] init];
        [self setupDialog:parentWindow];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create basic discovery dialog
    NSRect frame = NSMakeRect(0, 0, 600, 400);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = @"Device Discovery";
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    // TODO: Implement device discovery interface
}

- (void)showDialog {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
}

- (void)startScanning {
    // TODO: Implement scanning
}

- (void)stopScanning {
    // TODO: Implement stop scanning
}

@end

// MARK: - ConnectionMonitorDialog Implementation

@implementation ConnectionMonitorDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        self.mixerEngine = engine;
        self.connectionData = [[NSMutableArray alloc] init];
        [self setupDialog:parentWindow];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create basic monitor dialog
    NSRect frame = NSMakeRect(0, 0, 700, 500);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = @"Connection Monitor";
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    // TODO: Implement connection monitor interface
}

- (void)showDialog {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
}

- (void)updateConnectionData {
    // TODO: Implement connection data update
}

@end

// MARK: - ConfigurationPresetDialog Implementation

@implementation ConfigurationPresetDialog

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        self.mixerEngine = engine;
        self.presets = [[NSMutableArray alloc] init];
        [self setupDialog:parentWindow];
    }
    return self;
}

- (void)setupDialog:(NSWindow *)parentWindow {
    // Create basic preset dialog
    NSRect frame = NSMakeRect(0, 0, 600, 400);
    self.dialogWindow = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    
    self.dialogWindow.title = @"Configuration Presets";
    self.dialogWindow.releasedWhenClosed = NO;
    [self.dialogWindow center];
    
    // TODO: Implement preset interface
}

- (void)showDialog {
    [NSApp runModalForWindow:self.dialogWindow];
    [self.dialogWindow orderOut:nil];
}

@end

// MARK: - Utility Functions

OSCDeviceConfig convertFromObjCToDeviceConfig(NSDictionary *dict) {
    OSCDeviceConfig config;
    config.deviceName = std::string([[dict objectForKey:@"deviceName"] UTF8String]);
    config.deviceId = std::string([[dict objectForKey:@"deviceId"] UTF8String]);
    config.protocolType = (OSCProtocolType)[[dict objectForKey:@"protocolType"] intValue];
    config.networkAddress = std::string([[dict objectForKey:@"networkAddress"] UTF8String]);
    config.port = [[dict objectForKey:@"port"] intValue];
    config.localAddress = std::string([[dict objectForKey:@"localAddress"] UTF8String]);
    config.localPort = [[dict objectForKey:@"localPort"] intValue];
    config.oscAddress = std::string([[dict objectForKey:@"oscAddress"] UTF8String]);
    config.signalLevel = [[dict objectForKey:@"signalLevel"] floatValue];
    config.enabled = [[dict objectForKey:@"enabled"] boolValue];
    config.timeout = [[dict objectForKey:@"timeout"] intValue];
    config.useTimestamps = [[dict objectForKey:@"useTimestamps"] boolValue];
    return config;
}

NSDictionary* convertFromDeviceConfigToObjC(const OSCDeviceConfig& config) {
    return @{
        @"deviceName": [NSString stringWithUTF8String:config.deviceName.c_str()],
        @"deviceId": [NSString stringWithUTF8String:config.deviceId.c_str()],
        @"protocolType": @((int)config.protocolType),
        @"networkAddress": [NSString stringWithUTF8String:config.networkAddress.c_str()],
        @"port": @(config.port),
        @"localAddress": [NSString stringWithUTF8String:config.localAddress.c_str()],
        @"localPort": @(config.localPort),
        @"oscAddress": [NSString stringWithUTF8String:config.oscAddress.c_str()],
        @"signalLevel": @(config.signalLevel),
        @"enabled": @(config.enabled),
        @"timeout": @(config.timeout),
        @"useTimestamps": @(config.useTimestamps)
    };
}
