#import "ProfessionalMixerWindow.h"
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CALayer.h>
#import <objc/runtime.h>
#import "../platform/MacOSPermissions.h"

@interface ProfessionalMixerWindow () <NSTableViewDataSource, NSTableViewDelegate>
@property (strong) NSTimer *meterUpdateTimer;
@property (strong) NSMutableArray<NSDictionary*> *inputDevices;
@property (strong) NSMutableArray<NSDictionary*> *outputDevices;
@property (strong) NSPopover *currentPopover;
@property (nonatomic) AudioDeviceManager *audioDeviceManager;
@end

@implementation ProfessionalMixerWindow

- (std::shared_ptr<OSCMixerEngine>)mixerEngine {
    return _mixerEngine;
}

- (instancetype)initWithEngine:(std::shared_ptr<OSCMixerEngine>)engine {
    // Create compact window with adaptive mixer layout
    NSRect windowFrame = NSMakeRect(0, 0, 900, 650);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:windowFrame
                                                   styleMask:(NSWindowStyleMaskTitled | 
                                                            NSWindowStyleMaskClosable | 
                                                            NSWindowStyleMaskMiniaturizable | 
                                                            NSWindowStyleMaskResizable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    
    self = [super initWithWindow:window];
    if (self) {
        _mixerEngine = engine;
        self.inputDevices = [[NSMutableArray alloc] init];
        self.outputDevices = [[NSMutableArray alloc] init];
        self.channelStrips = [[NSMutableArray alloc] init];
        
        // Initialize audio device manager
        self.audioDeviceManager = new AudioDeviceManager();
        if (!self.audioDeviceManager->initialize()) {
            NSLog(@"Warning: Failed to initialize audio device manager");
        }
        
        [self.window setTitle:@"Professional OSC Mixer"];
        [self.window center];
        [self.window setMinSize:NSMakeSize(700, 500)];
        
        [self setupMainInterface];
        [self setupMenuAndToolbar];
        
        // Set dark appearance for professional look
        if (@available(macOS 10.14, *)) {
            [self.window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        }
        
        // Start meter updates automatically when window is created
        [self startMeterUpdates];
        NSLog(@"✅ Professional OSC Mixer window initialized with meter updates started");
    }
    return self;
}

- (void)setupMainInterface {
    NSView *contentView = self.window.contentView;
    
    // Main container with dark background
    self.mainContentView = [[NSView alloc] initWithFrame:contentView.bounds];
    [self.mainContentView setWantsLayer:YES];
    self.mainContentView.layer.backgroundColor = [NSColor colorWithRed:0.15 green:0.15 blue:0.15 alpha:1.0].CGColor;
    [contentView addSubview:self.mainContentView];
    
    // Setup auto-layout
    self.mainContentView.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [self.mainContentView.topAnchor constraintEqualToAnchor:contentView.topAnchor],
        [self.mainContentView.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor],
        [self.mainContentView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
        [self.mainContentView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor]
    ]];
    
    [self setupChannelStrips];
    [self setupMasterSection];
    [self setupDeviceSection];
}

- (void)setupChannelStrips {
    // Channel strips container with horizontal stack
    NSStackView *channelStackView = [[NSStackView alloc] init];
    channelStackView.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    channelStackView.distribution = NSStackViewDistributionFillEqually;
    channelStackView.spacing = 8;
    channelStackView.translatesAutoresizingMaskIntoConstraints = NO;
    
    [channelStackView setWantsLayer:YES];
    channelStackView.layer.backgroundColor = [NSColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0].CGColor;
    channelStackView.layer.cornerRadius = 8;
    
    [self.mainContentView addSubview:channelStackView];
    self.channelStripContainer = channelStackView;
    
    [NSLayoutConstraint activateConstraints:@[
        [channelStackView.topAnchor constraintEqualToAnchor:self.mainContentView.topAnchor constant:40],  // Reduced from 80
        [channelStackView.bottomAnchor constraintEqualToAnchor:self.mainContentView.bottomAnchor constant:-80], // Reduced from 200
        [channelStackView.leadingAnchor constraintEqualToAnchor:self.mainContentView.leadingAnchor constant:20],
        [channelStackView.trailingAnchor constraintEqualToAnchor:self.mainContentView.trailingAnchor constant:-250]
    ]];
    
    // Create 8 channel strips with adaptive layout
    for (int i = 0; i < 8; i++) {
        [self createChannelStrip:i atPosition:NSZeroPoint];
    }
}

- (void)createChannelStrip:(int)channelIndex atPosition:(NSPoint)position {
    // Channel strip with adaptive Auto Layout
    NSView *channelStrip = [[NSView alloc] init];
    [channelStrip setWantsLayer:YES];
    channelStrip.layer.backgroundColor = [NSColor colorWithRed:0.28 green:0.28 blue:0.28 alpha:1.0].CGColor;
    channelStrip.layer.cornerRadius = 6;
    channelStrip.layer.borderWidth = 1;
    channelStrip.layer.borderColor = [NSColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0].CGColor;
    channelStrip.translatesAutoresizingMaskIntoConstraints = NO;
    
    // Channel label
    NSTextField *channelLabel = [[NSTextField alloc] init];
    [channelLabel setStringValue:[NSString stringWithFormat:@"CH %d", channelIndex + 1]];
    [channelLabel setFont:[NSFont systemFontOfSize:11 weight:NSFontWeightMedium]];
    [channelLabel setTextColor:[NSColor lightGrayColor]];
    [channelLabel setBezeled:NO];
    [channelLabel setDrawsBackground:NO];
    [channelLabel setEditable:NO];
    [channelLabel setAlignment:NSTextAlignmentCenter];
    channelLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [channelStrip addSubview:channelLabel];
    
    // Main device buttons - prominent with device selection
    NSButton *inputButton = [[NSButton alloc] init];
    [inputButton setTitle:@"INPUT"];
    [inputButton setFont:[NSFont systemFontOfSize:10 weight:NSFontWeightMedium]];
    [inputButton setTarget:self];
    [inputButton setAction:@selector(selectInputDevice:)];
    [inputButton setTag:channelIndex];
    [inputButton setWantsLayer:YES];
    [inputButton setBordered:NO];
    inputButton.layer.backgroundColor = [NSColor colorWithRed:0.2 green:0.7 blue:0.2 alpha:1.0].CGColor;
    inputButton.layer.cornerRadius = 3.0;
    inputButton.translatesAutoresizingMaskIntoConstraints = NO;
    [inputButton setEnabled:YES]; // Always enabled
    [channelStrip addSubview:inputButton];
    
    NSButton *outputButton = [[NSButton alloc] init];
    [outputButton setTitle:@"OUTPUT"];
    [outputButton setFont:[NSFont systemFontOfSize:10 weight:NSFontWeightMedium]];
    [outputButton setTarget:self];
    [outputButton setAction:@selector(selectOutputDevice:)];
    [outputButton setTag:channelIndex];
    [outputButton setWantsLayer:YES];
    [outputButton setBordered:NO];
    outputButton.layer.backgroundColor = [NSColor colorWithRed:0.2 green:0.2 blue:0.7 alpha:1.0].CGColor;
    outputButton.layer.cornerRadius = 3.0;
    outputButton.translatesAutoresizingMaskIntoConstraints = NO;
    [outputButton setEnabled:YES]; // Always enabled
    [channelStrip addSubview:outputButton];
    
    // Settings button for detailed OSC configuration
    NSButton *settingsButton = [[NSButton alloc] init];
    [settingsButton setTitle:@"⚙️"];
    [settingsButton setFont:[NSFont systemFontOfSize:12]];
    [settingsButton setTarget:self];
    [settingsButton setAction:@selector(showChannelSettings:)];
    [settingsButton setTag:channelIndex];
    [settingsButton setWantsLayer:YES];
    [settingsButton setBordered:NO];
    settingsButton.layer.backgroundColor = [NSColor colorWithWhite:0.35 alpha:1.0].CGColor;
    settingsButton.layer.cornerRadius = 3.0;
    settingsButton.translatesAutoresizingMaskIntoConstraints = NO;
    [channelStrip addSubview:settingsButton];
    
    // Simplified: Only keep essential Mute button
    NSButton *muteButton = [[NSButton alloc] init];
    [muteButton setTitle:@"MUTE"];
    [muteButton setButtonType:NSButtonTypePushOnPushOff];
    [muteButton setFont:[NSFont systemFontOfSize:9 weight:NSFontWeightMedium]];
    [muteButton setTarget:self];
    [muteButton setAction:@selector(channelMutePressed:)];
    [muteButton setTag:channelIndex];
    [muteButton setWantsLayer:YES];
    [muteButton setBordered:NO];
    muteButton.layer.backgroundColor = [NSColor colorWithRed:1.0 green:0.3 blue:0.3 alpha:0.3].CGColor;
    muteButton.layer.cornerRadius = 3.0;
    muteButton.translatesAutoresizingMaskIntoConstraints = NO;
    [channelStrip addSubview:muteButton];
    
    
    // Auto Layout constraints - redesigned with correct priorities
    [NSLayoutConstraint activateConstraints:@[
        // Channel strip size
        [channelStrip.widthAnchor constraintEqualToConstant:90],
        
        // Channel label at top
        [channelLabel.topAnchor constraintEqualToAnchor:channelStrip.topAnchor constant:8],
        [channelLabel.leadingAnchor constraintEqualToAnchor:channelStrip.leadingAnchor constant:4],
        [channelLabel.trailingAnchor constraintEqualToAnchor:channelStrip.trailingAnchor constant:-4],
        [channelLabel.heightAnchor constraintEqualToConstant:20],
        
        // Main INPUT button - prominent
        [inputButton.topAnchor constraintEqualToAnchor:channelLabel.bottomAnchor constant:8],
        [inputButton.leadingAnchor constraintEqualToAnchor:channelStrip.leadingAnchor constant:6],
        [inputButton.trailingAnchor constraintEqualToAnchor:channelStrip.trailingAnchor constant:-6],
        [inputButton.heightAnchor constraintEqualToConstant:24],
        
        // Main OUTPUT button - prominent
        [outputButton.topAnchor constraintEqualToAnchor:inputButton.bottomAnchor constant:4],
        [outputButton.leadingAnchor constraintEqualToAnchor:channelStrip.leadingAnchor constant:6],
        [outputButton.trailingAnchor constraintEqualToAnchor:settingsButton.leadingAnchor constant:-4],
        [outputButton.heightAnchor constraintEqualToConstant:24],
        
        // Settings button - compact gear icon
        [settingsButton.topAnchor constraintEqualToAnchor:inputButton.bottomAnchor constant:4],
        [settingsButton.trailingAnchor constraintEqualToAnchor:channelStrip.trailingAnchor constant:-6],
        [settingsButton.widthAnchor constraintEqualToConstant:24],
        [settingsButton.heightAnchor constraintEqualToConstant:24],
        
        // Mute button at bottom - simple layout
        [muteButton.topAnchor constraintEqualToAnchor:outputButton.bottomAnchor constant:12],
        [muteButton.leadingAnchor constraintEqualToAnchor:channelStrip.leadingAnchor constant:6],
        [muteButton.trailingAnchor constraintEqualToAnchor:channelStrip.trailingAnchor constant:-6],
        [muteButton.heightAnchor constraintEqualToConstant:24],
        [muteButton.bottomAnchor constraintEqualToAnchor:channelStrip.bottomAnchor constant:-8]
    ]];
    
    // Add to stack view instead of as subview
    [(NSStackView*)self.channelStripContainer addArrangedSubview:channelStrip];
    [self.channelStrips addObject:channelStrip];
}

- (void)setupMasterSection {
    // Master section with same style as channels
    self.masterSection = [[NSView alloc] init];
    [self.masterSection setWantsLayer:YES];
    self.masterSection.layer.backgroundColor = [NSColor colorWithRed:0.28 green:0.28 blue:0.28 alpha:1.0].CGColor;
    self.masterSection.layer.cornerRadius = 6;
    self.masterSection.layer.borderWidth = 1;
    self.masterSection.layer.borderColor = [NSColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0].CGColor;
    [self.mainContentView addSubview:self.masterSection];
    
    self.masterSection.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [self.masterSection.topAnchor constraintEqualToAnchor:self.mainContentView.topAnchor constant:40],  // Reduced from 80
        [self.masterSection.bottomAnchor constraintEqualToAnchor:self.mainContentView.bottomAnchor constant:-80], // Reduced from 200
        [self.masterSection.trailingAnchor constraintEqualToAnchor:self.mainContentView.trailingAnchor constant:-20],
        [self.masterSection.widthAnchor constraintEqualToConstant:120]
    ]];
    
    // Master label
    NSTextField *masterLabel = [[NSTextField alloc] init];
    [masterLabel setStringValue:@"MASTER"];
    [masterLabel setFont:[NSFont systemFontOfSize:11 weight:NSFontWeightMedium]];
    [masterLabel setTextColor:[NSColor lightGrayColor]];
    [masterLabel setBezeled:NO];
    [masterLabel setDrawsBackground:NO];
    [masterLabel setEditable:NO];
    [masterLabel setAlignment:NSTextAlignmentCenter];
    [self.masterSection addSubview:masterLabel];
    
    masterLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [masterLabel.topAnchor constraintEqualToAnchor:self.masterSection.topAnchor constant:8],
        [masterLabel.leadingAnchor constraintEqualToAnchor:self.masterSection.leadingAnchor constant:4],
        [masterLabel.trailingAnchor constraintEqualToAnchor:self.masterSection.trailingAnchor constant:-4],
        [masterLabel.heightAnchor constraintEqualToConstant:20]
    ]];
    
    // Simplified master section - only essential start/stop control
    NSButton *startStopButton = [[NSButton alloc] init];
    [startStopButton setTitle:@"START"];
    [startStopButton setButtonType:NSButtonTypePushOnPushOff];
    [startStopButton setFont:[NSFont systemFontOfSize:11 weight:NSFontWeightMedium]];
    [startStopButton setTarget:self];
    [startStopButton setAction:@selector(toggleMixer:)];
    [startStopButton setWantsLayer:YES];
    [startStopButton setBordered:NO];
    startStopButton.layer.backgroundColor = [NSColor colorWithRed:0.2 green:0.7 blue:0.2 alpha:1.0].CGColor;
    startStopButton.layer.cornerRadius = 4.0;
    startStopButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.masterSection addSubview:startStopButton];
    
    // Store reference to start/stop button
    self.startStopButton = startStopButton;
    
    // Layout constraints - simplified master controls
    [NSLayoutConstraint activateConstraints:@[
        // Start/Stop button positioned below master label
        [startStopButton.topAnchor constraintEqualToAnchor:masterLabel.bottomAnchor constant:20],
        [startStopButton.leadingAnchor constraintEqualToAnchor:self.masterSection.leadingAnchor constant:6],
        [startStopButton.trailingAnchor constraintEqualToAnchor:self.masterSection.trailingAnchor constant:-6],
        [startStopButton.heightAnchor constraintEqualToConstant:32]
    ]];
}

- (void)setupDeviceSection {
    // Simplified status bar at the bottom - more compact
    self.deviceSection = [[NSView alloc] init];
    [self.deviceSection setWantsLayer:YES];
    self.deviceSection.layer.backgroundColor = [NSColor colorWithRed:0.12 green:0.12 blue:0.12 alpha:1.0].CGColor;
    [self.mainContentView addSubview:self.deviceSection];
    
    self.deviceSection.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [self.deviceSection.bottomAnchor constraintEqualToAnchor:self.mainContentView.bottomAnchor],
        [self.deviceSection.leadingAnchor constraintEqualToAnchor:self.mainContentView.leadingAnchor],
        [self.deviceSection.trailingAnchor constraintEqualToAnchor:self.mainContentView.trailingAnchor],
        [self.deviceSection.heightAnchor constraintEqualToConstant:60]  // Reduced from 180 to 60
    ]];
    
    // Compact status label
    self.statusLabel = [[NSTextField alloc] init];
    [self.statusLabel setStringValue:@"OSC Mixer - Ready"];
    [self.statusLabel setFont:[NSFont systemFontOfSize:11]];
    [self.statusLabel setTextColor:[NSColor lightGrayColor]];
    [self.statusLabel setBezeled:NO];
    [self.statusLabel setDrawsBackground:NO];
    [self.statusLabel setEditable:NO];
    [self.deviceSection addSubview:self.statusLabel];
    
    self.statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [self.statusLabel.centerYAnchor constraintEqualToAnchor:self.deviceSection.centerYAnchor],
        [self.statusLabel.leadingAnchor constraintEqualToAnchor:self.deviceSection.leadingAnchor constant:20]
    ]];
}

- (void)setupMenuAndToolbar {
    // Simplified menu - only essential items
    self.mixerMenu = [[NSMenu alloc] initWithTitle:@"OSC Mixer"];
    
    NSMenuItem *loadConfigItem = [[NSMenuItem alloc] initWithTitle:@"Load Configuration..." action:@selector(loadMixerConfiguration:) keyEquivalent:@"o"];
    [loadConfigItem setTarget:self];
    [self.mixerMenu addItem:loadConfigItem];
    
    NSMenuItem *saveConfigItem = [[NSMenuItem alloc] initWithTitle:@"Save Configuration..." action:@selector(saveMixerConfiguration:) keyEquivalent:@"s"];
    [saveConfigItem setTarget:self];
    [self.mixerMenu addItem:saveConfigItem];
}

#pragma mark - Actions

- (IBAction)toggleMixer:(id)sender {
    // Check if mixer engine is available
    if (!self.mixerEngine) {
        [self.statusLabel setStringValue:@"Error: Mixer engine not initialized"];
        [self.startStopButton setTitle:@"START"];
        NSLog(@"Error: Mixer engine is null in toggleMixer");
        return;
    }
    
    if (self.mixerEngine->isRunning()) {
        // Stop the mixer
        self.mixerEngine->stop();
        [self.startStopButton setTitle:@"START"];
        [self.statusLabel setStringValue:@"Mixer stopped"];
        [self stopMeterUpdates];
        NSLog(@"Mixer stopped successfully");
    } else {
        // Start the mixer
        bool startSuccess = self.mixerEngine->start();
        if (startSuccess) {
            [self.startStopButton setTitle:@"STOP"];
            [self.statusLabel setStringValue:@"Mixer running"];
            [self startMeterUpdates];
            NSLog(@"Mixer started successfully");
        } else {
            [self.statusLabel setStringValue:@"Error: Failed to start mixer - check audio devices"];
            [self.startStopButton setTitle:@"START"]; // Ensure button is in correct state
            NSLog(@"Error: Failed to start mixer engine");
            
            // Show more detailed error dialog
            NSAlert *alert = [[NSAlert alloc] init];
            alert.messageText = @"Failed to Start Mixer";
            alert.informativeText = @"Could not start the mixer engine. Please check that audio devices are properly connected and configured, and that no other application is using the audio system.";
            alert.alertStyle = NSAlertStyleWarning;
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
        }
    }
}

- (IBAction)toggleSoloMix:(id)sender {
    if (self.mixerEngine) {
        bool isSolo = self.mixerEngine->isSoloMode();
        self.mixerEngine->setSoloMode(!isSolo);
        [self.soloMixButton setTitle:isSolo ? @"MIX" : @"SOLO"];
    }
}

- (IBAction)masterVolumeChanged:(id)sender {
    if (self.mixerEngine) {
        float volume = [self.masterVolumeSlider floatValue];
        self.mixerEngine->setMasterVolume(volume);
        NSLog(@"🎵 Master volume changed to %.2f", volume);
    }
}

- (IBAction)masterMutePressed:(id)sender {
    NSButton *button = (NSButton *)sender;
    BOOL isMuted = [button state] == NSControlStateValueOn;
    
    if (self.mixerEngine) {
        self.mixerEngine->setMasterMute(isMuted);
        
        // Update button appearance
        [button setWantsLayer:YES];
        if (isMuted) {
            button.layer.backgroundColor = [NSColor colorWithRed:1.0 green:0.3 blue:0.3 alpha:1.0].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 2.0;
            [button setTitle:@"MUTED"];
            NSLog(@"🔇 Master mute clicked - MUTED");
        } else {
            button.layer.backgroundColor = [NSColor colorWithRed:1.0 green:0.3 blue:0.3 alpha:0.3].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 1.5;
            [button setTitle:@"MUTE"];
            NSLog(@"🔇 Master mute clicked - UNMUTED");
        }
    }
}

- (void)channelLevelChanged:(id)sender {
    NSSlider *slider = (NSSlider *)sender;
    int channelIndex = (int)[slider tag];
    float level = [slider floatValue];
    
    NSLog(@"Channel %d level changed to: %.2f", channelIndex, level);
    
    if (self.mixerEngine) {
        // Convert slider value (0-1) to voltage range (-10V to +10V)
        float voltage = (level * 20.0f) - 10.0f;
        self.mixerEngine->setChannelLevel(channelIndex, voltage);
        NSLog(@"Set channel %d voltage to: %.2fV", channelIndex, voltage);
    }
}

- (void)channelSoloPressed:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    BOOL isSolo = [button state] == NSControlStateValueOn;
    
    NSLog(@"Channel %d solo pressed: %s", channelIndex, isSolo ? "ON" : "OFF");
    
    if (self.mixerEngine) {
        self.mixerEngine->setChannelSolo(channelIndex, isSolo);
        
        // Update button appearance with professional styling
        [button setWantsLayer:YES];
        if (isSolo) {
            button.layer.backgroundColor = [NSColor colorWithRed:1.0 green:0.9 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.7 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 2.0;
            [button setTitle:@"S"];
        } else {
            button.layer.backgroundColor = [NSColor colorWithRed:0.9 green:0.9 blue:0.1 alpha:0.3].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.8 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 1.5;
            [button setTitle:@"S"];
        }
    }
}

- (void)channelRecordPressed:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    BOOL isRecording = [button state] == NSControlStateValueOn;
    
    NSLog(@"Channel %d record pressed: %s", channelIndex, isRecording ? "ON" : "OFF");
    
    if (self.mixerEngine) {
        // Toggle recording feature here for channelIndex
        
        // Update button appearance with professional styling
        [button setWantsLayer:YES];
        if (isRecording) {
            button.layer.backgroundColor = [NSColor colorWithRed:0.95 green:0.1 blue:0.1 alpha:1.0].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.7 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 2.0;
            [button setTitle:@"R"];
        } else {
            button.layer.backgroundColor = [NSColor colorWithRed:0.8 green:0.0 blue:0.0 alpha:0.3].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.7 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 1.5;
            [button setTitle:@"R"];
        }
    }
}

- (void)channelLearnPressed:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    BOOL isLearning = [button state] == NSControlStateValueOn;
    
    NSLog(@"Channel %d learn pressed: %s", channelIndex, isLearning ? "ON" : "OFF");
    
    if (self.mixerEngine) {
        // Toggle learning mode for channelIndex
        self.mixerEngine->enableLearningMode(isLearning);
        
        // Update button appearance with professional styling
        [button setWantsLayer:YES];
        if (isLearning) {
            button.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.9 blue:0.1 alpha:1.0].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.0 green:0.7 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 2.0;
            [button setTitle:@"L"];
        } else {
            button.layer.backgroundColor = [NSColor colorWithRed:0.1 green:0.8 blue:0.1 alpha:0.3].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.0 green:0.7 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 1.5;
            [button setTitle:@"L"];
        }
    }
}

- (void)channelMutePressed:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    BOOL isMuted = [button state] == NSControlStateValueOn;
    
    NSLog(@"Channel %d mute pressed: %s", channelIndex, isMuted ? "ON" : "OFF");
    
    if (self.mixerEngine) {
        self.mixerEngine->setChannelMute(channelIndex, isMuted);
        
        // Update button appearance with professional styling
        [button setWantsLayer:YES];
        if (isMuted) {
            button.layer.backgroundColor = [NSColor colorWithRed:1.0 green:0.2 blue:0.2 alpha:1.0].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 2.0;
            [button setTitle:@"M"];
        } else {
            button.layer.backgroundColor = [NSColor colorWithRed:0.9 green:0.1 blue:0.1 alpha:0.3].CGColor;
            button.layer.borderColor = [NSColor colorWithRed:0.8 green:0.0 blue:0.0 alpha:1.0].CGColor;
            button.layer.borderWidth = 1.5;
            [button setTitle:@"M"];
        }
    }
}

- (void)showInputDevices:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Show input devices for channel %d", channelIndex);
    
    // Show device configuration dialog for input devices
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.window];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            if (self.mixerEngine) {
                bool success = self.mixerEngine->addInputDevice(channelIndex, config);
                if (success) {
                    NSLog(@"Input device added successfully: %s", config.deviceName.c_str());
                    [self updateChannelStates];
                    [self refreshDeviceLists];
                    
                    // Update button appearance to show device is connected
                    [button setWantsLayer:YES];
                    button.layer.backgroundColor = [NSColor systemGreenColor].CGColor;
                    [button setTitle:@"Input ✓"];
                } else {
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Failed to Add Device";
                    alert.informativeText = @"Could not add the input device to the channel.";
                    [alert runModal];
                }
            }
        }
    }];
}

- (void)showOutputDevices:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Show output devices for channel %d", channelIndex);
    
    // Show device configuration dialog for output devices
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.window];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            if (self.mixerEngine) {
                bool success = self.mixerEngine->addOutputDevice(channelIndex, config);
                if (success) {
                    NSLog(@"Output device added successfully: %s", config.deviceName.c_str());
                    [self updateChannelStates];
                    [self refreshDeviceLists];
                    
                    // Update button appearance to show device is connected
                    [button setWantsLayer:YES];
                    button.layer.backgroundColor = [NSColor systemBlueColor].CGColor;
                    [button setTitle:@"OSC Out ✓"];
                } else {
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Failed to Add Output Device";
                    alert.informativeText = @"Could not add the output device to the channel. Maximum 8 devices per channel.";
                    [alert runModal];
                }
            }
        }
    }];
}

- (void)showChannelConfig:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Show channel configuration for channel %d", channelIndex);
    
    // Show comprehensive channel configuration dialog
    ChannelConfigurationDialog *channelDialog = [[ChannelConfigurationDialog alloc] initWithParentWindow:self.window 
                                                                                                channelId:channelIndex 
                                                                                              mixerEngine:self.mixerEngine];
    [channelDialog showDialog:^(BOOL cancelled) {
        if (!cancelled) {
            NSLog(@"Channel %d configuration updated successfully", channelIndex);
            [self updateChannelStates];
            [self refreshDeviceLists];
            
            // Update config button appearance to show configuration is set
            [button setWantsLayer:YES];
            button.layer.backgroundColor = [NSColor systemOrangeColor].CGColor;
            [button setTitle:@"⚙️ Set"];
        }
    }];
}

- (IBAction)loadMixerConfiguration:(id)sender {
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setAllowedFileTypes:@[@"json"]];
    
    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            NSURL *url = [openPanel.URLs firstObject];
            NSString *path = [url path];
            
            if (self.mixerEngine && self.mixerEngine->loadConfiguration([path UTF8String])) {
                [self.statusLabel setStringValue:@"Configuration loaded successfully"];
                [self updateChannelStates];
            } else {
                [self.statusLabel setStringValue:@"Failed to load configuration"];
            }
        }
    }];
}

- (IBAction)saveMixerConfiguration:(id)sender {
    NSSavePanel *savePanel = [NSSavePanel savePanel];
    [savePanel setAllowedFileTypes:@[@"json"]];
    [savePanel setNameFieldStringValue:@"mixer_config.json"];
    
    [savePanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if (result == NSModalResponseOK) {
            NSURL *url = [savePanel URL];
            NSString *path = [url path];
            
            if (self.mixerEngine && self.mixerEngine->saveConfiguration([path UTF8String])) {
                [self.statusLabel setStringValue:@"Configuration saved successfully"];
            } else {
                [self.statusLabel setStringValue:@"Failed to save configuration"];
            }
        }
    }];
}

- (IBAction)showMixerSettings:(id)sender {
    NSLog(@"Show mixer settings dialog");
    
    GlobalSettingsDialog *settingsDialog = [[GlobalSettingsDialog alloc] initWithParentWindow:self.window 
                                                                                    mixerEngine:self.mixerEngine];
    [settingsDialog showDialog:^(BOOL cancelled) {
        if (!cancelled) {
            NSLog(@"Settings applied successfully");
            [self updateChannelStates];
        }
    }];
}

#pragma mark - Real-time Updates

- (void)startMeterUpdates {
    // Stop any existing timer first to prevent duplicates
    [self stopMeterUpdates];
    
    // Start new meter update timer
    self.meterUpdateTimer = [NSTimer scheduledTimerWithTimeInterval:0.05 // 20 FPS
                                                             target:self
                                                           selector:@selector(updateMetersTimer:)
                                                           userInfo:nil
                                                            repeats:YES];
    
    if (self.meterUpdateTimer) {
        NSLog(@"Meter updates started successfully");
    } else {
        NSLog(@"Error: Failed to start meter update timer");
    }
}

- (void)stopMeterUpdates {
    if (self.meterUpdateTimer) {
        [self.meterUpdateTimer invalidate];
        self.meterUpdateTimer = nil;
        NSLog(@"Meter updates stopped");
    }
    
    // Clear all meters immediately when stopping
    [self clearAllMeters];
}

- (void)clearAllMeters {
    // Clear all channel meters
    for (int i = 0; i < 8 && i < self.channelStrips.count; i++) {
        NSView *channelStrip = self.channelStrips[i];
        if (!channelStrip) continue;
        
        // Clear meter display
        for (NSView *subview in channelStrip.subviews) {
            if ([subview isKindOfClass:[NSView class]] && subview.layer.backgroundColor) {
                if (subview.frame.size.width <= 12 && subview.frame.size.height > 50) {
                    [subview setWantsLayer:YES];
                    CALayer *meterLayer = subview.layer;
                    meterLayer.sublayers = nil;
                    meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
                    break;
                }
            }
        }
        
        // Reset voltage labels to 0.0V
        for (NSView *subview in channelStrip.subviews) {
            if ([subview isKindOfClass:[NSTextField class]]) {
                NSTextField *textField = (NSTextField *)subview;
                if (textField.tag == i + 1000) {
                    [textField setStringValue:@"0.0V"];
                    [textField setTextColor:[NSColor lightGrayColor]];
                    break;
                }
            }
        }
    }
    
    // Clear master meter
    if (self.masterSection) {
        for (NSView *subview in self.masterSection.subviews) {
            if ([subview isKindOfClass:[NSView class]] && subview.frame.size.width <= 12 && subview.frame.size.height > 50) {
                [subview setWantsLayer:YES];
                CALayer *meterLayer = subview.layer;
                meterLayer.sublayers = nil;
                meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
            } else if ([subview isKindOfClass:[NSTextField class]]) {
                NSTextField *textField = (NSTextField *)subview;
                if ([textField.stringValue containsString:@"V"]) {
                    [textField setStringValue:@"0.0V"];
                    [textField setTextColor:[NSColor lightGrayColor]];
                }
            }
        }
    }
    
    NSLog(@"All meters cleared");
}

- (void)updateMetersTimer:(NSTimer*)timer {
    [self updateChannelMeters];
}

- (void)updateChannelMeters {
    if (!self.mixerEngine) {
        NSLog(@"Warning: updateChannelMeters called but mixer engine is null");
        return;
    }
    
    // Show meters for individual channels regardless of main mixer state
    static float smoothLevel[8] = {0}; // Store smooth levels for each channel
    static int debugCounter = 0;
    bool mixerIsRunning = self.mixerEngine->isRunning();
    
    // Count actually running channels
    int runningChannels = 0;
    auto* mixerState = self.mixerEngine->getMixerState();
    if (mixerState) {
        for (const auto& channel : mixerState->channels) {
            if (channel->state == ChannelState::RUNNING) {
                runningChannels++;
            }
        }
    }
    
    // Debug log every 2 seconds (40 timer calls at 20 FPS)
    if (debugCounter % 40 == 0) {
        NSLog(@"🔧 Mixer status: %s, Running channels: %d, Timer active: YES", 
              mixerIsRunning ? "RUNNING" : "STOPPED", runningChannels);
    }
    debugCounter++;
    
    @try {
        // Update meter displays for each channel
        for (int i = 0; i < 8 && i < self.channelStrips.count; i++) {
            NSView *channelStrip = self.channelStrips[i];
            if (!channelStrip) {
                NSLog(@"Warning: Channel strip %d is null", i);
                continue;
            }
            
            float inputLevel = 0.0f;
            
            // Calculate signal level for each individual channel if it's running
            bool channelIsRunning = false;
            if (mixerState && i < mixerState->channels.size()) {
                auto& channel = mixerState->channels[i];
                channelIsRunning = (channel->state == ChannelState::RUNNING);
                
                if (channelIsRunning) {
                    // Get channel signal level - demo signal should be active for running channels
                    float channelSignalLevel = channel->inputMeter.getCurrentLevel();
                    float channelOutputLevel = channel->outputMeter.getCurrentLevel();
                    
                    // Use the maximum of input and output levels for display
                    float rawLevel = fmax(fabs(channelSignalLevel), fabs(channelOutputLevel));
                    
                    // Also consider the fader position for visualization
                    float faderLevel = fabs(channel->levelVolts) / 10.0f; // -10V to +10V normalized
                    faderLevel = fmax(0.0f, fmin(faderLevel, 1.0f));
                    
                    // Combine actual signal with fader position for better visualization
                    float targetLevel = fmax(rawLevel / 10.0f, faderLevel * 0.3f); // Scale down raw signal
                    targetLevel = fmax(0.0f, fmin(targetLevel, 1.0f));
                    
                    // Smooth interpolation
                    smoothLevel[i] = smoothLevel[i] * 0.9f + targetLevel * 0.1f;
                    inputLevel = smoothLevel[i];
                    
                    // Debug log for first channel every 2 seconds
                    if (i == 0 && debugCounter % 40 == 0) {
                        NSLog(@"📊 Ch%d: signal=%.3f, output=%.3f, fader=%.3f, final=%.3f, running=%s", 
                              i, channelSignalLevel, channelOutputLevel, faderLevel, inputLevel,
                              channelIsRunning ? "YES" : "NO");
                    }
                } else {
                    // When channel is stopped, gradually fade out the meter
                    smoothLevel[i] *= 0.85f; // Fast decay when stopped
                    inputLevel = smoothLevel[i];
                }
            }
        
            // Find and update meter view with signal level
            for (NSView *subview in channelStrip.subviews) {
                if ([subview isKindOfClass:[NSView class]] && subview.layer.backgroundColor) {
                    // Check if this is a meter view by its properties
                    if (subview.frame.size.width <= 12 && subview.frame.size.height > 50) {
                        // This is likely the meter view
                        
                        // Update meter visualization with smooth gradient
                        [subview setWantsLayer:YES];
                        
                        // Create height-based meter effect
                        CALayer *meterLayer = subview.layer;
                        meterLayer.sublayers = nil; // Clear previous layers
                        
                        if (inputLevel > 0.02f) {
                            // Create gradient meter effect
                            CGFloat meterHeight = subview.frame.size.height * inputLevel;
                            
                            // Background (empty part)
                            meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
                            
                            // Active meter part - positioned at bottom, growing upward
CALayer *activeMeter = [CALayer layer];
CGFloat yPosition = 0; // Position at bottom
activeMeter.frame = CGRectMake(0, yPosition, subview.frame.size.width, meterHeight);
                            
                            // Color based on level
                            if (inputLevel < 0.6f) {
                                activeMeter.backgroundColor = [NSColor colorWithRed:0.0 green:0.8 blue:0.2 alpha:1.0].CGColor;
                            } else if (inputLevel < 0.85f) {
                                activeMeter.backgroundColor = [NSColor colorWithRed:0.9 green:0.9 blue:0.0 alpha:1.0].CGColor;
                            } else {
                                activeMeter.backgroundColor = [NSColor colorWithRed:1.0 green:0.3 blue:0.0 alpha:1.0].CGColor;
                            }
                            
                            [meterLayer addSublayer:activeMeter];
                        } else {
                            meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
                        }
                        break;
                    }
                }
            }
            
            // Update voltage label for this channel
            float voltage = 0.0f;
            if (channelIsRunning && mixerState && i < mixerState->channels.size()) {
                auto& channel = mixerState->channels[i];
                voltage = channel->levelVolts;
            }
            
            for (NSView *subview in channelStrip.subviews) {
                if ([subview isKindOfClass:[NSTextField class]]) {
                    NSTextField *textField = (NSTextField *)subview;
                    if (textField.tag == i + 1000) { // Voltage labels have tag offset
                        [textField setStringValue:[NSString stringWithFormat:@"%.1fV", voltage]];
                        
                        // Color code the voltage based on level
                        if (fabs(voltage) > 8.0f) {
                            [textField setTextColor:[NSColor systemRedColor]];
                        } else if (fabs(voltage) > 5.0f) {
                            [textField setTextColor:[NSColor systemYellowColor]];
                        } else {
                            [textField setTextColor:[NSColor lightGrayColor]];
                        }
                        break;
                    }
                }
            }
        }
        
            // Update master section meter and voltage
        if (self.masterSection) {
            static float masterSmoothLevel = 0.0f;
            float masterLevel = 0.0f;
            
            // Show master level based on running channels and master volume
            if (runningChannels > 0) {
                // Calculate master level based on ALL running channel outputs
                float totalSignalLevel = 0.0f;
                int channelsWithSignal = 0;
                
                auto* mixerState = self.mixerEngine->getMixerState();
                if (mixerState) {
                    for (size_t i = 0; i < mixerState->channels.size() && i < 8; i++) {
                        auto& channel = mixerState->channels[i];
                        if (channel->state == ChannelState::RUNNING) {
                            // Get actual signal levels from running channels
                            float channelOutput = fabs(channel->outputMeter.getCurrentLevel());
                            if (channelOutput > 0.01f) {
                                totalSignalLevel += channelOutput;
                                channelsWithSignal++;
                            }
                        }
                    }
                }
                
                // Calculate master output level
                if (channelsWithSignal > 0) {
                    // Average signal level from all active channels
                    float avgSignalLevel = totalSignalLevel / channelsWithSignal;
                    
                    // Apply master volume control to the signal
                    float masterVolume = self.masterVolumeSlider ? [self.masterVolumeSlider floatValue] : 1.0f;
                    masterLevel = (avgSignalLevel / 5.0f) * masterVolume; // Scale down from voltage range
                    masterLevel = fmax(0.0f, fmin(masterLevel, 1.0f)); // Clamp to 0-1
                } else {
                    // No signals, but show master volume setting visually
                    float masterVolume = self.masterVolumeSlider ? [self.masterVolumeSlider floatValue] : 1.0f;
                    masterLevel = masterVolume * 0.2f; // Show minimal level indicating master volume
                }
                
                masterSmoothLevel = masterSmoothLevel * 0.8f + masterLevel * 0.2f;
                masterLevel = masterSmoothLevel;
            } else {
                // Fade out when no channels are running
                masterSmoothLevel *= 0.9f;
                masterLevel = masterSmoothLevel;
            }
            
            // Find master meter and voltage label
            for (NSView *subview in self.masterSection.subviews) {
                if ([subview isKindOfClass:[NSView class]] && subview.frame.size.width <= 12 && subview.frame.size.height > 50) {
                    // This is the master meter
                    [subview setWantsLayer:YES];
                    
                    CALayer *meterLayer = subview.layer;
                    meterLayer.sublayers = nil;
                    
                    if (masterLevel > 0.02f) {
                        CGFloat meterHeight = subview.frame.size.height * masterLevel;
                        
                        meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
                        
                        CALayer *activeMeter = [CALayer layer];
CGFloat yPosition = 0; // Position at bottom (снизу вверх)
activeMeter.frame = CGRectMake(0, yPosition, subview.frame.size.width, meterHeight);
                        
                        if (masterLevel < 0.6f) {
                            activeMeter.backgroundColor = [NSColor colorWithRed:0.0 green:0.8 blue:0.2 alpha:1.0].CGColor;
                        } else if (masterLevel < 0.85f) {
                            activeMeter.backgroundColor = [NSColor colorWithRed:0.9 green:0.9 blue:0.0 alpha:1.0].CGColor;
                        } else {
                            activeMeter.backgroundColor = [NSColor colorWithRed:1.0 green:0.3 blue:0.0 alpha:1.0].CGColor;
                        }
                        
                        [meterLayer addSublayer:activeMeter];
                    } else {
                        meterLayer.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0].CGColor;
                    }
                } else if ([subview isKindOfClass:[NSTextField class]]) {
                    NSTextField *textField = (NSTextField *)subview;
                    if ([textField.stringValue containsString:@"V"]) {
                        // Show master output voltage - actual processed signal
                        float voltage = 0.0f;
                        if (runningChannels > 0) {
                            // Calculate master output voltage from running channels
                            float totalOutputVoltage = 0.0f;
                            int channelsWithOutput = 0;
                            
                            auto* mixerState = self.mixerEngine->getMixerState();
                            if (mixerState) {
                                for (size_t i = 0; i < mixerState->channels.size() && i < 8; i++) {
                                    auto& channel = mixerState->channels[i];
                                    if (channel->state == ChannelState::RUNNING) {
                                        // Get the actual processed output signal from each channel
                                        float channelOutputVoltage = channel->outputMeter.getCurrentLevel();
                                        if (fabs(channelOutputVoltage) > 0.01f) {
                                            totalOutputVoltage += channelOutputVoltage;
                                            channelsWithOutput++;
                                        }
                                    }
                                }
                            }
                            
                            // Calculate average output and apply master volume
                            if (channelsWithOutput > 0) {
                                float avgOutputVoltage = totalOutputVoltage / channelsWithOutput;
                                float masterVolume = self.masterVolumeSlider ? [self.masterVolumeSlider floatValue] : 1.0f;
                                voltage = avgOutputVoltage * masterVolume;
                            }
                        }
                        [textField setStringValue:[NSString stringWithFormat:@"%.1fV", voltage]];
                        
                        if (fabs(voltage) > 8.0f) {
                            [textField setTextColor:[NSColor systemRedColor]];
                        } else if (fabs(voltage) > 5.0f) {
                            [textField setTextColor:[NSColor systemYellowColor]];
                        } else {
                            [textField setTextColor:[NSColor lightGrayColor]];
                        }
                    }
                }
            }
        }
    } @catch (NSException *exception) {
        NSLog(@"Error in updateChannelMeters: %@", exception.reason);
    }
}

- (void)updateChannelStates {
    // Update all channel UI elements to reflect current state
    for (int i = 0; i < 8; i++) {
        if (self.mixerEngine) {
            float level = self.mixerEngine->getChannelLevel(i);
            bool isMuted = self.mixerEngine->isChannelMuted(i);
            bool isSolo = self.mixerEngine->isChannelSolo(i);
            
            // Update UI elements for channel i
            // Implementation to find and update specific controls
        }
    }
}

// Missing method implementations
- (void)refreshDeviceLists {
    // Refresh input and output device lists
    NSLog(@"Refreshing device lists");
}

- (void)showDeviceConfigDialog:(OSCDeviceConfig*)device forChannel:(int)channelIndex {
    NSLog(@"Show device config for channel %d", channelIndex);
    
    ChannelConfigurationDialog *channelDialog = [[ChannelConfigurationDialog alloc] initWithParentWindow:self.window 
                                                                                                channelId:channelIndex 
                                                                                              mixerEngine:self.mixerEngine];
    [channelDialog showDialog:^(BOOL cancelled) {
        if (!cancelled) {
            NSLog(@"Channel %d configuration updated", channelIndex);
            [self updateChannelStates];
            [self refreshDeviceLists];
        }
    }];
}

- (IBAction)addInputDevice:(id)sender {
    NSLog(@"Add input device");
    
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.window];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            // Add device to current channel (assume channel 0 for now)
            if (self.mixerEngine) {
                bool success = self.mixerEngine->addInputDevice(0, config);
                if (success) {
                    NSLog(@"Input device added successfully: %s", config.deviceName.c_str());
                    [self refreshDeviceLists];
                } else {
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Failed to Add Device";
                    alert.informativeText = @"Could not add the input device to the mixer.";
                    [alert runModal];
                }
            }
        }
    }];
}

- (IBAction)addOutputDevice:(id)sender {
    NSLog(@"Add output device");
    
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.window];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            // Add device to current channel (assume channel 0 for now)
            if (self.mixerEngine) {
                bool success = self.mixerEngine->addOutputDevice(0, config);
                if (success) {
                    NSLog(@"Output device added successfully: %s", config.deviceName.c_str());
                    [self refreshDeviceLists];
                } else {
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Failed to Add Device";
                    alert.informativeText = @"Could not add the output device to the mixer.";
                    [alert runModal];
                }
            }
        }
    }];
}

- (IBAction)removeDevice:(id)sender {
    // Remove device
    NSLog(@"Remove device");
}

#pragma mark - New Device Selection Methods

- (void)selectInputDevice:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Select input device for channel %d", channelIndex);
    
    // Create device selection popover
    [self showDeviceSelectionPopover:button 
                           forChannel:channelIndex 
                                isInput:YES];
}

- (void)selectOutputDevice:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Select output device for channel %d", channelIndex);
    
    // Create device selection popover
    [self showDeviceSelectionPopover:button 
                           forChannel:channelIndex 
                                isInput:NO];
}

- (void)showChannelSettings:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"Show detailed OSC settings for channel %d", channelIndex);
    
    // Create settings window to show connected devices and their parameters
    [self showChannelDevicesWindow:channelIndex];
}

- (void)showChannelDevicesWindow:(int)channelIndex {
    // Create window for channel device settings
    NSRect windowFrame = NSMakeRect(0, 0, 500, 400);
    NSWindow *settingsWindow = [[NSWindow alloc] initWithContentRect:windowFrame
                                                           styleMask:(NSWindowStyleMaskTitled | 
                                                                    NSWindowStyleMaskClosable | 
                                                                    NSWindowStyleMaskResizable)
                                                             backing:NSBackingStoreBuffered
                                                               defer:NO];
    
    [settingsWindow setTitle:[NSString stringWithFormat:@"Channel %d Settings", channelIndex + 1]];
    [settingsWindow center];
    
    // Create content view
    NSView *contentView = settingsWindow.contentView;
    
    // Title
    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 360, 460, 25)];
    [titleLabel setStringValue:[NSString stringWithFormat:@"Channel %d - Connected Devices", channelIndex + 1]];
    [titleLabel setFont:[NSFont boldSystemFontOfSize:16]];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [contentView addSubview:titleLabel];
    
    // Get connected devices from mixer engine
    auto* mixerState = self.mixerEngine->getMixerState();
    std::vector<OSCDeviceConfig> inputDevices;
    std::vector<OSCDeviceConfig> outputDevices;
    
    if (mixerState && channelIndex < mixerState->channels.size()) {
        auto& channel = mixerState->channels[channelIndex];
        inputDevices = channel->inputDevices;
        outputDevices = channel->outputDevices;
    }
    
    // Input devices section
    NSTextField *inputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 320, 200, 20)];
    [inputLabel setStringValue:@"Input Devices:"];
    [inputLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [inputLabel setBezeled:NO];
    [inputLabel setDrawsBackground:NO];
    [inputLabel setEditable:NO];
    [contentView addSubview:inputLabel];
    
    // Input devices list
    NSScrollView *inputScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 240, 460, 75)];
    NSTableView *inputTableView = [[NSTableView alloc] init];
    
    NSTableColumn *inputNameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    [inputNameColumn setTitle:@"Device Name"];
    [inputNameColumn setWidth:150];
    [inputTableView addTableColumn:inputNameColumn];
    
    NSTableColumn *inputAddressColumn = [[NSTableColumn alloc] initWithIdentifier:@"address"];
    [inputAddressColumn setTitle:@"Network Address"];
    [inputAddressColumn setWidth:120];
    [inputTableView addTableColumn:inputAddressColumn];
    
    NSTableColumn *inputPortColumn = [[NSTableColumn alloc] initWithIdentifier:@"port"];
    [inputPortColumn setTitle:@"Port"];
    [inputPortColumn setWidth:60];
    [inputTableView addTableColumn:inputPortColumn];
    
    NSTableColumn *inputOSCColumn = [[NSTableColumn alloc] initWithIdentifier:@"osc"];
    [inputOSCColumn setTitle:@"OSC Address"];
    [inputOSCColumn setWidth:120];
    [inputTableView addTableColumn:inputOSCColumn];
    
    [inputScrollView setDocumentView:inputTableView];
    [inputScrollView setHasVerticalScroller:YES];
    [contentView addSubview:inputScrollView];
    
    // Output devices section
    NSTextField *outputLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 200, 200, 20)];
    [outputLabel setStringValue:@"Output Devices:"];
    [outputLabel setFont:[NSFont boldSystemFontOfSize:14]];
    [outputLabel setBezeled:NO];
    [outputLabel setDrawsBackground:NO];
    [outputLabel setEditable:NO];
    [contentView addSubview:outputLabel];
    
    // Output devices list
    NSScrollView *outputScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 120, 460, 75)];
    NSTableView *outputTableView = [[NSTableView alloc] init];
    
    NSTableColumn *outputNameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    [outputNameColumn setTitle:@"Device Name"];
    [outputNameColumn setWidth:150];
    [outputTableView addTableColumn:outputNameColumn];
    
    NSTableColumn *outputAddressColumn = [[NSTableColumn alloc] initWithIdentifier:@"address"];
    [outputAddressColumn setTitle:@"Network Address"];
    [outputAddressColumn setWidth:120];
    [outputTableView addTableColumn:outputAddressColumn];
    
    NSTableColumn *outputPortColumn = [[NSTableColumn alloc] initWithIdentifier:@"port"];
    [outputPortColumn setTitle:@"Port"];
    [outputPortColumn setWidth:60];
    [outputTableView addTableColumn:outputPortColumn];
    
    NSTableColumn *outputOSCColumn = [[NSTableColumn alloc] initWithIdentifier:@"osc"];
    [outputOSCColumn setTitle:@"OSC Address"];
    [outputOSCColumn setWidth:120];
    [outputTableView addTableColumn:outputOSCColumn];
    
    [outputScrollView setDocumentView:outputTableView];
    [outputScrollView setHasVerticalScroller:YES];
    [contentView addSubview:outputScrollView];
    
    // Populate tables with device data
    NSMutableArray *inputDeviceData = [[NSMutableArray alloc] init];
    for (const auto& device : inputDevices) {
        [inputDeviceData addObject:@{
            @"name": [NSString stringWithUTF8String:device.deviceName.c_str()],
            @"address": [NSString stringWithUTF8String:device.networkAddress.c_str()],
            @"port": [NSString stringWithFormat:@"%d", device.port],
            @"osc": [NSString stringWithUTF8String:device.oscAddress.c_str()]
        }];
    }
    
    NSMutableArray *outputDeviceData = [[NSMutableArray alloc] init];
    for (const auto& device : outputDevices) {
        [outputDeviceData addObject:@{
            @"name": [NSString stringWithUTF8String:device.deviceName.c_str()],
            @"address": [NSString stringWithUTF8String:device.networkAddress.c_str()],
            @"port": [NSString stringWithFormat:@"%d", device.port],
            @"osc": [NSString stringWithUTF8String:device.oscAddress.c_str()]
        }];
    }
    
    // Simple data source implementation using associated objects
    objc_setAssociatedObject(inputTableView, "deviceData", inputDeviceData, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(outputTableView, "deviceData", outputDeviceData, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    
    // Set data source and delegate
    [inputTableView setDataSource:self];
    [inputTableView setDelegate:self];
    [outputTableView setDataSource:self];
    [outputTableView setDelegate:self];
    
    // Channel status info
    NSTextField *statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 80, 460, 30)];
    if (mixerState && channelIndex < mixerState->channels.size()) {
        auto& channel = mixerState->channels[channelIndex];
        NSString *statusText = [NSString stringWithFormat:@"Channel Status: %@  |  Level: %.1fV  |  Input Devices: %lu  |  Output Devices: %lu",
                               (channel->state == ChannelState::RUNNING) ? @"RUNNING" : @"STOPPED",
                               channel->levelVolts,
                               (unsigned long)inputDevices.size(),
                               (unsigned long)outputDevices.size()];
        [statusLabel setStringValue:statusText];
    } else {
        [statusLabel setStringValue:@"Channel not available"];
    }
    [statusLabel setFont:[NSFont systemFontOfSize:12]];
    [statusLabel setBezeled:NO];
    [statusLabel setDrawsBackground:NO];
    [statusLabel setEditable:NO];
    [contentView addSubview:statusLabel];
    
    // Action buttons
    NSButton *removeInputButton = [[NSButton alloc] initWithFrame:NSMakeRect(20, 30, 120, 30)];
    [removeInputButton setTitle:@"Remove Input"];
    [removeInputButton setTarget:self];
    [removeInputButton setAction:@selector(removeSelectedInputDevice:)];
    removeInputButton.tag = channelIndex;
    [contentView addSubview:removeInputButton];
    
    NSButton *removeOutputButton = [[NSButton alloc] initWithFrame:NSMakeRect(150, 30, 120, 30)];
    [removeOutputButton setTitle:@"Remove Output"];
    [removeOutputButton setTarget:self];
    [removeOutputButton setAction:@selector(removeSelectedOutputDevice:)];
    removeOutputButton.tag = channelIndex;
    [contentView addSubview:removeOutputButton];
    
    NSButton *refreshButton = [[NSButton alloc] initWithFrame:NSMakeRect(280, 30, 100, 30)];
    [refreshButton setTitle:@"Refresh"];
    [refreshButton setTarget:self];
    [refreshButton setAction:@selector(refreshChannelSettings:)];
    refreshButton.tag = channelIndex;
    [contentView addSubview:refreshButton];
    
    NSButton *closeButton = [[NSButton alloc] initWithFrame:NSMakeRect(390, 30, 100, 30)];
    [closeButton setTitle:@"Close"];
    [closeButton setTarget:settingsWindow];
    [closeButton setAction:@selector(close)];
    [contentView addSubview:closeButton];
    
    // Store references for button actions
    objc_setAssociatedObject(removeInputButton, "inputTableView", inputTableView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(removeOutputButton, "outputTableView", outputTableView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(refreshButton, "settingsWindow", settingsWindow, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    
    // Show the window
    [settingsWindow makeKeyAndOrderFront:nil];
}

- (void)showDeviceSelectionPopover:(NSButton *)sourceButton 
                        forChannel:(int)channelIndex 
                           isInput:(BOOL)isInput {
    
    // Create popover content
    NSViewController *popoverContent = [[NSViewController alloc] init];
    NSView *contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 280, 300)];
    popoverContent.view = contentView;
    
    // Title label
    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 270, 260, 20)];
    [titleLabel setStringValue:[NSString stringWithFormat:@"Select %@ Device - CH %d", 
                                isInput ? @"Input" : @"Output", channelIndex + 1]];
    [titleLabel setFont:[NSFont boldSystemFontOfSize:12]];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [contentView addSubview:titleLabel];
    
    // Available devices list
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 80, 260, 180)];
    NSTableView *tableView = [[NSTableView alloc] init];
    
    // Configure table
    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    [nameColumn setTitle:@"Device Name"];
    [nameColumn setWidth:180];
    [tableView addTableColumn:nameColumn];
    
    NSTableColumn *statusColumn = [[NSTableColumn alloc] initWithIdentifier:@"status"];
    [statusColumn setTitle:@"Status"];
    [statusColumn setWidth:70];
    [tableView addTableColumn:statusColumn];
    
    [tableView setDataSource:self];
    [tableView setDelegate:self];
    [tableView setTarget:self];
    [tableView setDoubleAction:@selector(deviceTableDoubleClick:)];
    
    [scrollView setDocumentView:tableView];
    [scrollView setHasVerticalScroller:YES];
    [contentView addSubview:scrollView];
    
    // Auto-scan for devices
    NSButton *scanButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, 50, 100, 25)];
    [scanButton setTitle:@"Scan Devices"];
    [scanButton setTarget:self];
    [scanButton setAction:@selector(scanForDevices:)];
    [contentView addSubview:scanButton];
    
    // Manual add button
    NSButton *addButton = [[NSButton alloc] initWithFrame:NSMakeRect(120, 50, 100, 25)];
    [addButton setTitle:@"Add Manual"];
    [addButton setTarget:self];
    [addButton setAction:@selector(addManualDevice:)];
    [contentView addSubview:addButton];
    
    // Connect button
    NSButton *connectButton = [[NSButton alloc] initWithFrame:NSMakeRect(170, 10, 100, 30)];
    [connectButton setTitle:@"Connect"];
    [connectButton setTarget:self];
    [connectButton setAction:@selector(connectSelectedDevice:)];
    [connectButton setKeyEquivalent:@"\r"];
    [contentView addSubview:connectButton];
    
    // Store channel info for later use
    connectButton.tag = channelIndex;
    tableView.tag = isInput ? 1 : 0;
    
    // Create and show popover
    NSPopover *popover = [[NSPopover alloc] init];
    popover.contentViewController = popoverContent;
    popover.behavior = NSPopoverBehaviorTransient;
    
    // Store popover reference for later closing
    self.currentPopover = popover;
    
    [popover showRelativeToRect:sourceButton.bounds 
                         ofView:sourceButton 
                  preferredEdge:NSRectEdgeMaxY];
    
    // Trigger initial device scan
    [self scanForDevices:nil];
    
    // Reload table data to ensure it's displayed
    [tableView reloadData];
}

- (void)scanForDevices:(id)sender {
    // Clear existing devices
    [self.inputDevices removeAllObjects];
    [self.outputDevices removeAllObjects];
    
    // Get real audio devices from AudioDeviceManager
    if (self.audioDeviceManager) {
        // Refresh device list
        self.audioDeviceManager->refreshDeviceList();
        
        // Get input devices
        auto inputDevices = self.audioDeviceManager->getInputDevices();
        for (const auto& device : inputDevices) {
            if (device.isCurrentlyAvailable && device.maxInputChannels > 0) {
                NSString *deviceName = [NSString stringWithUTF8String:device.name.c_str()];
                NSString *statusStr = device.isCurrentlyAvailable ? @"Available" : @"Unavailable";
                
                [self.inputDevices addObject:@{
                    @"name": deviceName,
                    @"type": @"AudioInput",
                    @"status": statusStr,
                    @"deviceIndex": @(device.index),
                    @"channels": @(device.maxInputChannels),
                    @"hostApi": [NSString stringWithUTF8String:device.hostApi.c_str()],
                    @"sampleRate": @(device.defaultSampleRate)
                }];
            }
        }
        
        // Get output devices
        auto outputDevices = self.audioDeviceManager->getOutputDevices();
        for (const auto& device : outputDevices) {
            if (device.isCurrentlyAvailable && device.maxOutputChannels > 0) {
                NSString *deviceName = [NSString stringWithUTF8String:device.name.c_str()];
                NSString *statusStr = device.isCurrentlyAvailable ? @"Available" : @"Unavailable";
                
                [self.outputDevices addObject:@{
                    @"name": deviceName,
                    @"type": @"AudioOutput",
                    @"status": statusStr,
                    @"deviceIndex": @(device.index),
                    @"channels": @(device.maxOutputChannels),
                    @"hostApi": [NSString stringWithUTF8String:device.hostApi.c_str()],
                    @"sampleRate": @(device.defaultSampleRate)
                }];
            }
        }
    }
    
    // Add OSC output devices
    [self.outputDevices addObjectsFromArray:@[
        @{@"name": @"TouchOSC (192.168.1.100)", @"type": @"OSC", @"status": @"Available", @"address": @"192.168.1.100", @"port": @8000},
        @{@"name": @"Ableton Live (localhost)", @"type": @"OSC", @"status": @"Available", @"address": @"127.0.0.1", @"port": @9001},
        @{@"name": @"Max/MSP Patch (localhost)", @"type": @"OSC", @"status": @"Available", @"address": @"127.0.0.1", @"port": @7000},
        @{@"name": @"TouchDesigner (localhost)", @"type": @"OSC", @"status": @"Available", @"address": @"127.0.0.1", @"port": @9000},
        @{@"name": @"VCV Rack (localhost)", @"type": @"OSC", @"status": @"Available", @"address": @"127.0.0.1", @"port": @8001},
        @{@"name": @"Custom OSC Device", @"type": @"OSC", @"status": @"Configurable", @"address": @"127.0.0.1", @"port": @9002}
    ]];
    
    // Add CV/Modular output devices
    [self.outputDevices addObjectsFromArray:@[
        @{@"name": @"Expert Sleepers ES-9 (USB)", @"type": @"CV", @"status": @"Available", @"channels": @16},
        @{@"name": @"MOTU 828mk3 (CV Outputs)", @"type": @"CV", @"status": @"Available", @"channels": @8},
        @{@"name": @"Eurorack Silent Way", @"type": @"CV", @"status": @"Available", @"channels": @4}
    ]];
    
    NSLog(@"Scanned and found %lu input devices, %lu output devices", 
          (unsigned long)self.inputDevices.count, 
          (unsigned long)self.outputDevices.count);
}

- (void)addManualDevice:(id)sender {
    // Show manual device configuration dialog
    DeviceConfigurationDialog *deviceDialog = [[DeviceConfigurationDialog alloc] initWithParentWindow:self.window];
    [deviceDialog showDialog:^(OSCDeviceConfig config, BOOL cancelled) {
        if (!cancelled) {
            // Add to appropriate device list
            NSDictionary *deviceInfo = @{
                @"name": [NSString stringWithUTF8String:config.deviceName.c_str()],
                @"type": @"Manual",
                @"status": @"Configured"
            };
            [self.outputDevices addObject:deviceInfo];
            NSLog(@"Manual device added: %@", deviceInfo[@"name"]);
        }
    }];
}

- (void)connectSelectedDevice:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    NSLog(@"DEBUG: connectSelectedDevice called for channel %d", channelIndex);
    
    // Check microphone permission first for audio input devices
#ifdef __APPLE__
    NSView *popoverContentView = button.superview;
    NSTableView *tableView = nil;
    
    // Find the table view to check which device is selected
    for (NSView *subview in popoverContentView.subviews) {
        if ([subview isKindOfClass:[NSScrollView class]]) {
            NSScrollView *scrollView = (NSScrollView *)subview;
            if ([scrollView.documentView isKindOfClass:[NSTableView class]]) {
                tableView = (NSTableView *)scrollView.documentView;
                break;
            }
        }
    }
    
    if (tableView && tableView.selectedRow >= 0) {
        BOOL isInput = tableView.tag == 1;
        NSArray *devices = isInput ? self.inputDevices : self.outputDevices;
        
        if (tableView.selectedRow < devices.count) {
            NSDictionary *selectedDevice = devices[tableView.selectedRow];
            NSString *deviceType = selectedDevice[@"type"];
            
            // Check permission for audio input devices
            if (isInput && [deviceType isEqualToString:@"AudioInput"]) {
                if (!CHECK_MICROPHONE_PERMISSION()) {
                    NSAlert *alert = [[NSAlert alloc] init];
                    alert.messageText = @"Microphone Permission Required";
                    alert.informativeText = @"This app needs microphone access to use audio input devices.\n\nPlease grant microphone access in:\nSystem Preferences > Security & Privacy > Privacy > Microphone";
                    [alert addButtonWithTitle:@"Open System Preferences"];
                    [alert addButtonWithTitle:@"Cancel"];
                    
                    NSModalResponse response = [alert runModal];
                    if (response == NSAlertFirstButtonReturn) {
                        // Open System Preferences to the Privacy pane
                        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone"]];
                    }
                    return;
                }
            }
        }
    }
#endif
    
    // Now continue with the rest of the method logic
    // tableView was already found above if permission check was needed
    if (!tableView) {
        // Find the table view again if we didn't check permissions
        NSView *popoverContentView = button.superview;
        for (NSView *subview in popoverContentView.subviews) {
            if ([subview isKindOfClass:[NSScrollView class]]) {
                NSScrollView *scrollView = (NSScrollView *)subview;
                if ([scrollView.documentView isKindOfClass:[NSTableView class]]) {
                    tableView = (NSTableView *)scrollView.documentView;
                    break;
                }
            }
        }
    }
    
    if (!tableView || tableView.selectedRow < 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"No Device Selected";
        alert.informativeText = @"Please select a device from the list before connecting.";
        [alert runModal];
        return;
    }
    
    // Get the selected device
    BOOL isInput = tableView.tag == 1;
    NSArray *devices = isInput ? self.inputDevices : self.outputDevices;
    NSInteger selectedRow = tableView.selectedRow;
    
    if (selectedRow >= devices.count) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Invalid Selection";
        alert.informativeText = @"The selected device is no longer available.";
        [alert runModal];
        return;
    }
    
    NSDictionary *selectedDevice = devices[selectedRow];
    NSString *deviceName = selectedDevice[@"name"];
    NSString *deviceType = selectedDevice[@"type"];
    
    // Create OSCDeviceConfig
    OSCDeviceConfig deviceConfig;
    deviceConfig.deviceId = [[NSString stringWithFormat:@"device_%d_%@_%ld", channelIndex, deviceType, (long)[[NSDate date] timeIntervalSince1970]] UTF8String];
    deviceConfig.deviceName = [deviceName UTF8String];
    deviceConfig.protocolType = OSCProtocolType::UDP_UNICAST;
    
    // Set device-specific configuration based on type
    if ([deviceType isEqualToString:@"AudioInput"]) {
        deviceConfig.deviceType = OSCDeviceType::AUDIO_INPUT;
        deviceConfig.networkAddress = "127.0.0.1";
        deviceConfig.port = 9000 + channelIndex;
        deviceConfig.localPort = 8000 + channelIndex; // For receiving OSC from audio processing
        deviceConfig.oscAddress = [[NSString stringWithFormat:@"/channel/%d/audio/input", channelIndex + 1] UTF8String];
        
        // Store audio device index for actual hardware access
        NSNumber *deviceIndexObj = selectedDevice[@"deviceIndex"];
        if (deviceIndexObj) {
            deviceConfig.audioDeviceIndex = [deviceIndexObj intValue];
        }
    } else if ([deviceType isEqualToString:@"AudioOutput"]) {
        deviceConfig.deviceType = OSCDeviceType::AUDIO_OUTPUT;
        deviceConfig.networkAddress = "127.0.0.1";
        deviceConfig.port = 9000 + channelIndex;
        deviceConfig.localPort = 0; // Output devices don't need local port
        deviceConfig.oscAddress = [[NSString stringWithFormat:@"/channel/%d/audio/output", channelIndex + 1] UTF8String];
        
        // Store audio device index for actual hardware access
        NSNumber *deviceIndexObj = selectedDevice[@"deviceIndex"];
        if (deviceIndexObj) {
            deviceConfig.audioDeviceIndex = [deviceIndexObj intValue];
        }
    } else if ([deviceType isEqualToString:@"CV"]) {
        deviceConfig.deviceType = isInput ? OSCDeviceType::CV_INPUT : OSCDeviceType::CV_OUTPUT;
        deviceConfig.networkAddress = "127.0.0.1";
        deviceConfig.port = 9020 + channelIndex;
        deviceConfig.localPort = isInput ? (8020 + channelIndex) : 0;
        deviceConfig.oscAddress = [[NSString stringWithFormat:@"/channel/%d/cv", channelIndex + 1] UTF8String];
    } else if ([deviceType isEqualToString:@"OSC"]) {
        deviceConfig.deviceType = isInput ? OSCDeviceType::OSC_INPUT : OSCDeviceType::OSC_OUTPUT;
        
        // Get network address and port from device info
        NSString *address = selectedDevice[@"address"];
        NSNumber *portNum = selectedDevice[@"port"];
        
        if (address) {
            deviceConfig.networkAddress = [address UTF8String];
        } else {
            // Extract IP address if it's in the device name
            NSRange ipRange = [deviceName rangeOfString:@"(" options:NSBackwardsSearch];
            if (ipRange.location != NSNotFound) {
                NSString *ipPart = [deviceName substringFromIndex:ipRange.location + 1];
                NSRange closeRange = [ipPart rangeOfString:@")"];
                if (closeRange.location != NSNotFound) {
                    deviceConfig.networkAddress = [[ipPart substringToIndex:closeRange.location] UTF8String];
                } else {
                    deviceConfig.networkAddress = "127.0.0.1";
                }
            } else {
                deviceConfig.networkAddress = "127.0.0.1";
            }
        }
        
        deviceConfig.port = portNum ? [portNum intValue] : (8000 + channelIndex);
        deviceConfig.localPort = isInput ? (8000 + channelIndex) : 0;
        deviceConfig.oscAddress = [[NSString stringWithFormat:@"/channel/%d", channelIndex + 1] UTF8String];
    } else {
        // Manual or other type
        deviceConfig.deviceType = isInput ? OSCDeviceType::OSC_INPUT : OSCDeviceType::OSC_OUTPUT;
        deviceConfig.networkAddress = "127.0.0.1";
        deviceConfig.port = 9000 + channelIndex;
        deviceConfig.localPort = isInput ? (9000 + channelIndex) : 0;
        deviceConfig.oscAddress = [[NSString stringWithFormat:@"/channel/%d", channelIndex + 1] UTF8String];
    }
    
    deviceConfig.enabled = true;
    deviceConfig.signalLevel = 1.0f;
    deviceConfig.connected = true;  // Mark as connected
    
    // Set localPort for receiver
    if ([deviceType isEqualToString:@"Audio"]) {
        deviceConfig.localPort = 9000 + channelIndex;
    } else if ([deviceType isEqualToString:@"MIDI"]) {
        deviceConfig.localPort = 9010 + channelIndex;
    } else if ([deviceType isEqualToString:@"CV"]) {
        deviceConfig.localPort = 9020 + channelIndex;
    } else if ([deviceType isEqualToString:@"OSC"]) {
        deviceConfig.localPort = 8000 + channelIndex;
    } else {
        deviceConfig.localPort = 9000 + channelIndex;
    }
    
    // Debug: Print device configuration
    NSLog(@"DEBUG: Created device config with localPort = %d for device %s (channel %d)", 
          deviceConfig.localPort, deviceConfig.deviceId.c_str(), channelIndex);
    
    // Add the device to the mixer engine
    if (self.mixerEngine) {
        bool success;
        if (isInput) {
            success = self.mixerEngine->addInputDevice(channelIndex, deviceConfig);
        } else {
            success = self.mixerEngine->addOutputDevice(channelIndex, deviceConfig);
        }
        
        if (success) {
            NSLog(@"%@ device '%@' added successfully to channel %d", 
                  isInput ? @"Input" : @"Output", deviceName, channelIndex + 1);
            
            [self.statusLabel setStringValue:[NSString stringWithFormat:@"%@ device connected to channel %d", 
                                              isInput ? @"Input" : @"Output", channelIndex + 1]];
            
            // Update the channel button appearance to show connection
            [self updateChannelButtonAppearance:channelIndex isInput:isInput connected:YES];
            
            // Start the channel if it's not running
            if (!self.mixerEngine->isRunning()) {
                self.mixerEngine->start();
                [self.startStopButton setTitle:@"STOP"];
                [self.statusLabel setStringValue:@"Mixer running"];
                [self startMeterUpdates];
            }
            
            // Start the specific channel
            self.mixerEngine->startChannel(channelIndex);
            
            // Close the popover using the stored reference
            if (self.currentPopover) {
                [self.currentPopover close];
                self.currentPopover = nil;
            }
            
            // Refresh the device lists and channel states
            [self refreshDeviceLists];
            [self updateChannelStates];
            
        } else {
            NSAlert *alert = [[NSAlert alloc] init];
            alert.messageText = @"Failed to Connect Device";
            alert.informativeText = [NSString stringWithFormat:@"Could not add the %@ device '%@' to channel %d. The channel may have reached its device limit (8 max) or the device configuration is invalid.", 
                                   isInput ? @"input" : @"output", deviceName, channelIndex + 1];
            [alert runModal];
        }
    } else {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Mixer Engine Not Available";
        alert.informativeText = @"The mixer engine is not initialized. Please restart the application.";
        [alert runModal];
    }
}

- (void)deviceTableDoubleClick:(id)sender {
    NSTableView *tableView = (NSTableView *)sender;
    NSInteger selectedRow = [tableView selectedRow];
    
    if (selectedRow >= 0) {
        BOOL isInput = tableView.tag == 1;
        NSArray *devices = isInput ? self.inputDevices : self.outputDevices;
        
        if (selectedRow < devices.count) {
            NSDictionary *device = devices[selectedRow];
            NSLog(@"Double-clicked device: %@", device[@"name"]);
            
            // Auto-connect on double-click
            [self connectSelectedDevice:nil];
        }
    }
}

#pragma mark - NSTableView DataSource & Delegate

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    // Check if this is a settings window table view with associated device data
    NSArray *deviceData = objc_getAssociatedObject(tableView, "deviceData");
    if (deviceData) {
        return deviceData.count;
    }
    
    // Fallback to device selection popover logic
    BOOL isInput = tableView.tag == 1;
    return isInput ? self.inputDevices.count : self.outputDevices.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    // Check if this is a settings window table view with associated device data
    NSArray *deviceData = objc_getAssociatedObject(tableView, "deviceData");
    if (deviceData && row < deviceData.count) {
        NSDictionary *device = deviceData[row];
        if ([tableColumn.identifier isEqualToString:@"name"]) {
            return device[@"name"];
        } else if ([tableColumn.identifier isEqualToString:@"address"]) {
            return device[@"address"];
        } else if ([tableColumn.identifier isEqualToString:@"port"]) {
            return device[@"port"];
        } else if ([tableColumn.identifier isEqualToString:@"osc"]) {
            return device[@"osc"];
        }
        return @"";
    }
    
    // Fallback to device selection popover logic
    BOOL isInput = tableView.tag == 1;
    NSArray *devices = isInput ? self.inputDevices : self.outputDevices;
    
    if (row < devices.count) {
        NSDictionary *device = devices[row];
        if ([tableColumn.identifier isEqualToString:@"name"]) {
            return device[@"name"];
        } else if ([tableColumn.identifier isEqualToString:@"status"]) {
            return device[@"status"];
        }
    }
    return @"";
}

- (void)updateChannelButtonAppearance:(int)channelIndex isInput:(BOOL)isInput connected:(BOOL)connected {
    // Find the channel strip for the given index
    if (channelIndex < 0 || channelIndex >= self.channelStrips.count) {
        return;
    }
    
    NSView *channelStrip = self.channelStrips[channelIndex];
    
    // Find the input or output button in this channel strip
    for (NSView *subview in channelStrip.subviews) {
        if ([subview isKindOfClass:[NSButton class]]) {
            NSButton *button = (NSButton *)subview;
            NSString *buttonTitle = [button title];
            
            if (isInput && [buttonTitle containsString:@"INPUT"]) {
                [button setWantsLayer:YES];
                if (connected) {
                    button.layer.backgroundColor = [NSColor systemGreenColor].CGColor;
                    [button setTitle:@"INPUT ✓"];
                } else {
                    button.layer.backgroundColor = [NSColor colorWithWhite:0.45 alpha:1.0].CGColor;
                    [button setTitle:@"INPUT"];
                }
                break;
            } else if (!isInput && [buttonTitle containsString:@"OUTPUT"]) {
                [button setWantsLayer:YES];
                if (connected) {
                    button.layer.backgroundColor = [NSColor systemBlueColor].CGColor;
                    [button setTitle:@"OUTPUT ✓"];
                } else {
                    button.layer.backgroundColor = [NSColor colorWithWhite:0.45 alpha:1.0].CGColor;
                    [button setTitle:@"OUTPUT"];
                }
                break;
            }
        }
    }
}

- (void)removeSelectedInputDevice:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    // Get the associated table view
    NSTableView *tableView = objc_getAssociatedObject(button, "inputTableView");
    if (!tableView || tableView.selectedRow < 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"No Device Selected";
        alert.informativeText = @"Please select an input device to remove.";
        [alert runModal];
        return;
    }
    
    // Get selected device info from mixer engine
    auto* mixerState = self.mixerEngine->getMixerState();
    if (mixerState && channelIndex < mixerState->channels.size()) {
        auto& channel = mixerState->channels[channelIndex];
        if (tableView.selectedRow < channel->inputDevices.size()) {
            std::string deviceId = channel->inputDevices[tableView.selectedRow].deviceId;
            
            if (self.mixerEngine->removeInputDevice(channelIndex, deviceId)) {
                NSLog(@"Removed input device from channel %d", channelIndex + 1);
                [self updateChannelButtonAppearance:channelIndex isInput:YES connected:NO];
                
                // Refresh the settings window
                NSWindow *settingsWindow = objc_getAssociatedObject(button, "settingsWindow");
                if (settingsWindow) {
                    [settingsWindow close];
                    [self showChannelDevicesWindow:channelIndex];
                }
            }
        }
    }
}

- (void)removeSelectedOutputDevice:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    // Get the associated table view
    NSTableView *tableView = objc_getAssociatedObject(button, "outputTableView");
    if (!tableView || tableView.selectedRow < 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"No Device Selected";
        alert.informativeText = @"Please select an output device to remove.";
        [alert runModal];
        return;
    }
    
    // Get selected device info from mixer engine
    auto* mixerState = self.mixerEngine->getMixerState();
    if (mixerState && channelIndex < mixerState->channels.size()) {
        auto& channel = mixerState->channels[channelIndex];
        if (tableView.selectedRow < channel->outputDevices.size()) {
            std::string deviceId = channel->outputDevices[tableView.selectedRow].deviceId;
            
            if (self.mixerEngine->removeOutputDevice(channelIndex, deviceId)) {
                NSLog(@"Removed output device from channel %d", channelIndex + 1);
                [self updateChannelButtonAppearance:channelIndex isInput:NO connected:NO];
                
                // Refresh the settings window
                NSWindow *settingsWindow = objc_getAssociatedObject(button, "settingsWindow");
                if (settingsWindow) {
                    [settingsWindow close];
                    [self showChannelDevicesWindow:channelIndex];
                }
            }
        }
    }
}

- (void)refreshChannelSettings:(id)sender {
    NSButton *button = (NSButton *)sender;
    int channelIndex = (int)[button tag];
    
    // Close current settings window and reopen with fresh data
    NSWindow *settingsWindow = objc_getAssociatedObject(button, "settingsWindow");
    if (settingsWindow) {
        [settingsWindow close];
    }
    
    // Show updated settings window
    [self showChannelDevicesWindow:channelIndex];
}

- (void)dealloc {
    [self stopMeterUpdates];
    
    // Clean up audio device manager
    if (self.audioDeviceManager) {
        self.audioDeviceManager->cleanup();
        delete self.audioDeviceManager;
        self.audioDeviceManager = nullptr;
    }
}

@end
