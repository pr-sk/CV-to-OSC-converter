#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import "ProfessionalMixerWindow.h"
#include "Version.h"
#include "OSCMixerEngine.h"
#include <memory>

@interface CVToOSCAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) ProfessionalMixerWindow *oscMixerWindow;
@property std::shared_ptr<OSCMixerEngine> mixerEngine;
@end

@implementation CVToOSCAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Initialize mixer engine
    self.mixerEngine = std::make_shared<OSCMixerEngine>(8); // 8 channels
    
    // Initialize the mixer engine
    if (self.mixerEngine->initialize()) {
        NSLog(@"OSC Mixer Engine initialized successfully");
    } else {
        NSLog(@"Failed to initialize OSC Mixer Engine");
    }
    
    // Create and show OSC Mixer window directly
    self.oscMixerWindow = [[ProfessionalMixerWindow alloc] initWithEngine:self.mixerEngine];
    [self.oscMixerWindow showWindow:nil];
    
    // Set the mixer window as the main window
    [self.oscMixerWindow.window makeMainWindow];
    [self.oscMixerWindow.window makeKeyAndOrderFront:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (self.mixerEngine) {
        self.mixerEngine->shutdown();
    }
    return NSTerminateNow;
}
    
CGFloat y = contentView.frame.size.height - 50;
    
    // Title
NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 40)];
    [titleLabel setStringValue:[NSString stringWithUTF8String:Version::getAppTitle().c_str()]];
[titleLabel setFont:[NSFont boldSystemFontOfSize:18]];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [titleLabel setSelectable:NO];
    [contentView addSubview:titleLabel];
    
y -= 70;
    
// OSC Section
    NSTextField *oscSectionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 20)];
    [oscSectionLabel setStringValue:@"OSC Configuration"];
    [oscSectionLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [oscSectionLabel setBezeled:NO];
    [oscSectionLabel setDrawsBackground:NO];
    [oscSectionLabel setEditable:NO];
    [oscSectionLabel setSelectable:NO];
    [contentView addSubview:oscSectionLabel];
    
    y -= 30;
    
    NSTextField *oscReceiverLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [oscReceiverLabel setStringValue:@"OSC Receiver:"];
    [oscReceiverLabel setBezeled:NO];
    [oscReceiverLabel setDrawsBackground:NO];
    [oscReceiverLabel setEditable:NO];
    [oscReceiverLabel setSelectable:NO];
    [contentView addSubview:oscReceiverLabel];
    
    NSButton *startOscReceiverButton = [[NSButton alloc] initWithFrame:NSMakeRect(120, y, 80, 20)];
    [startOscReceiverButton setTitle:@"Start"];
    [startOscReceiverButton setTarget:self];
    [startOscReceiverButton setAction:@selector(toggleOSCReceiver:)];
    [contentView addSubview:startOscReceiverButton];
    
    y -= 30;
    
    NSTextField *oscSenderLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [oscSenderLabel setStringValue:@"OSC Sender:"];
    [oscSenderLabel setBezeled:NO];
    [oscSenderLabel setDrawsBackground:NO];
    [oscSenderLabel setEditable:NO];
    [oscSenderLabel setSelectable:NO];
    [contentView addSubview:oscSenderLabel];
    
    NSButton *startOscSenderButton = [[NSButton alloc] initWithFrame:NSMakeRect(120, y, 80, 20)];
    [startOscSenderButton setTitle:@"Start"];
    [startOscSenderButton setTarget:self];
    [startOscSenderButton setAction:@selector(toggleOSCSender:)];
    [contentView addSubview:startOscSenderButton];
    
    NSTextField *oscVisualizationLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(220, y, 150, 20)];
    [oscVisualizationLabel setStringValue:@"Signal Visualization"];
    [oscVisualizationLabel setBezeled:NO];
    [oscVisualizationLabel setDrawsBackground:NO];
    [oscVisualizationLabel setEditable:NO];
    [oscVisualizationLabel setSelectable:NO];
    [contentView addSubview:oscVisualizationLabel];
    
    y -= 30;
    
    NSTextField *oscStatusMonitoringLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [oscStatusMonitoringLabel setStringValue:@"OSC Status Monitoring"];
    [oscStatusMonitoringLabel setBezeled:NO];
    [oscStatusMonitoringLabel setDrawsBackground:NO];
    [oscStatusMonitoringLabel setEditable:NO];
    [oscStatusMonitoringLabel setSelectable:NO];
    [contentView addSubview:oscStatusMonitoringLabel];

    y -= 60;

    // CV Section
    NSTextField *cvSectionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 20)];
    [cvSectionLabel setStringValue:@"CV Configuration"];
    [cvSectionLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [cvSectionLabel setBezeled:NO];
    [cvSectionLabel setDrawsBackground:NO];
    [cvSectionLabel setEditable:NO];
    [cvSectionLabel setSelectable:NO];
    [contentView addSubview:cvSectionLabel];
    
    y -= 30;
    
    NSTextField *cvReceiverLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [cvReceiverLabel setStringValue:@"CV Receiver:"];
    [cvReceiverLabel setBezeled:NO];
    [cvReceiverLabel setDrawsBackground:NO];
    [cvReceiverLabel setEditable:NO];
    [cvReceiverLabel setSelectable:NO];
    [contentView addSubview:cvReceiverLabel];
    
    NSButton *startCvReceiverButton = [[NSButton alloc] initWithFrame:NSMakeRect(120, y, 80, 20)];
    [startCvReceiverButton setTitle:@"Start"];
    [startCvReceiverButton setTarget:self];
    [startCvReceiverButton setAction:@selector(toggleCVReceiver:)];
    [contentView addSubview:startCvReceiverButton];
    
    y -= 30;
    
    NSTextField *cvSenderLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [cvSenderLabel setStringValue:@"CV Sender:"];
    [cvSenderLabel setBezeled:NO];
    [cvSenderLabel setDrawsBackground:NO];
    [cvSenderLabel setEditable:NO];
    [cvSenderLabel setSelectable:NO];
    [contentView addSubview:cvSenderLabel];
    
    NSButton *startCvSenderButton = [[NSButton alloc] initWithFrame:NSMakeRect(120, y, 80, 20)];
    [startCvSenderButton setTitle:@"Start"];
    [startCvSenderButton setTarget:self];
    [startCvSenderButton setAction:@selector(toggleCVSender:)];
    [contentView addSubview:startCvSenderButton];
    
    NSTextField *cvMetersLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(220, y, 150, 20)];
    [cvMetersLabel setStringValue:@"Real-time CV Meters"];
    [cvMetersLabel setBezeled:NO];
    [cvMetersLabel setDrawsBackground:NO];
    [cvMetersLabel setEditable:NO];
    [cvMetersLabel setSelectable:NO];
    [contentView addSubview:cvMetersLabel];
    
    y -= 60;

    // OSC Settings Configuration Section
    NSTextField *oscConfigLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 20)];
    [oscConfigLabel setStringValue:@"OSC Network Configuration"];
    [oscConfigLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [oscConfigLabel setBezeled:NO];
    [oscConfigLabel setDrawsBackground:NO];
    [oscConfigLabel setEditable:NO];
    [oscConfigLabel setSelectable:NO];
    [contentView addSubview:oscConfigLabel];
    
    y -= 30;
    
    // OSC Sender Configuration
    NSTextField *senderHostLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [senderHostLabel setStringValue:@"Sender Host:"];
    [senderHostLabel setBezeled:NO];
    [senderHostLabel setDrawsBackground:NO];
    [senderHostLabel setEditable:NO];
    [senderHostLabel setSelectable:NO];
    [contentView addSubview:senderHostLabel];
    
    self.oscSenderHostField = [[NSTextField alloc] initWithFrame:NSMakeRect(120, y, 120, 20)];
    [self.oscSenderHostField setStringValue:@"127.0.0.1"];
    [contentView addSubview:self.oscSenderHostField];
    
    NSTextField *senderPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(260, y, 80, 20)];
    [senderPortLabel setStringValue:@"Sender Port:"];
    [senderPortLabel setBezeled:NO];
    [senderPortLabel setDrawsBackground:NO];
    [senderPortLabel setEditable:NO];
    [senderPortLabel setSelectable:NO];
    [contentView addSubview:senderPortLabel];
    
    self.oscSenderPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(350, y, 80, 20)];
    [self.oscSenderPortField setStringValue:@"9000"];
    [contentView addSubview:self.oscSenderPortField];
    
    y -= 30;
    
    // OSC Receiver Configuration
    NSTextField *receiverHostLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 100, 20)];
    [receiverHostLabel setStringValue:@"Receiver Host:"];
    [receiverHostLabel setBezeled:NO];
    [receiverHostLabel setDrawsBackground:NO];
    [receiverHostLabel setEditable:NO];
    [receiverHostLabel setSelectable:NO];
    [contentView addSubview:receiverHostLabel];
    
    self.oscReceiverHostField = [[NSTextField alloc] initWithFrame:NSMakeRect(120, y, 120, 20)];
    [self.oscReceiverHostField setStringValue:@"127.0.0.1"];
    [contentView addSubview:self.oscReceiverHostField];
    
    NSTextField *receiverPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(260, y, 100, 20)];
    [receiverPortLabel setStringValue:@"Receiver Port:"];
    [receiverPortLabel setBezeled:NO];
    [receiverPortLabel setDrawsBackground:NO];
    [receiverPortLabel setEditable:NO];
    [receiverPortLabel setSelectable:NO];
    [contentView addSubview:receiverPortLabel];
    
    self.oscReceiverPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(370, y, 80, 20)];
    [self.oscReceiverPortField setStringValue:@"8001"];
    [contentView addSubview:self.oscReceiverPortField];
    
y -= 80;
    
    // Start/Stop button
    self.startStopButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 120, 30)];
    [self.startStopButton setTitle:@"Start Conversion"];
    [self.startStopButton setTarget:self];
    [self.startStopButton setAction:@selector(startStopButtonClicked:)];
    [contentView addSubview:self.startStopButton];
    
    // Progress indicator
self.progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(160, y + 5, 200, 20)];
    [self.progressIndicator setStyle:NSProgressIndicatorStyleBar];
    [self.progressIndicator setIndeterminate:YES];
    [contentView addSubview:self.progressIndicator];
    
    y -= 50;
    
    // Status label
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 60)];
[self.statusLabel setStringValue:@"Ready to start conversion.\n\nSet up OSC and CV, then click 'Start Conversion'."];
    [self.statusLabel setBezeled:NO];
    [self.statusLabel setDrawsBackground:NO];
    [self.statusLabel setEditable:NO];
    [self.statusLabel setSelectable:YES]; // Allow text selection for copying
    [contentView addSubview:self.statusLabel];
    
    y -= 80;
    
// Settings Section
    NSTextField *settingsLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 460, 20)];
    [settingsLabel setStringValue:@"Settings"];
    [settingsLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [settingsLabel setBezeled:NO];
    [settingsLabel setDrawsBackground:NO];
    [settingsLabel setEditable:NO];
    [settingsLabel setSelectable:NO];
    [contentView addSubview:settingsLabel];
    
    y -= 30;
    
    NSButton *audioDeviceButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [audioDeviceButton setTitle:@"Audio Device Selection"];
    [audioDeviceButton setTarget:self];
    [audioDeviceButton setAction:@selector(showAudioDeviceSelection:)];
    [contentView addSubview:audioDeviceButton];
    
    y -= 30;
    
    NSButton *channelConfigButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [channelConfigButton setTitle:@"Channel Configuration"];
    [channelConfigButton setTarget:self];
    [channelConfigButton setAction:@selector(showChannelConfiguration:)];
    [contentView addSubview:channelConfigButton];
    
    y -= 30;
    
    NSButton *performanceMonitorButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [performanceMonitorButton setTitle:@"Performance Monitor"];
    [performanceMonitorButton setTarget:self];
    [performanceMonitorButton setAction:@selector(showPerformanceMonitor:)];
    [contentView addSubview:performanceMonitorButton];
    
    y -= 30;
    
    NSButton *midiControlButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [midiControlButton setTitle:@"MIDI Control"];
    [midiControlButton setTarget:self];
    [midiControlButton setAction:@selector(showMIDIControl:)];
    [contentView addSubview:midiControlButton];
    
    y -= 30;
    
    NSButton *externalDevicesButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [externalDevicesButton setTitle:@"External Devices"];
    [externalDevicesButton setTarget:self];
    [externalDevicesButton setAction:@selector(showExternalDevices:)];
    [contentView addSubview:externalDevicesButton];
    
    y -= 30;
    
    NSButton *advancedSettingsButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [advancedSettingsButton setTitle:@"Advanced Settings"];
    [advancedSettingsButton setTarget:self];
    [advancedSettingsButton setAction:@selector(showAdvancedSettings:)];
    [contentView addSubview:advancedSettingsButton];
    
    y -= 30;
    
    NSButton *calibrationButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 150, 20)];
    [calibrationButton setTitle:@"Calibration"];
    [calibrationButton setTarget:self];
    [calibrationButton setAction:@selector(showCalibration:)];
    [contentView addSubview:calibrationButton];
    
    y -= 30;
    
    // Professional OSC Mixer button
    NSButton *professionalMixerButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, y, 200, 30)];
    [professionalMixerButton setTitle:@"🎛️ Professional OSC Mixer"];
    [professionalMixerButton setFont:[NSFont boldSystemFontOfSize:13]];
    [professionalMixerButton setTarget:self];
    [professionalMixerButton setAction:@selector(showProfessionalMixer:)];
    [contentView addSubview:professionalMixerButton];
    
    y -= 60;
}

- (void)loadConfiguration {
    // Load configuration and update UI
    std::string host = self.config->getOSCHost();
    std::string port = self.config->getOSCPort();
    
    if (!host.empty()) {
        [self.oscSenderHostField setStringValue:[NSString stringWithUTF8String:host.c_str()]];
        [self.oscReceiverHostField setStringValue:[NSString stringWithUTF8String:host.c_str()]];
    }
    if (!port.empty()) {
        [self.oscSenderPortField setStringValue:[NSString stringWithUTF8String:port.c_str()]];
    }
}

- (void)saveConfiguration {
    // Save configuration from UI
    std::string senderHost = [[self.oscSenderHostField stringValue] UTF8String];
    std::string senderPort = [[self.oscSenderPortField stringValue] UTF8String];
    
    self.config->setOSCHost(senderHost);
    self.config->setOSCPort(senderPort);
    self.config->saveToFile("config.json");
}

- (IBAction)startStopButtonClicked:(id)sender {
    if (self.isRunning) {
        [self stopConversion];
    } else {
        [self startConversion];
    }
}

- (void)startConversion {
    [self saveConfiguration];
    
    std::string host = [[self.oscSenderHostField stringValue] UTF8String];
    std::string port = [[self.oscSenderPortField stringValue] UTF8String];
    
    if (host.empty() || port.empty()) {
        [self showAlert:@"Error" message:@"Please enter OSC host and port"];
        return;
    }
    
    try {
        // Initialize components
        self.cvReader = new CVReader();
        self.oscSender = new OSCSender(host, port);
        
        // Update UI
        self.isRunning = YES;
        [self.startStopButton setTitle:@"Stop Conversion"];
        [self.progressIndicator startAnimation:nil];
        [self.statusLabel setStringValue:[NSString stringWithFormat:@"Converting CV to OSC...\nTarget: %s:%s\nPress 'Stop Conversion' to stop.", host.c_str(), port.c_str()]];
        
        // Start worker thread
        self.workerRunning = YES;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            [self workerLoop];
        });
        
    } catch (const std::exception& e) {
        [self showAlert:@"Error" message:[NSString stringWithFormat:@"Failed to start conversion: %s", e.what()]];
        self.isRunning = NO;
    }
}

- (void)stopConversion {
    self.isRunning = NO;
    self.workerRunning = NO;
    
    // Give worker thread time to finish
    usleep(100000); // 100ms
    
    // Clean up
    delete self.cvReader;
    delete self.oscSender;
    self.cvReader = nullptr;
    self.oscSender = nullptr;
    
    // Update UI
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.startStopButton setTitle:@"Start Conversion"];
        [self.progressIndicator stopAnimation:nil];
        [self.statusLabel setStringValue:@"Conversion stopped.\n\nReady to start again."];
    });
}

- (void)workerLoop {
    std::vector<float> cvBuffer;
    int messageCount = 0;
    auto lastUpdate = std::chrono::steady_clock::now();
    
    while (self.workerRunning) {
        try {
            auto now = std::chrono::steady_clock::now();
            
            // Read CV values
            if (self.cvReader) {
                self.cvReader->readChannels(cvBuffer);
                
                // Send OSC messages
                if (self.oscSender && !cvBuffer.empty()) {
                    std::vector<std::string> addresses;
                    std::vector<float> values;
                    
                    for (size_t i = 0; i < cvBuffer.size(); ++i) {
                        addresses.push_back("/cv/channel/" + std::to_string(i + 1));
                        // Normalize CV value (assuming -10V to +10V range)
                        float normalized = (cvBuffer[i] + 10.0f) / 20.0f;
                        normalized = std::clamp(normalized, 0.0f, 1.0f);
                        values.push_back(normalized);
                    }
                    
                    if (self.oscSender->sendFloatBatch(addresses, values)) {
                        messageCount += addresses.size();
                    }
                }
            }
            
            // Update status every second
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
            if (elapsed.count() >= 1) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self.statusLabel setStringValue:[NSString stringWithFormat:@"Converting CV to OSC...\nChannels: %zu, Messages/sec: %d\nTarget: %s:%s", 
                                                     cvBuffer.size(), messageCount,
                                                     [[self.oscSenderHostField stringValue] UTF8String],
                                                     [[self.oscSenderPortField stringValue] UTF8String]]];
                });
                messageCount = 0;
                lastUpdate = now;
            }
            
            // Sleep for update interval
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 Hz update rate
            
        } catch (const std::exception& e) {
            NSLog(@"Error in worker loop: %s", e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

- (void)showAlert:(NSString *)title message:(NSString *)message {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:title];
    [alert setInformativeText:message];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (self.isRunning) {
        [self stopConversion];
    }
    return NSTerminateNow;
}

// New action methods for advanced functionality
- (IBAction)toggleOSCReceiver:(id)sender {
    self.oscReceiverRunning = !self.oscReceiverRunning;
    NSButton *button = (NSButton *)sender;
    
    if (self.oscReceiverRunning) {
        [button setTitle:@"Stop"];
        // Initialize OSC Receiver
        if (!self.oscReceiver) {
            self.oscReceiver = new OSCReceiver("8001");
        }
        // Start OSC receiver logic here
        NSLog(@"OSC Receiver started");
    } else {
        [button setTitle:@"Start"];
        // Stop OSC receiver logic here
        NSLog(@"OSC Receiver stopped");
    }
}

- (IBAction)toggleOSCSender:(id)sender {
// (Unused) NSButton *button = (NSButton *)sender; // Suppress unused variable warning
    NSLog(@"OSC Sender toggled");
}

- (IBAction)toggleCVReceiver:(id)sender {
// (Unused) NSButton *button = (NSButton *)sender; // Suppress unused variable warning
    NSLog(@"CV Receiver toggled");
}

- (IBAction)toggleCVSender:(id)sender {
    self.cvSenderRunning = !self.cvSenderRunning;
    NSButton *button = (NSButton *)sender;
    
    if (self.cvSenderRunning) {
        [button setTitle:@"Stop"];
        // Initialize CV Writer
        if (!self.cvWriter) {
            self.cvWriter = new CVWriter();
        }
        NSLog(@"CV Sender started");
    } else {
        [button setTitle:@"Start"];
        NSLog(@"CV Sender stopped");
    }
}

- (IBAction)showAudioDeviceSelection:(id)sender {
    // Create audio device selection window
    NSWindow *deviceWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 300)
                                                          styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
    [deviceWindow setTitle:@"Audio Device Selection"];
    [deviceWindow center];
    
    NSView *contentView = [[NSView alloc] initWithFrame:deviceWindow.contentView.frame];
    [deviceWindow setContentView:contentView];
    
    // Create device list
    NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 250, 360, 20)];
    [label setStringValue:@"Select Audio Input Device:"];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];
    [label setEditable:NO];
    [contentView addSubview:label];
    
    // Mock device list (real implementation would query AudioDeviceManager)
    NSArray *devices = @[@"Built-in Microphone", @"USB Audio Interface", @"Scarlett 2i2", @"Focusrite Interface"];
    
    NSPopUpButton *devicePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(20, 220, 360, 25)];
    [devicePopup addItemsWithTitles:devices];
    [contentView addSubview:devicePopup];
    
    // Channel count info
    NSTextField *channelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 180, 360, 40)];
    [channelLabel setStringValue:@"Channels available: 8\nSample Rate: 44100 Hz"];
    [channelLabel setBezeled:NO];
    [channelLabel setDrawsBackground:NO];
    [channelLabel setEditable:NO];
    [contentView addSubview:channelLabel];
    
    // Buttons
    NSButton *okButton = [[NSButton alloc] initWithFrame:NSMakeRect(300, 20, 80, 30)];
    [okButton setTitle:@"OK"];
    [okButton setTarget:deviceWindow];
    [okButton setAction:@selector(close)];
    [contentView addSubview:okButton];
    
    [deviceWindow makeKeyAndOrderFront:nil];
}

- (IBAction)showChannelConfiguration:(id)sender {
    // Create channel configuration window
    NSWindow *channelWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                                          styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                                            backing:NSBackingStoreBuffered
                                                              defer:NO];
    [channelWindow setTitle:@"Channel Configuration"];
    [channelWindow center];
    
    NSView *contentView = [[NSView alloc] initWithFrame:channelWindow.contentView.frame];
    [channelWindow setContentView:contentView];
    
    // Header
    NSTextField *headerLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 350, 460, 20)];
    [headerLabel setStringValue:@"Configure CV Channel Ranges and Settings"];
    [headerLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [headerLabel setBezeled:NO];
    [headerLabel setDrawsBackground:NO];
    [headerLabel setEditable:NO];
    [contentView addSubview:headerLabel];
    
    // Create channel controls for 8 channels
    for (int i = 0; i < 8; i++) {
        CGFloat y = 320 - (i * 35);
        
        // Channel label
        NSTextField *channelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, y, 80, 20)];
        [channelLabel setStringValue:[NSString stringWithFormat:@"Channel %d:", i + 1]];
        [channelLabel setBezeled:NO];
        [channelLabel setDrawsBackground:NO];
        [channelLabel setEditable:NO];
        [contentView addSubview:channelLabel];
        
        // Enable checkbox
        NSButton *enableButton = [[NSButton alloc] initWithFrame:NSMakeRect(100, y, 60, 20)];
        [enableButton setButtonType:NSButtonTypeSwitch];
        [enableButton setTitle:@"Enable"];
        [enableButton setState:NSControlStateValueOn];
        [contentView addSubview:enableButton];
        
        // Min voltage
        NSTextField *minLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(170, y, 30, 20)];
        [minLabel setStringValue:@"Min:"];
        [minLabel setBezeled:NO];
        [minLabel setDrawsBackground:NO];
        [minLabel setEditable:NO];
        [contentView addSubview:minLabel];
        
        NSTextField *minField = [[NSTextField alloc] initWithFrame:NSMakeRect(200, y, 50, 20)];
        [minField setStringValue:@"-10.0"];
        [contentView addSubview:minField];
        
        // Max voltage
        NSTextField *maxLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(260, y, 30, 20)];
        [maxLabel setStringValue:@"Max:"];
        [maxLabel setBezeled:NO];
        [maxLabel setDrawsBackground:NO];
        [maxLabel setEditable:NO];
        [contentView addSubview:maxLabel];
        
        NSTextField *maxField = [[NSTextField alloc] initWithFrame:NSMakeRect(290, y, 50, 20)];
        [maxField setStringValue:@"+10.0"];
        [contentView addSubview:maxField];
        
        // OSC Address
        NSTextField *oscLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(350, y, 60, 20)];
        [oscLabel setStringValue:@"OSC Addr:"];
        [oscLabel setBezeled:NO];
        [oscLabel setDrawsBackground:NO];
        [oscLabel setEditable:NO];
        [contentView addSubview:oscLabel];
        
        NSTextField *oscField = [[NSTextField alloc] initWithFrame:NSMakeRect(410, y, 70, 20)];
        [oscField setStringValue:[NSString stringWithFormat:@"/cv/ch/%d", i + 1]];
        [contentView addSubview:oscField];
    }
    
    // Apply and Cancel buttons
    NSButton *applyButton = [[NSButton alloc] initWithFrame:NSMakeRect(320, 20, 80, 30)];
    [applyButton setTitle:@"Apply"];
    [applyButton setTarget:channelWindow];
    [applyButton setAction:@selector(close)];
    [contentView addSubview:applyButton];
    
    NSButton *cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(410, 20, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setTarget:channelWindow];
    [cancelButton setAction:@selector(close)];
    [contentView addSubview:cancelButton];
    
    [channelWindow makeKeyAndOrderFront:nil];
}

- (IBAction)showPerformanceMonitor:(id)sender {
    [self showAlert:@"Performance Monitor" 
            message:@"Performance monitoring window will be implemented here."];
}

- (IBAction)showMIDIControl:(id)sender {
    [self showAlert:@"MIDI Control" 
            message:@"MIDI control configuration will be implemented here."];
}

- (IBAction)showExternalDevices:(id)sender {
    [self showAlert:@"External Devices" 
            message:@"External device management will be implemented here."];
}

- (IBAction)showAdvancedSettings:(id)sender {
    [self showAlert:@"Advanced Settings" 
            message:@"Advanced settings configuration will be implemented here."];
}

- (IBAction)showCalibration:(id)sender {
    [self showAlert:@"Calibration" 
            message:@"CV calibration tools will be implemented here."];
}

- (IBAction)showProfessionalMixer:(id)sender {
    if (!self.professionalMixerWindow) {
        // Create mixer engine
        auto mixerEngine = std::make_shared<OSCMixerEngine>(8); // 8 channels
        
        // Create and show Professional Mixer window
        self.professionalMixerWindow = [[ProfessionalMixerWindow alloc] initWithEngine:mixerEngine];
        [self.professionalMixerWindow showWindow:nil];
        
        // Update status
        [self.statusLabel setStringValue:@"Professional OSC Mixer launched.\n\nCheck the mixer window for controls."];
    } else {
        // Bring existing window to front
        [self.professionalMixerWindow.window makeKeyAndOrderFront:nil];
    }
}
- (void)dealloc
{
    [super dealloc];
    if (self.isRunning) {
        [self stopConversion];
    }
    delete self.config;
    delete self.cvReader;
    delete self.cvWriter;
    delete self.oscSender;
    delete self.oscReceiver;
    delete self.performanceMonitor;
    delete self.externalDevices;
    delete self.calibrator;
    delete self.audioManager;
}

@end

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        CVToOSCAppDelegate *delegate = [[CVToOSCAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
