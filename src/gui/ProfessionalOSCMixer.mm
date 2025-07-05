#include "ProfessionalOSCMixer.h"

// Implementation of OSCDeviceConfigDialog
@implementation OSCDeviceConfigDialog {
    OSCDeviceConfig currentDevice;
    void(^completionHandler)(OSCDeviceConfig, BOOL);
}

- (instancetype)initWithDevice:(OSCDeviceConfig*)device completion:(void(^)(OSCDeviceConfig, BOOL))completion {
    self = [super init];
    if (self) {
        if (device) {
            currentDevice = *device;
        }
        completionHandler = completion;
        [self setupConfigWindow];
    }
    return self;
}

- (void)setupConfigWindow {
    // Create window
    NSRect windowFrame = NSMakeRect(0, 0, 600, 700);
    self.configWindow = [[NSWindow alloc] initWithContentRect:windowFrame
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [self.configWindow setTitle:@"OSC Device Configuration"];
    [self.configWindow center];
    
    // Create content view with scroll view
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:self.configWindow.contentView.bounds];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutohidesScrollers:YES];
    
    NSView *contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 580, 1000)];
    [scrollView setDocumentView:contentView];
    [self.configWindow setContentView:scrollView];
    
    CGFloat y = 950;
    
    // Title
    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 540, 30)];
    [titleLabel setStringValue:@"OSC Device Configuration"];
    [titleLabel setFont:[NSFont boldSystemFontOfSize:18]];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [contentView addSubview:titleLabel];
    y -= 50;
    
    // Basic Information Section
    [self addSectionHeader:@"Basic Information" atY:&y inView:contentView];
    
    [self addLabelAndField:@"Device ID:" field:&_deviceIdField atY:&y inView:contentView];
    [_deviceIdField setStringValue:[NSString stringWithUTF8String:currentDevice.deviceId.c_str()]];
    
    [self addLabelAndField:@"Device Name:" field:&_deviceNameField atY:&y inView:contentView];
    [_deviceNameField setStringValue:[NSString stringWithUTF8String:currentDevice.deviceName.c_str()]];
    
    [self addLabelAndField:@"Description:" field:&_descriptionField atY:&y inView:contentView];
    [_descriptionField setStringValue:[NSString stringWithUTF8String:currentDevice.description.c_str()]];
    
    // Protocol Section
    [self addSectionHeader:@"Protocol Configuration" atY:&y inView:contentView];
    
    [self addLabelAndPopup:@"Protocol Type:" popup:&_protocolTypePopup 
                   options:@[@"UDP", @"TCP", @"Multicast", @"Broadcast"] 
                      atY:&y inView:contentView];
    [_protocolTypePopup selectItemAtIndex:(int)currentDevice.protocolType];
    
    [self addLabelAndField:@"Network Address:" field:&_networkAddressField atY:&y inView:contentView];
    [_networkAddressField setStringValue:[NSString stringWithUTF8String:currentDevice.networkAddress.c_str()]];
    
    [self addLabelAndField:@"Port:" field:&_portField atY:&y inView:contentView];
    [_portField setStringValue:[NSString stringWithFormat:@"%d", currentDevice.port]];
    
    [self addLabelAndField:@"Local Address:" field:&_localAddressField atY:&y inView:contentView];
    [_localAddressField setStringValue:[NSString stringWithUTF8String:currentDevice.localAddress.c_str()]];
    
    [self addLabelAndField:@"Local Port:" field:&_localPortField atY:&y inView:contentView];
    [_localPortField setStringValue:[NSString stringWithFormat:@"%d", currentDevice.localPort]];
    
    // OSC Configuration Section
    [self addSectionHeader:@"OSC Configuration" atY:&y inView:contentView];
    
    [self addLabelAndField:@"OSC Address:" field:&_oscAddressField atY:&y inView:contentView];
    [_oscAddressField setStringValue:[NSString stringWithUTF8String:currentDevice.oscAddress.c_str()]];
    
    [self addLabelAndField:@"OSC Message:" field:&_oscMessageField atY:&y inView:contentView];
    [_oscMessageField setStringValue:[NSString stringWithUTF8String:currentDevice.oscMessage.c_str()]];
    
    [self addLabelAndPopup:@"Message Type:" popup:&_messageTypePopup 
                   options:@[@"Float", @"Double", @"Int32", @"Int64", @"String", @"Symbol", @"Blob"] 
                      atY:&y inView:contentView];
    
    // Signal Processing Section
    [self addSectionHeader:@"Signal Processing" atY:&y inView:contentView];
    
    [self addLabelAndSlider:@"Signal Level:" slider:&_signalLevelSlider 
                        min:0.0 max:4.0 value:currentDevice.signalLevel atY:&y inView:contentView];
    
    [self addLabelAndSlider:@"Signal Offset:" slider:&_signalOffsetSlider 
                        min:-1.0 max:1.0 value:currentDevice.signalOffset atY:&y inView:contentView];
    
    [self addCheckbox:@"Invert Signal" button:&_invertSignalButton 
              checked:currentDevice.invertSignal atY:&y inView:contentView];
    
    // Advanced Options Section
    [self addSectionHeader:@"Advanced Options" atY:&y inView:contentView];
    
    [self addLabelAndField:@"Timeout (ms):" field:&_timeoutField atY:&y inView:contentView];
    [_timeoutField setStringValue:[NSString stringWithFormat:@"%d", currentDevice.timeoutMs]];
    
    [self addLabelAndField:@"Max Retries:" field:&_maxRetriesField atY:&y inView:contentView];
    [_maxRetriesField setStringValue:[NSString stringWithFormat:@"%d", currentDevice.maxRetries]];
    
    [self addLabelAndField:@"Buffer Size:" field:&_bufferSizeField atY:&y inView:contentView];
    [_bufferSizeField setStringValue:[NSString stringWithFormat:@"%d", currentDevice.bufferSize]];
    
    [self addCheckbox:@"Use Timestamps" button:&_useTimestampButton 
              checked:currentDevice.useTimeTag atY:&y inView:contentView];
    
    [self addCheckbox:@"Use Bundles" button:&_useBundlesButton 
              checked:currentDevice.useBundles atY:&y inView:contentView];
    
    // Connection Options Section
    [self addSectionHeader:@"Connection Options" atY:&y inView:contentView];
    
    [self addCheckbox:@"Enabled" button:&_enabledButton 
              checked:currentDevice.enabled atY:&y inView:contentView];
    
    [self addCheckbox:@"Auto Reconnect" button:&_autoReconnectButton 
              checked:currentDevice.autoReconnect atY:&y inView:contentView];
    
    // Buttons
    y -= 40;
    NSButton *okButton = [[NSButton alloc] initWithFrame:NSMakeRect(400, y, 80, 30)];
    [okButton setTitle:@"OK"];
    [okButton setTarget:self];
    [okButton setAction:@selector(okClicked:)];
    [contentView addSubview:okButton];
    
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(490, y, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(cancelClicked:)];
    [contentView addSubview:cancelButton];
}

- (void)addSectionHeader:(NSString*)title atY:(CGFloat*)y inView:(NSView*)view {
    *y -= 30;
    NSTextField *header = [[NSTextField alloc] initWithFrame:NSMakeRect(20, *y, 540, 25)];
    [header setStringValue:title];
    [header setFont:[NSFont boldSystemFontOfSize:14]];
    [header setBezeled:NO];
    [header setDrawsBackground:NO];
    [header setEditable:NO];
    [view addSubview:header];
    *y -= 30;
}

- (void)addLabelAndField:(NSString*)label field:(NSTextField**)field atY:(CGFloat*)y inView:(NSView*)view {
    NSTextField *labelView = [[NSTextField alloc] initWithFrame:NSMakeRect(20, *y, 150, 20)];
    [labelView setStringValue:label];
    [labelView setBezeled:NO];
    [labelView setDrawsBackground:NO];
    [labelView setEditable:NO];
    [view addSubview:labelView];
    
    *field = [[NSTextField alloc] initWithFrame:NSMakeRect(180, *y, 200, 20)];
    [view addSubview:*field];
    *y -= 30;
}

- (void)addLabelAndPopup:(NSString*)label popup:(NSPopUpButton**)popup 
                 options:(NSArray*)options atY:(CGFloat*)y inView:(NSView*)view {
    NSTextField *labelView = [[NSTextField alloc] initWithFrame:NSMakeRect(20, *y, 150, 20)];
    [labelView setStringValue:label];
    [labelView setBezeled:NO];
    [labelView setDrawsBackground:NO];
    [labelView setEditable:NO];
    [view addSubview:labelView];
    
    *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(180, *y, 200, 25)];
    [*popup addItemsWithTitles:options];
    [view addSubview:*popup];
    *y -= 35;
}

- (void)addLabelAndSlider:(NSString*)label slider:(NSSlider**)slider 
                      min:(double)min max:(double)max value:(double)value 
                     atY:(CGFloat*)y inView:(NSView*)view {
    NSTextField *labelView = [[NSTextField alloc] initWithFrame:NSMakeRect(20, *y, 150, 20)];
    [labelView setStringValue:label];
    [labelView setBezeled:NO];
    [labelView setDrawsBackground:NO];
    [labelView setEditable:NO];
    [view addSubview:labelView];
    
    *slider = [[NSSlider alloc] initWithFrame:NSMakeRect(180, *y, 150, 20)];
    [*slider setMinValue:min];
    [*slider setMaxValue:max];
    [*slider setDoubleValue:value];
    [view addSubview:*slider];
    
    NSTextField *valueLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(340, *y, 60, 20)];
    [valueLabel bind:@"value" toObject:*slider withKeyPath:@"value" options:nil];
    [valueLabel setBezeled:NO];
    [valueLabel setDrawsBackground:NO];
    [valueLabel setEditable:NO];
    [view addSubview:valueLabel];
    *y -= 30;
}

- (void)addCheckbox:(NSString*)label button:(NSButton**)button 
            checked:(BOOL)checked atY:(CGFloat*)y inView:(NSView*)view {
    *button = [[NSButton alloc] initWithFrame:NSMakeRect(20, *y, 300, 20)];
    [*button setButtonType:NSButtonTypeSwitch];
    [*button setTitle:label];
    [*button setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
    [view addSubview:*button];
    *y -= 25;
}

- (void)showDialog {
    [self.configWindow makeKeyAndOrderFront:nil];
}

- (IBAction)okClicked:(id)sender {
    // Collect all data and update currentDevice
    currentDevice.deviceId = [[_deviceIdField stringValue] UTF8String];
    currentDevice.deviceName = [[_deviceNameField stringValue] UTF8String];
    currentDevice.description = [[_descriptionField stringValue] UTF8String];
    currentDevice.protocolType = (OSCProtocolType)[_protocolTypePopup indexOfSelectedItem];
    currentDevice.networkAddress = [[_networkAddressField stringValue] UTF8String];
    currentDevice.port = [[_portField stringValue] intValue];
    currentDevice.localAddress = [[_localAddressField stringValue] UTF8String];
    currentDevice.localPort = [[_localPortField stringValue] intValue];
    currentDevice.oscAddress = [[_oscAddressField stringValue] UTF8String];
    currentDevice.oscMessage = [[_oscMessageField stringValue] UTF8String];
    currentDevice.signalLevel = [_signalLevelSlider floatValue];
    currentDevice.signalOffset = [_signalOffsetSlider floatValue];
    currentDevice.invertSignal = [_invertSignalButton state] == NSControlStateValueOn;
    currentDevice.timeoutMs = [[_timeoutField stringValue] intValue];
    currentDevice.maxRetries = [[_maxRetriesField stringValue] intValue];
    currentDevice.bufferSize = [[_bufferSizeField stringValue] intValue];
    currentDevice.useTimeTag = [_useTimestampButton state] == NSControlStateValueOn;
    currentDevice.useBundles = [_useBundlesButton state] == NSControlStateValueOn;
    currentDevice.enabled = [_enabledButton state] == NSControlStateValueOn;
    currentDevice.autoReconnect = [_autoReconnectButton state] == NSControlStateValueOn;
    
    [self.configWindow close];
    if (completionHandler) {
        completionHandler(currentDevice, NO);
    }
}

- (IBAction)cancelClicked:(id)sender {
    [self.configWindow close];
    if (completionHandler) {
        completionHandler(currentDevice, YES);
    }
}

@end

// Implementation of OSCDeviceRowView
@implementation OSCDeviceRowView

- (instancetype)initWithDevice:(OSCDeviceConfig*)device {
    self = [super initWithFrame:NSMakeRect(0, 0, 300, 40)];
    if (self) {
        _device = device;
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    // Device name
    _deviceNameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(5, 20, 150, 15)];
    [_deviceNameLabel setStringValue:[NSString stringWithUTF8String:_device->deviceName.c_str()]];
    [_deviceNameLabel setFont:[NSFont boldSystemFontOfSize:12]];
    [_deviceNameLabel setBezeled:NO];
    [_deviceNameLabel setDrawsBackground:NO];
    [_deviceNameLabel setEditable:NO];
    [self addSubview:_deviceNameLabel];
    
    // Connection info
    _connectionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(5, 5, 150, 12)];
    [_connectionLabel setStringValue:[NSString stringWithUTF8String:_device->getConnectionString().c_str()]];
    [_connectionLabel setFont:[NSFont systemFontOfSize:10]];
    [_connectionLabel setTextColor:[NSColor grayColor]];
    [_connectionLabel setBezeled:NO];
    [_connectionLabel setDrawsBackground:NO];
    [_connectionLabel setEditable:NO];
    [self addSubview:_connectionLabel];
    
    // Status indicator
    _statusIndicator = [[NSView alloc] initWithFrame:NSMakeRect(160, 15, 10, 10)];
    [_statusIndicator setWantsLayer:YES];
    _statusIndicator.layer.cornerRadius = 5;
    _statusIndicator.layer.backgroundColor = [NSColor redColor].CGColor;
    [self addSubview:_statusIndicator];
    
    // Config button
    _configButton = [[NSButton alloc] initWithFrame:NSMakeRect(180, 10, 50, 20)];
    [_configButton setTitle:@"⚙️"];
    [_configButton setFont:[NSFont systemFontOfSize:12]];
    [_configButton setTarget:self];
    [_configButton setAction:@selector(configClicked:)];
    [self addSubview:_configButton];
    
    // Remove button
    _removeButton = [[NSButton alloc] initWithFrame:NSMakeRect(240, 10, 50, 20)];
    [_removeButton setTitle:@"🗑️"];
    [_removeButton setFont:[NSFont systemFontOfSize:12]];
    [_removeButton setTarget:self];
    [_removeButton setAction:@selector(removeClicked:)];
    [self addSubview:_removeButton];
}

- (void)updateStatus:(DeviceStatus*)status {
    if (status && status->isConnected()) {
        _statusIndicator.layer.backgroundColor = [NSColor greenColor].CGColor;
    } else if (status && status->hasError()) {
        _statusIndicator.layer.backgroundColor = [NSColor redColor].CGColor;
    } else {
        _statusIndicator.layer.backgroundColor = [NSColor orangeColor].CGColor;
    }
}

- (IBAction)configClicked:(id)sender {
    if (_delegate && [_delegate respondsToSelector:@selector(configureDevice:)]) {
        [_delegate performSelector:@selector(configureDevice:) withObject:[NSValue valueWithPointer:_device]];
    }
}

- (IBAction)removeClicked:(id)sender {
    if (_delegate && [_delegate respondsToSelector:@selector(removeDevice:)]) {
        [_delegate performSelector:@selector(removeDevice:) withObject:[NSValue valueWithPointer:_device]];
    }
}

@end

// Implementation of ProfessionalChannelStrip
@implementation ProfessionalChannelStrip

- (instancetype)initWithChannelId:(int)channelId {
    self = [super initWithFrame:NSMakeRect(0, 0, 150, 600)];
    if (self) {
        _channelId = channelId;
        [self setupChannelStrip];
    }
    return self;
}

- (void)setupChannelStrip {
    [self setWantsLayer:YES];
    self.layer.backgroundColor = [NSColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0].CGColor;
    self.layer.cornerRadius = 8;
    self.layer.borderWidth = 1;
    self.layer.borderColor = [NSColor grayColor].CGColor;
    
    CGFloat y = 570;
    
    // Channel label
    _channelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y, 130, 20)];
    [_channelLabel setStringValue:[NSString stringWithFormat:@"Channel %d", _channelId + 1]];
    [_channelLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [_channelLabel setTextColor:[NSColor whiteColor]];
    [_channelLabel setBezeled:NO];
    [_channelLabel setDrawsBackground:NO];
    [_channelLabel setEditable:YES];
    [_channelLabel setAlignment:NSTextAlignmentCenter];
    [self addSubview:_channelLabel];
    y -= 30;
    
    // Input devices section
    NSTextField *inputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y, 130, 15)];
    [inputLabel setStringValue:@"OSC RECEIVER IN"];
    [inputLabel setFont:[NSFont boldSystemFontOfSize:10]];
    [inputLabel setTextColor:[NSColor lightGrayColor]];
    [inputLabel setBezeled:NO];
    [inputLabel setDrawsBackground:NO];
    [inputLabel setEditable:NO];
    [inputLabel setAlignment:NSTextAlignmentCenter];
    [self addSubview:inputLabel];
    y -= 20;
    
    // Input devices scroll view
    _inputDevicesScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(5, y - 80, 140, 80)];
    [_inputDevicesScrollView setHasVerticalScroller:YES];
    [_inputDevicesScrollView setAutohidesScrollers:YES];
    [_inputDevicesScrollView setBorderType:NSLineBorder];
    
    _inputDevicesStack = [[NSStackView alloc] init];
    [_inputDevicesStack setOrientation:NSUserInterfaceLayoutOrientationVertical];
    [_inputDevicesStack setAlignment:NSLayoutAttributeLeading];
    [_inputDevicesStack setSpacing:2];
    [_inputDevicesScrollView setDocumentView:_inputDevicesStack];
    [self addSubview:_inputDevicesScrollView];
    y -= 90;
    
    // Add input device button
    _addInputButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, y, 130, 20)];
    [_addInputButton setTitle:@"+ Add OSC IN"];
    [_addInputButton setFont:[NSFont systemFontOfSize:10]];
    [_addInputButton setTarget:self];
    [_addInputButton setAction:@selector(addInputDevice:)];
    [self addSubview:_addInputButton];
    y -= 30;
    
    
    
    
    
    // Output devices section header
    NSTextField *outputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y, 130, 15)];
    [outputLabel setStringValue:@"OSC SENDER OUT"];
    [outputLabel setFont:[NSFont boldSystemFontOfSize:10]];
    [outputLabel setTextColor:[NSColor lightGrayColor]];
    [outputLabel setBezeled:NO];
    [outputLabel setDrawsBackground:NO];
    [outputLabel setEditable:NO];
    [outputLabel setAlignment:NSTextAlignmentCenter];
    [self addSubview:outputLabel];
    y -= 20;
    
    // Output devices scroll view
    _outputDevicesScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(5, y - 80, 140, 80)];
    [_outputDevicesScrollView setHasVerticalScroller:YES];
    [_outputDevicesScrollView setAutohidesScrollers:YES];
    [_outputDevicesScrollView setBorderType:NSLineBorder];
    
    _outputDevicesStack = [[NSStackView alloc] init];
    [_outputDevicesStack setOrientation:NSUserInterfaceLayoutOrientationVertical];
    [_outputDevicesStack setAlignment:NSLayoutAttributeLeading];
    [_outputDevicesStack setSpacing:2];
    [_outputDevicesScrollView setDocumentView:_outputDevicesStack];
    [self addSubview:_outputDevicesScrollView];
    y -= 90;
    
    // Add output device button
    _addOutputButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, y, 130, 20)];
    [_addOutputButton setTitle:@"+ Add OSC OUT"];
    [_addOutputButton setFont:[NSFont systemFontOfSize:10]];
    [_addOutputButton setTarget:self];
    [_addOutputButton setAction:@selector(addOutputDevice:)];
    [_addOutputButton setEnabled:YES]; // Explicitly enable the button
    [_addOutputButton setBackgroundColor:[NSColor colorWithRed:0.2 green:0.6 blue:0.8 alpha:1.0]];
    [self addSubview:_addOutputButton];
    y -= 30;
    
    // Basic level control for routing (minimal interface)
    NSTextField *levelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y, 130, 15)];
    [levelLabel setStringValue:@"CHANNEL LEVEL"];
    [levelLabel setFont:[NSFont boldSystemFontOfSize:9]];
    [levelLabel setTextColor:[NSColor lightGrayColor]];
    [levelLabel setBezeled:NO];
    [levelLabel setDrawsBackground:NO];
    [levelLabel setEditable:NO];
    [levelLabel setAlignment:NSTextAlignmentCenter];
    [self addSubview:levelLabel];
    y -= 18;
    
    // Level slider (essential for signal routing)
    _levelFader = [[NSSlider alloc] initWithFrame:NSMakeRect(10, y - 40, 130, 20)];
    [_levelFader setSliderType:NSSliderTypeLinear];
    [_levelFader setVertical:NO];
    [_levelFader setMinValue:0.0];
    [_levelFader setMaxValue:1.0];
    [_levelFader setDoubleValue:0.8]; // Default to 80%
    [_levelFader setTarget:self];
    [_levelFader setAction:@selector(levelChanged:)];
    [self addSubview:_levelFader];
    y -= 25;
    
    // Level display
    _levelDisplay = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y - 15, 130, 15)];
    [_levelDisplay setStringValue:@"0.8"];
    [_levelDisplay setFont:[NSFont monospacedDigitSystemFontOfSize:9 weight:NSFontWeightRegular]];
    [_levelDisplay setTextColor:[NSColor lightGrayColor]];
    [_levelDisplay setBezeled:NO];
    [_levelDisplay setDrawsBackground:NO];
    [_levelDisplay setEditable:NO];
    [_levelDisplay setAlignment:NSTextAlignmentCenter];
    [self addSubview:_levelDisplay];
    
    NSLog(@"✅ Channel %d: Output button created and enabled", _channelId);
}


- (void)updateFromChannel:(MixerChannel*)channel {
    if (!channel) return;
    
    _mixerChannel = channel;
    
    // Update UI elements
    [_channelLabel setStringValue:[NSString stringWithUTF8String:channel->channelName.c_str()]];
    
    
    // Start/Stop button removed - channels auto-start when input device is added
    // Visual indication of channel state
    if (channel->isRunning()) {
        [_channelLabel setTextColor:[NSColor greenColor]];
        // Add green border to show channel is active
        self.layer.borderColor = [NSColor colorWithRed:0.2 green:0.8 blue:0.2 alpha:0.8].CGColor;
        self.layer.borderWidth = 2.0;
        
        // Update section label to show state
        NSTextField *sectionLabel = nil;
        for (NSView *subview in self.subviews) {
            if ([subview isKindOfClass:[NSTextField class]] && 
                [[(NSTextField*)subview stringValue] isEqualToString:@"CV INPUT"]) {
                sectionLabel = (NSTextField*)subview;
                break;
            }
        }
        if (sectionLabel) {
            [sectionLabel setStringValue:@"CV INPUT • ACTIVE"];
            [sectionLabel setTextColor:[NSColor greenColor]];
        }
    } else {
        [_channelLabel setTextColor:[NSColor lightGrayColor]];
        // Remove border when inactive
        self.layer.borderColor = [NSColor grayColor].CGColor;
        self.layer.borderWidth = 1.0;
        
        // Update section label to show state
        NSTextField *sectionLabel = nil;
        for (NSView *subview in self.subviews) {
            if ([subview isKindOfClass:[NSTextField class]] && 
                [[(NSTextField*)subview stringValue] containsString:@"CV INPUT"]) {
                sectionLabel = (NSTextField*)subview;
                break;
            }
        }
        if (sectionLabel) {
            [sectionLabel setStringValue:@"CV INPUT"];
            [sectionLabel setTextColor:[NSColor lightGrayColor]];
        }
    }
    
    // Debug output button status
    NSLog(@"🔍 Channel %d: updateFromChannel - output button enabled: %d, target: %@", 
          _channelId, [_addOutputButton isEnabled], [_addOutputButton target]);
    
    // Update device lists
    [self updateDeviceLists];
}

- (void)updateDeviceLists {
    if (!_mixerChannel) return;
    
    // Clear existing device views
    for (NSView *view in [_inputDevicesStack arrangedSubviews]) {
        [_inputDevicesStack removeArrangedSubview:view];
        [view removeFromSuperview];
    }
    
    for (NSView *view in [_outputDevicesStack arrangedSubviews]) {
        [_outputDevicesStack removeArrangedSubview:view];
        [view removeFromSuperview];
    }
    
    // Add input devices
    for (auto& device : _mixerChannel->inputDevices) {
        OSCDeviceRowView *deviceRow = [[OSCDeviceRowView alloc] initWithDevice:&device];
        deviceRow.delegate = self;
        [_inputDevicesStack addArrangedSubview:deviceRow];
    }
    
    // Add output devices
    for (auto& device : _mixerChannel->outputDevices) {
        OSCDeviceRowView *deviceRow = [[OSCDeviceRowView alloc] initWithDevice:&device];
        deviceRow.delegate = self;
        [_outputDevicesStack addArrangedSubview:deviceRow];
    }
    
    NSLog(@"📋 Channel %d: Updated device lists - Input devices: %zu, Output devices: %zu", 
          _channelId, _mixerChannel->inputDevices.size(), _mixerChannel->outputDevices.size());
}

// Volume meter functions removed - simplified OSC routing only

// updateMeterView function removed - simplified OSC routing only

// Action methods for level control
- (IBAction)levelChanged:(id)sender {
    NSLog(@"✅ Channel %d: Level changed to %.2f", _channelId, [_levelFader doubleValue]);
    if (_mixerChannel && [_delegate respondsToSelector:@selector(channelLevelChanged:value:)]) {
        _mixerChannel->levelVolts = [_levelFader doubleValue];
        [_levelDisplay setStringValue:[NSString stringWithFormat:@"%.2f", _mixerChannel->levelVolts]];
        [_delegate performSelector:@selector(channelLevelChanged:value:) 
                        withObject:[NSNumber numberWithInt:_channelId]
                        withObject:[NSNumber numberWithDouble:_mixerChannel->levelVolts]];
    }
}

// Start/Stop button handler removed - channels auto-start when input device is added

// Mute, Solo, Link handlers removed - simplified OSC routing only

- (IBAction)addInputDevice:(id)sender {
    NSLog(@"➕ Channel %d: Add Input Device button clicked", _channelId);
    [self showDeviceSelectionDialog:YES]; // true for input devices
}

- (IBAction)addOutputDevice:(id)sender {
    NSLog(@"➕ Channel %d: Add Output Device button clicked", _channelId);
    
    // Debug info
    NSLog(@"🔍 Debug: sender = %@, delegate = %@", sender, _delegate);
    NSLog(@"🔍 Debug: mixerChannel = %p", _mixerChannel);
    
    [self showDeviceSelectionDialog:NO]; // false for output devices
}

// Device selection dialog
- (void)showDeviceSelectionDialog:(BOOL)isInput {
    // Create device selection alert
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithFormat:@"Select %@ Device for Channel %d", isInput ? @"Input" : @"Output", _channelId + 1]];
    [alert setInformativeText:@"Choose from available devices:"];
    
    // Get real audio devices from AudioDeviceManager
    NSMutableArray *deviceTypes = [[NSMutableArray alloc] init];
    
    NSLog(@"🔍 Debug: Starting device selection dialog for %@", isInput ? @"input" : @"output");
    NSLog(@"🔍 Debug: delegate = %@", _delegate);
    NSLog(@"🔍 Debug: respondsToSelector = %d", [_delegate respondsToSelector:@selector(getAudioDeviceManager)]);
    
    if (_delegate && [_delegate respondsToSelector:@selector(getAudioDeviceManager)]) {
        ProfessionalOSCMixer *mixer = (ProfessionalOSCMixer*)_delegate;
        auto audioDeviceManager = [mixer getAudioDeviceManager];
        NSLog(@"🔍 Debug: audioDeviceManager = %p", audioDeviceManager.get());
        
        if (audioDeviceManager) {
            std::vector<AudioDeviceInfo> audioDevices;
            if (isInput) {
                audioDevices = audioDeviceManager->getInputDevices();
                NSLog(@"🔍 Debug: Found %lu input devices", audioDevices.size());
                
                // Add section header
                [deviceTypes addObject:@"📱 Real Audio Devices:"];
                
                // Add real audio input devices
                for (const auto& device : audioDevices) {
                    NSLog(@"🔍 Debug: Input device: %s (index: %d, channels: %d)", device.name.c_str(), device.index, device.maxInputChannels);
                    
                    // Create NSString from UTF8 to ensure proper encoding
                    NSString *deviceNameFromUTF8 = [NSString stringWithUTF8String:device.name.c_str()];
                    NSString *deviceName = [NSString stringWithFormat:@"🎤 %@%s", 
                                          deviceNameFromUTF8,
                                          device.isDefaultInput ? " (Default)" : ""];
                    
                    NSLog(@"🔍 Debug: Created deviceName: '%@'", deviceName);
                    NSLog(@"🔍 Debug: Device name bytes: '%s'", [deviceName UTF8String]);
                    [deviceTypes addObject:deviceName];
                }
            } else {
                audioDevices = audioDeviceManager->getOutputDevices();
                NSLog(@"🔍 Debug: Found %lu output devices", audioDevices.size());
                
                // Add section header
                [deviceTypes addObject:@"📱 Real Audio Devices:"];
                
                // Add real audio output devices
                for (const auto& device : audioDevices) {
                    NSLog(@"🔍 Debug: Output device: %s (index: %d, channels: %d)", device.name.c_str(), device.index, device.maxOutputChannels);
                    
                    // Create NSString from UTF8 to ensure proper encoding
                    NSString *deviceNameFromUTF8 = [NSString stringWithUTF8String:device.name.c_str()];
                    NSString *deviceName = [NSString stringWithFormat:@"🔊 %@%s", 
                                          deviceNameFromUTF8,
                                          device.isDefaultOutput ? " (Default)" : ""];
                    
                    NSLog(@"🔍 Debug: Created deviceName: '%@'", deviceName);
                    NSLog(@"🔍 Debug: Device name bytes: '%s'", [deviceName UTF8String]);
                    [deviceTypes addObject:deviceName];
                }
            }
        } else {
            NSLog(@"🔍 Debug: audioDeviceManager is null");
        }
    } else {
        NSLog(@"🔍 Debug: delegate doesn't respond to getAudioDeviceManager");
    }
    
    // Add separator and virtual OSC devices
    if ([deviceTypes count] > 0) {
        [deviceTypes addObject:@"────────────────────"];
        [deviceTypes addObject:@"📡 Virtual OSC Devices:"];
    }
    
    // Add OSC device types
    if (isInput) {
        [deviceTypes addObjectsFromArray:@[
            @"📡 OSC Input (TouchOSC)",
            @"📡 OSC Input (Max/MSP)",
            @"🔌 CV Input (Expert Sleepers)",
            @"⚙️ Manual Configuration"
        ]];
    } else {
        [deviceTypes addObjectsFromArray:@[
            @"📡 OSC Output (TouchOSC)",
            @"📡 OSC Output (Ableton Live)", 
            @"📡 OSC Output (Max/MSP)",
            @"📡 OSC Output (TouchDesigner)",
            @"🔌 CV Output (Expert Sleepers)",
            @"⚙️ Manual Configuration"
        ]];
    }
    
    // Create a proper vertical list dialog instead of horizontal buttons
    NSWindow *deviceSelectionWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 500)
                                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                                     backing:NSBackingStoreBuffered
                                                                       defer:NO];
    [deviceSelectionWindow setTitle:[NSString stringWithFormat:@"Select %@ Device", isInput ? @"Input" : @"Output"]];
    [deviceSelectionWindow center];
    
    NSView *contentView = deviceSelectionWindow.contentView;
    
    // Create scroll view for device list
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 60, 360, 380)];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutohidesScrollers:YES];
    [scrollView setBorderType:NSLineBorder];
    
    // Create table view for devices
    NSTableView *tableView = [[NSTableView alloc] init];
    NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"device"];
    [column setTitle:@"Available Devices"];
    [column setWidth:340];
    [tableView addTableColumn:column];
    [tableView setDataSource:(id<NSTableViewDataSource>)self];
    [tableView setDelegate:(id<NSTableViewDelegate>)self];
    
    // Store device types for table view
    _deviceSelectionList = deviceTypes;
    
    [scrollView setDocumentView:tableView];
    [contentView addSubview:scrollView];
    
    // Add instruction label
    NSTextField *instructionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 450, 360, 30)];
    [instructionLabel setStringValue:@"Select a device from the list below:"];
    [instructionLabel setBezeled:NO];
    [instructionLabel setDrawsBackground:NO];
    [instructionLabel setEditable:NO];
    [instructionLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [contentView addSubview:instructionLabel];
    
    // Add buttons
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(220, 20, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(cancelDeviceSelection:)];
    [contentView addSubview:cancelButton];
    
    NSButton *selectButton = [[NSButton alloc] initWithFrame:NSMakeRect(310, 20, 80, 30)];
    [selectButton setTitle:@"Select"];
    [selectButton setTarget:self];
    [selectButton setAction:@selector(selectDevice:)];
    [selectButton setKeyEquivalent:@"\r"];
    [contentView addSubview:selectButton];
    
    // Store references for callbacks
    _deviceSelectionWindow = deviceSelectionWindow;
    _deviceSelectionTableView = tableView;
    _isInputDeviceSelection = isInput;
    
    [tableView reloadData];
    
    // Show the window modally
    [NSApp runModalForWindow:deviceSelectionWindow];
    [deviceSelectionWindow orderOut:nil];
    
    // Check if a device was selected
    NSInteger selectedRow = [tableView selectedRow];
    if (selectedRow >= 0 && selectedRow < [deviceTypes count] && !_deviceSelectionCancelled) {
        NSString *selectedDevice = deviceTypes[selectedRow];
        NSInteger response = selectedRow; // Use row index as response
        
        // Skip header and separator items
        if ([selectedDevice containsString:@"Real Audio Devices:"] || 
            [selectedDevice containsString:@"Virtual OSC Devices:"] ||
            [selectedDevice containsString:@"────────────────────"]) {
            NSLog(@"⚠️ Invalid selection: %@", selectedDevice);
            return;
        }
        
        // Create device configuration based on selection
        OSCDeviceConfig *newDevice = new OSCDeviceConfig();
        auto timestamp = time(nullptr);
        
        // Handle real audio devices (with 🎤 or 🔊 emoji)
        if ([selectedDevice hasPrefix:@"🎤"] || [selectedDevice hasPrefix:@"🔊"]) {
            // Extract device name by removing emoji and cleaning up
            NSString *cleanDeviceName = selectedDevice;
            cleanDeviceName = [cleanDeviceName stringByReplacingOccurrencesOfString:@"🎤 " withString:@""];
            cleanDeviceName = [cleanDeviceName stringByReplacingOccurrencesOfString:@"🔊 " withString:@""];
            cleanDeviceName = [cleanDeviceName stringByReplacingOccurrencesOfString:@" (Default)" withString:@""];
            
            NSLog(@"🔍 Debug: cleanDeviceName = '%@'", cleanDeviceName);
            NSLog(@"🔍 Debug: cleanDeviceName UTF8 bytes = '%s'", [cleanDeviceName UTF8String]);
            
            // Find the actual AudioDeviceInfo for this device
            if (_delegate && [_delegate respondsToSelector:@selector(getAudioDeviceManager)]) {
                ProfessionalOSCMixer *mixer = (ProfessionalOSCMixer*)_delegate;
                auto audioDeviceManager = [mixer getAudioDeviceManager];
                if (audioDeviceManager) {
                    // Try exact match first
                    AudioDeviceInfo deviceInfo = audioDeviceManager->findDeviceByName([cleanDeviceName UTF8String]);
                    
                    // If exact match fails, try finding by partial match through all devices
                    if (deviceInfo.index == -1) {
                        NSLog(@"🔍 Debug: Exact match failed, trying partial match for '%@'", cleanDeviceName);
                        std::vector<AudioDeviceInfo> allDevices = isInput ? audioDeviceManager->getInputDevices() : audioDeviceManager->getOutputDevices();
                        for (const auto& device : allDevices) {
                            NSString *deviceNameNS = [NSString stringWithUTF8String:device.name.c_str()];
                            NSLog(@"🔍 Debug: Comparing with device: '%@'", deviceNameNS);
                            if ([deviceNameNS isEqualToString:cleanDeviceName]) {
                                deviceInfo = device;
                                NSLog(@"✅ Found device by exact NSString match: %s", device.name.c_str());
                                break;
                            }
                        }
                    }
                    
                    if (deviceInfo.index != -1) {
                        // Real audio device found
                        if (isInput && deviceInfo.maxInputChannels > 0) {
                            newDevice->deviceType = OSCDeviceType::AUDIO_INPUT;
                            newDevice->deviceId = "real_audio_input_" + std::to_string(_channelId) + "_" + std::to_string(deviceInfo.index);
                            newDevice->deviceName = deviceInfo.name;
                            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1) + "/audio/in";
                            newDevice->networkAddress = "127.0.0.1";
                            newDevice->port = 9000 + _channelId;
                            newDevice->localPort = 8000 + _channelId;
                            newDevice->audioDeviceIndex = deviceInfo.index;
                            NSLog(@"✅ Configured real audio input device: %s (index: %d)", deviceInfo.name.c_str(), deviceInfo.index);
                        } else if (!isInput && deviceInfo.maxOutputChannels > 0) {
                            newDevice->deviceType = OSCDeviceType::AUDIO_OUTPUT;
                            newDevice->deviceId = "real_audio_output_" + std::to_string(_channelId) + "_" + std::to_string(deviceInfo.index);
                            newDevice->deviceName = deviceInfo.name;
                            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1) + "/audio/out";
                            newDevice->networkAddress = "127.0.0.1";
                            newDevice->port = 9000 + _channelId;
                            newDevice->localPort = 0;
                            newDevice->audioDeviceIndex = deviceInfo.index;
                            NSLog(@"✅ Configured real audio output device: %s (index: %d)", deviceInfo.name.c_str(), deviceInfo.index);
                        } else {
                            NSLog(@"❌ Selected device doesn't support required direction");
                            delete newDevice;
                            return;
                        }
                    } else {
                        NSLog(@"❌ Could not find audio device: %@", cleanDeviceName);
                        delete newDevice;
                        return;
                    }
                }
            }
        } else if ([selectedDevice containsString:@"Microphone"] || [selectedDevice containsString:@"Audio Interface"]) {
            // Legacy audio input device (fallback)
            newDevice->deviceType = OSCDeviceType::AUDIO_INPUT;
            newDevice->deviceId = "audio_input_" + std::to_string(_channelId) + "_" + std::to_string(timestamp);
            newDevice->deviceName = [selectedDevice UTF8String];
            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1) + "/audio/in";
            newDevice->networkAddress = "127.0.0.1";
            newDevice->port = 9000 + _channelId;
            newDevice->localPort = 8000 + _channelId;
        } else if ([selectedDevice containsString:@"Speakers"] || [selectedDevice containsString:@"Audio Interface"]) {
            // Legacy audio output device (fallback)
            newDevice->deviceType = OSCDeviceType::AUDIO_OUTPUT;
            newDevice->deviceId = "audio_output_" + std::to_string(_channelId) + "_" + std::to_string(timestamp);
            newDevice->deviceName = [selectedDevice UTF8String];
            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1) + "/audio/out";
            newDevice->networkAddress = "127.0.0.1";
            newDevice->port = 9000 + _channelId;
            newDevice->localPort = 0;
        } else if ([selectedDevice containsString:@"OSC"]) {
            // OSC device
            newDevice->deviceType = isInput ? OSCDeviceType::OSC_INPUT : OSCDeviceType::OSC_OUTPUT;
            newDevice->deviceId = (isInput ? "osc_input_" : "osc_output_") + std::to_string(_channelId) + "_" + std::to_string(timestamp);
            newDevice->deviceName = [selectedDevice UTF8String];
            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1);
            
            if ([selectedDevice containsString:@"TouchOSC"]) {
                newDevice->networkAddress = "192.168.1.100";
                newDevice->port = 8000 + _channelId;
            } else if ([selectedDevice containsString:@"Ableton"]) {
                newDevice->networkAddress = "127.0.0.1";
                newDevice->port = 9001;
            } else if ([selectedDevice containsString:@"Max/MSP"]) {
                newDevice->networkAddress = "127.0.0.1";
                newDevice->port = 7000 + _channelId;
            } else if ([selectedDevice containsString:@"TouchDesigner"]) {
                newDevice->networkAddress = "127.0.0.1";
                newDevice->port = 9000;
            } else {
                newDevice->networkAddress = "127.0.0.1";
                newDevice->port = 8000 + _channelId;
            }
            newDevice->localPort = isInput ? (8000 + _channelId) : 0;
        } else if ([selectedDevice containsString:@"CV"]) {
            // CV device
            newDevice->deviceType = isInput ? OSCDeviceType::CV_INPUT : OSCDeviceType::CV_OUTPUT;
            newDevice->deviceId = (isInput ? "cv_input_" : "cv_output_") + std::to_string(_channelId) + "_" + std::to_string(timestamp);
            newDevice->deviceName = [selectedDevice UTF8String];
            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1) + "/cv";
            newDevice->networkAddress = "127.0.0.1";
            newDevice->port = 9020 + _channelId;
            newDevice->localPort = isInput ? (8020 + _channelId) : 0;
        } else {
            // Manual configuration
            newDevice->deviceType = isInput ? OSCDeviceType::OSC_INPUT : OSCDeviceType::OSC_OUTPUT;
            newDevice->deviceId = (isInput ? "manual_input_" : "manual_output_") + std::to_string(_channelId) + "_" + std::to_string(timestamp);
            newDevice->deviceName = "Manual " + std::string(isInput ? "Input" : "Output") + " Device";
            newDevice->oscAddress = "/channel/" + std::to_string(_channelId + 1);
            newDevice->networkAddress = "127.0.0.1";
            newDevice->port = 9000 + _channelId;
            newDevice->localPort = isInput ? (9000 + _channelId) : 0;
        }
        
        newDevice->enabled = true;
        newDevice->connected = true;
        newDevice->protocolType = OSCProtocolType::UDP_UNICAST;
        
        // Add the device
        if (isInput && [_delegate respondsToSelector:@selector(addInputDevice:toChannel:)]) {
            NSLog(@"🔄 Calling addInputDevice delegate method");
            [_delegate performSelector:@selector(addInputDevice:toChannel:) 
                            withObject:[NSValue valueWithPointer:newDevice]
                            withObject:[NSNumber numberWithInt:_channelId]];
        } else if (!isInput && [_delegate respondsToSelector:@selector(addOutputDevice:toChannel:)]) {
            NSLog(@"🔄 Calling addOutputDevice delegate method");
            [_delegate performSelector:@selector(addOutputDevice:toChannel:) 
                            withObject:[NSValue valueWithPointer:newDevice]
                            withObject:[NSNumber numberWithInt:_channelId]];
        } else {
            NSLog(@"❌ Delegate method not available: isInput=%d, delegate=%@", isInput, _delegate);
            if (_delegate) {
                NSLog(@"❌ Available methods: addInputDevice = %d, addOutputDevice = %d", 
                      [_delegate respondsToSelector:@selector(addInputDevice:toChannel:)],
                      [_delegate respondsToSelector:@selector(addOutputDevice:toChannel:)]);
            }
        }
        
        NSLog(@"✅ Added %@ device '%@' to channel %d", isInput ? @"input" : @"output", 
              [NSString stringWithUTF8String:newDevice->deviceName.c_str()], _channelId + 1);
    }
}

// Device delegate methods
- (void)configureDevice:(NSValue*)devicePtr {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    OSCDeviceConfigDialog *dialog = [[OSCDeviceConfigDialog alloc] 
        initWithDevice:device completion:^(OSCDeviceConfig config, BOOL cancelled) {
            if (!cancelled && [self->_delegate respondsToSelector:@selector(updateDevice:)]) {
                *device = config;
                [self->_delegate performSelector:@selector(updateDevice:) 
                                      withObject:[NSValue valueWithPointer:device]];
            }
        }];
    [dialog showDialog];
}

- (void)removeDevice:(NSValue*)devicePtr {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    if ([_delegate respondsToSelector:@selector(removeDevice:fromChannel:)]) {
        [_delegate performSelector:@selector(removeDevice:fromChannel:) 
                        withObject:[NSValue valueWithPointer:device]
                        withObject:[NSNumber numberWithInt:_channelId]];
    }
}

// Device selection dialog callbacks
- (void)cancelDeviceSelection:(id)sender {
    _deviceSelectionCancelled = YES;
    [NSApp stopModal];
}

- (void)selectDevice:(id)sender {
    _deviceSelectionCancelled = NO;
    [NSApp stopModal];
}

// NSTableViewDataSource methods
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView == _deviceSelectionTableView) {
        return [_deviceSelectionList count];
    }
    return 0;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (tableView == _deviceSelectionTableView && row < [_deviceSelectionList count]) {
        NSString *deviceName = _deviceSelectionList[row];
        
        // Style header and separator rows differently
        if ([deviceName containsString:@"Real Audio Devices:"] || 
            [deviceName containsString:@"Virtual OSC Devices:"]) {
            return [[NSAttributedString alloc] initWithString:deviceName 
                                                   attributes:@{NSFontAttributeName: [NSFont boldSystemFontOfSize:12],
                                                               NSForegroundColorAttributeName: [NSColor systemBlueColor]}];
        } else if ([deviceName containsString:@"────────────────────"]) {
            return [[NSAttributedString alloc] initWithString:@"" attributes:nil]; // Empty for separator
        } else {
            return deviceName;
        }
    }
    return nil;
}

// NSTableViewDelegate methods
- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
    if (tableView == _deviceSelectionTableView && row < [_deviceSelectionList count]) {
        NSString *deviceName = _deviceSelectionList[row];
        // Don't allow selection of headers and separators
        return !([deviceName containsString:@"Real Audio Devices:"] || 
                [deviceName containsString:@"Virtual OSC Devices:"] ||
                [deviceName containsString:@"────────────────────"]);
    }
    return YES;
}

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row {
    if (tableView == _deviceSelectionTableView && row < [_deviceSelectionList count]) {
        NSString *deviceName = _deviceSelectionList[row];
        if ([deviceName containsString:@"────────────────────"]) {
            return 10; // Smaller height for separator
        } else if ([deviceName containsString:@"Real Audio Devices:"] || 
                  [deviceName containsString:@"Virtual OSC Devices:"]) {
            return 25; // Slightly taller for headers
        }
    }
    return 20; // Default height for regular items
}

@end

// Implementation of ProfessionalOSCMixer
@implementation ProfessionalOSCMixer

- (instancetype)initWithMixerEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    self = [super init];
    if (self) {
        _mixerEngine = engine;
        _channelStrips = [[NSMutableArray alloc] init];
        
        // Initialize AudioDeviceManager
        _audioDeviceManager = std::make_shared<AudioDeviceManager>();
        if (!_audioDeviceManager->initialize()) {
            NSLog(@"⚠️ Failed to initialize AudioDeviceManager");
        } else {
            NSLog(@"✅ AudioDeviceManager initialized successfully");
            _audioDeviceManager->refreshDeviceList();
        }
        
        [self setupMainWindow];
        [self setupUI];
        [self startUpdateTimer];
    }
    return self;
}

- (void)setupMainWindow {
    NSRect windowFrame = NSMakeRect(0, 0, 1400, 800);
    _mainWindow = [[NSWindow alloc] initWithContentRect:windowFrame
                                             styleMask:(NSWindowStyleMaskTitled | 
                                                      NSWindowStyleMaskClosable | 
                                                      NSWindowStyleMaskMiniaturizable | 
                                                      NSWindowStyleMaskResizable)
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    [_mainWindow setTitle:@"Professional OSC Mixer"];
    [_mainWindow center];
    [_mainWindow setMinSize:NSMakeSize(800, 600)];
    
    // Set dark appearance
    if (@available(macOS 10.14, *)) {
        [_mainWindow setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
    }
}

- (void)setupUI {
    NSView *contentView = _mainWindow.contentView;
    [contentView setWantsLayer:YES];
    contentView.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0].CGColor;
    
    // Channel strip scroll view
    _channelScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 60, 1200, 600)];
    [_channelScrollView setHasHorizontalScroller:YES];
    [_channelScrollView setHasVerticalScroller:NO];
    [_channelScrollView setAutohidesScrollers:YES];
    [contentView addSubview:_channelScrollView];
    
    // Create container view for channels with explicit size
    NSView *channelContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 8 * 160, 600)];
    [_channelScrollView setDocumentView:channelContainer];
    
    // Create 8 channel strips with explicit positioning
    for (int i = 0; i < 8; i++) {
        ProfessionalChannelStrip *channelStrip = [[ProfessionalChannelStrip alloc] initWithChannelId:i];
        channelStrip.delegate = self;
        [_channelStrips addObject:channelStrip];
        
        // Position each channel strip manually
        NSRect channelFrame = NSMakeRect(i * 160, 0, 150, 600);
        [channelStrip setFrame:channelFrame];
        [channelContainer addSubview:channelStrip];
    }
    
    // Master section
    _masterSection = [[NSView alloc] initWithFrame:NSMakeRect(1240, 60, 140, 600)];
    [_masterSection setWantsLayer:YES];
    _masterSection.layer.backgroundColor = [NSColor colorWithRed:0.15 green:0.15 blue:0.15 alpha:1.0].CGColor;
    _masterSection.layer.cornerRadius = 8;
    [contentView addSubview:_masterSection];
    
    // Master volume controls removed - simplified OSC routing only
    NSTextField *masterLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 570, 120, 20)];
    [masterLabel setStringValue:@"OSC ROUTER"];
    [masterLabel setFont:[NSFont boldSystemFontOfSize:16]];
    [masterLabel setTextColor:[NSColor whiteColor]];
    [masterLabel setBezeled:NO];
    [masterLabel setDrawsBackground:NO];
    [masterLabel setEditable:NO];
    [masterLabel setAlignment:NSTextAlignmentCenter];
    [_masterSection addSubview:masterLabel];
    
    // Control bar at bottom
    NSView *controlBar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1400, 50)];
    [controlBar setWantsLayer:YES];
    controlBar.layer.backgroundColor = [NSColor colorWithRed:0.05 green:0.05 blue:0.05 alpha:1.0].CGColor;
    [contentView addSubview:controlBar];
    
    // Status label
    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 15, 400, 20)];
    [_statusLabel setStringValue:@"Professional OSC Mixer - Ready"];
    [_statusLabel setTextColor:[NSColor lightGrayColor]];
    [_statusLabel setBezeled:NO];
    [_statusLabel setDrawsBackground:NO];
    [_statusLabel setEditable:NO];
    [controlBar addSubview:_statusLabel];
    
    // Scan devices button
    _scanDevicesButton = [[NSButton alloc] initWithFrame:NSMakeRect(450, 10, 120, 30)];
    [_scanDevicesButton setTitle:@"Scan Devices"];
    [_scanDevicesButton setTarget:self];
    [_scanDevicesButton setAction:@selector(scanDevicesClicked:)];
    [controlBar addSubview:_scanDevicesButton];
    
    // Configuration controls
    _configurationPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(600, 10, 150, 30)];
    [_configurationPopup addItemWithTitle:@"Default Configuration"];
    [controlBar addSubview:_configurationPopup];
    
    _saveConfigButton = [[NSButton alloc] initWithFrame:NSMakeRect(770, 10, 80, 30)];
    [_saveConfigButton setTitle:@"Save"];
    [_saveConfigButton setTarget:self];
    [_saveConfigButton setAction:@selector(saveConfigClicked:)];
    [controlBar addSubview:_saveConfigButton];
    
    _loadConfigButton = [[NSButton alloc] initWithFrame:NSMakeRect(860, 10, 80, 30)];
    [_loadConfigButton setTitle:@"Load"];
    [_loadConfigButton setTarget:self];
    [_loadConfigButton setAction:@selector(loadConfigClicked:)];
    [controlBar addSubview:_loadConfigButton];
}

- (void)startUpdateTimer {
    _updateTimer = [NSTimer scheduledTimerWithTimeInterval:0.05 // 20 FPS
                                                    target:self
                                                  selector:@selector(updateDisplay)
                                                  userInfo:nil
                                                   repeats:YES];
}

- (void)updateDisplay {
    if (!_mixerEngine) return;
    
    auto mixerState = _mixerEngine->getMixerState();
    if (!mixerState) return;
    
    // Update channel strips
    for (int i = 0; i < [_channelStrips count] && i < 8; i++) {
        ProfessionalChannelStrip *strip = _channelStrips[i];
        auto channel = mixerState->getChannel(i);
        if (channel) {
            [strip updateFromChannel:channel];
            // updateMeters removed - simplified OSC routing only
        }
    }
    
    // Master volume controls removed - simplified OSC routing only
    
    // Update status
    [_statusLabel setStringValue:[NSString stringWithFormat:@"Active: %d channels, %d devices, %d msg/sec", 
                                 (int)mixerState->getRunningChannels().size(),
                                 mixerState->getTotalDeviceCount(),
                                 mixerState->totalMessagesPerSecond.load()]];
}

// Master meter removed - simplified OSC routing only

- (std::shared_ptr<AudioDeviceManager>)getAudioDeviceManager {
    return _audioDeviceManager;
}

- (void)showWindow:(id)sender {
    [_mainWindow makeKeyAndOrderFront:sender];
}

// Master volume action methods removed - simplified OSC routing only

- (IBAction)scanDevicesClicked:(id)sender {
    NSLog(@"🔍 Scan Devices button clicked");
    if (_mixerEngine) {
        _mixerEngine->startDeviceDiscovery();
        [_statusLabel setStringValue:@"Scanning for OSC devices..."];
    }
}

- (IBAction)saveConfigClicked:(id)sender {
    NSLog(@"💾 Save Config button clicked");
    NSSavePanel *savePanel = [NSSavePanel savePanel];
    [savePanel setAllowedFileTypes:@[@"json"]];
    [savePanel setNameFieldStringValue:@"mixer_config.json"];
    
    [savePanel beginSheetModalForWindow:_mainWindow completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK && self->_mixerEngine) {
            NSURL *url = [savePanel URL];
            NSString *path = [url path];
            NSLog(@"💾 Saving config to: %@", path);
            if (self->_mixerEngine->saveConfiguration([path UTF8String])) {
                [self->_statusLabel setStringValue:@"Configuration saved successfully"];
            } else {
                [self->_statusLabel setStringValue:@"Failed to save configuration"];
            }
        }
    }];
}

- (IBAction)loadConfigClicked:(id)sender {
    NSLog(@"📂 Load Config button clicked");
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setAllowedFileTypes:@[@"json"]];
    
    [openPanel beginSheetModalForWindow:_mainWindow completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK && self->_mixerEngine) {
            NSURL *url = [openPanel.URLs firstObject];
            NSString *path = [url path];
            NSLog(@"📂 Loading config from: %@", path);
            if (self->_mixerEngine->loadConfiguration([path UTF8String])) {
                [self->_statusLabel setStringValue:@"Configuration loaded successfully"];
                [self updateDisplay];
            } else {
                [self->_statusLabel setStringValue:@"Failed to load configuration"];
            }
        }
    }];
}

// Channel control delegate methods
- (void)channelLevelChanged:(NSNumber*)channelId value:(NSNumber*)value {
    if (_mixerEngine) {
        _mixerEngine->setChannelLevel([channelId intValue], [value floatValue]);
        NSLog(@"🔊 Channel %d level set to %.2f", [channelId intValue] + 1, [value floatValue]);
    }
}

- (void)channelLinkClicked:(NSNumber*)channelId {
    // Link functionality removed - simplified OSC routing only
}

- (void)addInputDevice:(NSValue*)devicePtr toChannel:(NSNumber*)channelId {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    if (_mixerEngine) {
        _mixerEngine->addInputDevice([channelId intValue], *device);
    }
}

- (void)addOutputDevice:(NSValue*)devicePtr toChannel:(NSNumber*)channelId {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    if (_mixerEngine) {
        _mixerEngine->addOutputDevice([channelId intValue], *device);
    }
}

- (void)updateDevice:(NSValue*)devicePtr {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    if (_mixerEngine) {
        _mixerEngine->updateDeviceConfig(device->deviceId, *device);
    }
}

- (void)removeDevice:(NSValue*)devicePtr fromChannel:(NSNumber*)channelId {
    OSCDeviceConfig *device = (OSCDeviceConfig*)[devicePtr pointerValue];
    if (_mixerEngine && device) {
        // Determine if it's input or output device and remove accordingly
        auto mixerState = _mixerEngine->getMixerState();
        auto channel = mixerState->getChannel([channelId intValue]);
        if (channel) {
            bool found = false;
            for (const auto& inputDevice : channel->inputDevices) {
                if (inputDevice.deviceId == device->deviceId) {
                    _mixerEngine->removeInputDevice([channelId intValue], device->deviceId);
                    found = true;
                    break;
                }
            }
            if (!found) {
                _mixerEngine->removeOutputDevice([channelId intValue], device->deviceId);
            }
        }
    }
}

- (void)dealloc {
    [_updateTimer invalidate];
    [super dealloc];
}

@end
