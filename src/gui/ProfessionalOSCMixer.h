#pragma once

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/Foundation.h>
#include "OSCMixerTypes.h"
#include "OSCMixerEngine.h"
#include "AudioDeviceManager.h"
#include "Version.h"
#include <memory>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cmath>

// OSC Device Configuration Dialog
@interface OSCDeviceConfigDialog : NSWindowController
@property (strong) NSWindow *configWindow;
@property (strong) NSTextField *deviceIdField;
@property (strong) NSTextField *deviceNameField;
@property (strong) NSTextField *descriptionField;
@property (strong) NSPopUpButton *protocolTypePopup;
@property (strong) NSTextField *networkAddressField;
@property (strong) NSTextField *portField;
@property (strong) NSTextField *localAddressField;
@property (strong) NSTextField *localPortField;
@property (strong) NSTextField *oscAddressField;
@property (strong) NSTextField *oscMessageField;
@property (strong) NSSlider *signalLevelSlider;
@property (strong) NSSlider *signalOffsetSlider;
@property (strong) NSButton *invertSignalButton;
@property (strong) NSButton *enabledButton;
@property (strong) NSButton *autoReconnectButton;
@property (strong) NSTextField *timeoutField;
@property (strong) NSTextField *maxRetriesField;
@property (strong) NSTextField *bufferSizeField;
@property (strong) NSPopUpButton *messageTypePopup;
@property (strong) NSButton *useTimestampButton;
@property (strong) NSButton *useBundlesButton;

- (instancetype)initWithDevice:(OSCDeviceConfig*)device completion:(void(^)(OSCDeviceConfig, BOOL))completion;
- (void)showDialog;
@end

// Forward declarations for delegate protocols
@protocol ProfessionalChannelStripDelegate
// Basic level control for signal routing
- (void)channelLevelChanged:(NSNumber*)channelId value:(NSNumber*)value;
- (void)channelLinkClicked:(NSNumber*)channelId;
- (void)addInputDevice:(NSValue*)devicePtr toChannel:(NSNumber*)channelId;
- (void)addOutputDevice:(NSValue*)devicePtr toChannel:(NSNumber*)channelId;
- (void)updateDevice:(NSValue*)devicePtr;
- (void)removeDevice:(NSValue*)devicePtr fromChannel:(NSNumber*)channelId;
@end

// Professional Channel Strip
@interface ProfessionalChannelStrip : NSView <NSTableViewDataSource, NSTableViewDelegate>
@property (assign) int channelId;
@property (strong) NSTextField *channelLabel;
// Basic level control for routing functionality
@property (strong) NSSlider *levelFader;
@property (strong) NSTextField *levelDisplay;
// Start/Stop button removed - channels auto-start when input device is added
@property (strong) NSScrollView *inputDevicesScrollView;
@property (strong) NSScrollView *outputDevicesScrollView;
@property (strong) NSStackView *inputDevicesStack;
@property (strong) NSStackView *outputDevicesStack;
@property (strong) NSButton *addInputButton;
@property (strong) NSButton *addOutputButton;

// Device selection dialog properties
@property (strong) NSWindow *deviceSelectionWindow;
@property (strong) NSTableView *deviceSelectionTableView;
@property (strong) NSMutableArray *deviceSelectionList;
@property (assign) BOOL isInputDeviceSelection;
@property (assign) BOOL deviceSelectionCancelled;

@property (assign) id<ProfessionalChannelStripDelegate> delegate;
@property (assign) MixerChannel *mixerChannel;

- (instancetype)initWithChannelId:(int)channelId;
- (void)updateFromChannel:(MixerChannel*)channel;
// updateMeters removed - simplified OSC routing only

// Device selection dialog methods
- (void)cancelDeviceSelection:(id)sender;
- (void)selectDevice:(id)sender;
@end

// OSC Device Row View
@interface OSCDeviceRowView : NSView
@property (strong) NSTextField *deviceNameLabel;
@property (strong) NSTextField *connectionLabel;
@property (strong) NSButton *configButton;
@property (strong) NSButton *removeButton;
@property (strong) NSView *statusIndicator;
@property (assign) OSCDeviceConfig *device;
@property (assign) id delegate;

- (instancetype)initWithDevice:(OSCDeviceConfig*)device;
- (void)updateStatus:(DeviceStatus*)status;
@end

// Main Professional OSC Mixer Window
@interface ProfessionalOSCMixer : NSWindowController <ProfessionalChannelStripDelegate>
@property (strong) NSWindow *mainWindow;
@property (strong) NSScrollView *channelScrollView;
@property (strong) NSStackView *channelStackView;
@property (strong) NSView *masterSection;
// Master volume controls removed - simplified OSC routing only
@property (strong) NSTextField *statusLabel;
@property (strong) NSButton *scanDevicesButton;
@property (strong) NSPopUpButton *configurationPopup;
@property (strong) NSButton *saveConfigButton;
@property (strong) NSButton *loadConfigButton;

@property (strong) NSMutableArray<ProfessionalChannelStrip*> *channelStrips;
@property (assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@property (assign) std::shared_ptr<AudioDeviceManager> audioDeviceManager;
@property (strong) NSTimer *updateTimer;

- (instancetype)initWithMixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)setupUI;
- (void)updateDisplay;
- (std::shared_ptr<AudioDeviceManager>)getAudioDeviceManager;
@end
