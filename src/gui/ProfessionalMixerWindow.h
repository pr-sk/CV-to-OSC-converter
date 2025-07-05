#import <Cocoa/Cocoa.h>
#import "OSCMixerEngine.h"
#import "OSCMixerTypes.h"
#import "AudioDeviceManager.h"
#import "DeviceConfigurationDialogs.h"
#include <memory>

@interface ProfessionalMixerWindow : NSWindowController {
    std::shared_ptr<OSCMixerEngine> _mixerEngine;
}

// Core mixer engine
@property (nonatomic, readonly) std::shared_ptr<OSCMixerEngine> mixerEngine;

// Main UI components
@property (strong) NSView *mainContentView;
@property (strong) NSView *channelStripContainer;
@property (strong) NSView *masterSection;
@property (strong) NSView *deviceSection;

// Control elements
@property (strong) NSButton *startStopButton;
@property (strong) NSButton *soloMixButton;
@property (strong) NSButton *masterMuteButton;
@property (strong) NSTextField *statusLabel;

// Channel strips (up to 8)
@property (strong) NSMutableArray<NSView*> *channelStrips;

// Master controls
@property (strong) NSSlider *masterVolumeSlider;
@property (strong) NSView *masterMeterView;

// Device management
@property (strong) NSTableView *inputDevicesTable;
@property (strong) NSTableView *outputDevicesTable;

// Menu and toolbar
@property (strong) NSMenu *mixerMenu;
@property (strong) NSToolbar *mixerToolbar;

// Initialize mixer window
- (instancetype)initWithEngine:(std::shared_ptr<OSCMixerEngine>)engine;

// UI Setup methods
- (void)setupMainInterface;
- (void)setupChannelStrips;
- (void)setupMasterSection;
- (void)setupDeviceSection;
- (void)setupMenuAndToolbar;

// Channel operations
- (void)createChannelStrip:(int)channelIndex atPosition:(NSPoint)position;
- (void)updateChannelMeters;
- (void)updateChannelStates;

// Channel control actions
- (void)channelLevelChanged:(id)sender;
- (void)channelSoloPressed:(id)sender;
- (void)channelMutePressed:(id)sender;
- (void)channelRecordPressed:(id)sender;
- (void)channelLearnPressed:(id)sender;
- (void)showInputDevices:(id)sender;
- (void)showOutputDevices:(id)sender;
- (void)showChannelConfig:(id)sender;

// Device management
- (void)refreshDeviceLists;
- (void)showDeviceConfigDialog:(OSCDeviceConfig*)device forChannel:(int)channelIndex;

// Mixer control actions
- (IBAction)toggleMixer:(id)sender;
- (IBAction)toggleSoloMix:(id)sender;
- (IBAction)masterVolumeChanged:(id)sender;
- (IBAction)masterMutePressed:(id)sender;
- (IBAction)addInputDevice:(id)sender;
- (IBAction)addOutputDevice:(id)sender;
- (IBAction)removeDevice:(id)sender;

// Configuration
- (IBAction)loadMixerConfiguration:(id)sender;
- (IBAction)saveMixerConfiguration:(id)sender;
- (IBAction)showMixerSettings:(id)sender;

// Real-time updates
- (void)startMeterUpdates;
- (void)stopMeterUpdates;
- (void)updateMetersTimer:(NSTimer*)timer;

@end
