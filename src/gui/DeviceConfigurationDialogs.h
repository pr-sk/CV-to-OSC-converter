#pragma once

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "../core/OSCMixerTypes.h"
#include "../core/OSCMixerEngine.h"
#include <memory>

// Protocol types for Objective-C
typedef enum {
    OSCProtocolTypeUDPUnicast = 0,
    OSCProtocolTypeUDPMulticast = 1,
    OSCProtocolTypeTCP = 2
} OSCProtocolTypeObjC;

// Message types for Objective-C
typedef enum {
    OSCMessageTypeFloat = 0,
    OSCMessageTypeInt = 1,
    OSCMessageTypeString = 2,
    OSCMessageTypeBlob = 3,
    OSCMessageTypeBundle = 4
} OSCMessageTypeObjC;

/**
 * @brief Device Configuration Dialog for adding/editing OSC devices
 */
@interface DeviceConfigurationDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSTextField *deviceNameField;
@property (nonatomic, strong) NSTextField *deviceIdField;
@property (nonatomic, strong) NSPopUpButton *protocolTypeButton;
@property (nonatomic, strong) NSTextField *networkAddressField;
@property (nonatomic, strong) NSTextField *portField;
@property (nonatomic, strong) NSTextField *localAddressField;
@property (nonatomic, strong) NSTextField *localPortField;
@property (nonatomic, strong) NSTextField *oscAddressField;
@property (nonatomic, strong) NSTextField *oscMessageField;
@property (nonatomic, strong) NSSlider *signalLevelSlider;
@property (nonatomic, strong) NSTextField *signalLevelLabel;
@property (nonatomic, strong) NSButton *enabledCheckbox;
@property (nonatomic, strong) NSTextField *namespaceField;
@property (nonatomic, strong) NSTextField *patternField;
@property (nonatomic, strong) NSMatrix *supportedTypesMatrix;
@property (nonatomic, strong) NSTextField *maxBundleSizeField;
@property (nonatomic, strong) NSTextField *timeoutField;
@property (nonatomic, strong) NSButton *useTimestampsCheckbox;
@property (nonatomic, strong) NSButton *okButton;
@property (nonatomic, strong) NSButton *cancelButton;
@property (nonatomic, strong) NSButton *testConnectionButton;

@property (nonatomic, assign) BOOL cancelled;
@property (nonatomic, assign) OSCDeviceConfig deviceConfig;
@property (nonatomic, assign) BOOL isEditMode;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow;
- (instancetype)initWithParentWindow:(NSWindow *)parentWindow deviceConfig:(OSCDeviceConfig)config;
- (void)showDialog:(void (^)(OSCDeviceConfig config, BOOL cancelled))completion;
- (OSCDeviceConfig)getDeviceConfig;

@end

/**
 * @brief Channel Configuration Dialog for configuring channels
 */
@interface ChannelConfigurationDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSTextField *channelNameField;
@property (nonatomic, strong) NSSlider *levelVoltsSlider;
@property (nonatomic, strong) NSTextField *levelVoltsLabel;
@property (nonatomic, strong) NSTextField *minRangeField;
@property (nonatomic, strong) NSTextField *maxRangeField;
@property (nonatomic, strong) NSColorWell *channelColorWell;
@property (nonatomic, strong) NSButton *showInMasterCheckbox;
@property (nonatomic, strong) NSTableView *inputDevicesTable;
@property (nonatomic, strong) NSTableView *outputDevicesTable;
@property (nonatomic, strong) NSButton *addInputDeviceButton;
@property (nonatomic, strong) NSButton *editInputDeviceButton;
@property (nonatomic, strong) NSButton *removeInputDeviceButton;
@property (nonatomic, strong) NSButton *addOutputDeviceButton;
@property (nonatomic, strong) NSButton *editOutputDeviceButton;
@property (nonatomic, strong) NSButton *removeOutputDeviceButton;
@property (nonatomic, strong) NSButton *okButton;
@property (nonatomic, strong) NSButton *cancelButton;

@property (nonatomic, assign) BOOL cancelled;
@property (nonatomic, assign) int channelId;
@property (nonatomic, strong) NSMutableArray *inputDevices;
@property (nonatomic, strong) NSMutableArray *outputDevices;
@property (nonatomic, assign) std::shared_ptr<OSCMixerEngine> mixerEngine;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                           channelId:(int)channelId 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)showDialog:(void (^)(BOOL cancelled))completion;

@end

/**
 * @brief Device Discovery Dialog for scanning and discovering OSC devices
 */
@interface DeviceDiscoveryDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSTableView *discoveredDevicesTable;
@property (nonatomic, strong) NSButton *scanButton;
@property (nonatomic, strong) NSButton *stopScanButton;
@property (nonatomic, strong) NSProgressIndicator *scanProgressIndicator;
@property (nonatomic, strong) NSTextField *scanStatusLabel;
@property (nonatomic, strong) NSButton *connectButton;
@property (nonatomic, strong) NSButton *configureButton;
@property (nonatomic, strong) NSButton *closeButton;
@property (nonatomic, strong) NSTextField *autoScanIntervalField;
@property (nonatomic, strong) NSButton *autoScanCheckbox;

@property (nonatomic, assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@property (nonatomic, strong) NSMutableArray *discoveredDevices;
@property (nonatomic, strong) NSTimer *scanTimer;
@property (nonatomic, assign) BOOL isScanning;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)showDialog;
- (void)startScanning;
- (void)stopScanning;

@end

/**
 * @brief Global Settings Dialog for mixer-wide configuration
 */
@interface GlobalSettingsDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSSlider *masterLevelSlider;
@property (nonatomic, strong) NSTextField *masterLevelLabel;
@property (nonatomic, strong) NSButton *masterMuteCheckbox;
@property (nonatomic, strong) NSTextField *defaultNetworkAddressField;
@property (nonatomic, strong) NSTextField *defaultPortField;
@property (nonatomic, strong) NSTextField *defaultLocalPortField;
@property (nonatomic, strong) NSPopUpButton *defaultProtocolButton;
@property (nonatomic, strong) NSTextField *maxDevicesPerChannelField;
@property (nonatomic, strong) NSTextField *messageQueueSizeField;
@property (nonatomic, strong) NSTextField *statsUpdateIntervalField;
@property (nonatomic, strong) NSButton *enableLoggingCheckbox;
@property (nonatomic, strong) NSTextField *logFilePathField;
@property (nonatomic, strong) NSButton *browseLogFileButton;
@property (nonatomic, strong) NSButton *enableLearningModeCheckbox;
@property (nonatomic, strong) NSButton *resetStatisticsButton;
@property (nonatomic, strong) NSButton *okButton;
@property (nonatomic, strong) NSButton *cancelButton;
@property (nonatomic, strong) NSButton *applyButton;

@property (nonatomic, assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@property (nonatomic, assign) BOOL cancelled;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)showDialog:(void (^)(BOOL cancelled))completion;

@end

/**
 * @brief Connection Monitor Dialog for monitoring device connections and statistics
 */
@interface ConnectionMonitorDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSTableView *connectionsTable;
@property (nonatomic, strong) NSTextField *totalMessagesLabel;
@property (nonatomic, strong) NSTextField *totalConnectionsLabel;
@property (nonatomic, strong) NSTextField *totalErrorsLabel;
@property (nonatomic, strong) NSTextField *messagesPerSecondLabel;
@property (nonatomic, strong) NSButton *refreshButton;
@property (nonatomic, strong) NSButton *resetStatsButton;
@property (nonatomic, strong) NSButton *disconnectButton;
@property (nonatomic, strong) NSButton *disconnectAllButton;
@property (nonatomic, strong) NSButton *exportStatsButton;
@property (nonatomic, strong) NSButton *closeButton;

@property (nonatomic, assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@property (nonatomic, strong) NSMutableArray *connectionData;
@property (nonatomic, strong) NSTimer *updateTimer;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)showDialog;
- (void)updateConnectionData;

@end

/**
 * @brief Configuration Preset Manager Dialog
 */
@interface ConfigurationPresetDialog : NSObject

@property (nonatomic, strong) NSWindow *dialogWindow;
@property (nonatomic, strong) NSTableView *presetsTable;
@property (nonatomic, strong) NSTextField *presetNameField;
@property (nonatomic, strong) NSTextView *presetDescriptionView;
@property (nonatomic, strong) NSButton *savePresetButton;
@property (nonatomic, strong) NSButton *loadPresetButton;
@property (nonatomic, strong) NSButton *deletePresetButton;
@property (nonatomic, strong) NSButton *exportPresetButton;
@property (nonatomic, strong) NSButton *importPresetButton;
@property (nonatomic, strong) NSButton *closeButton;

@property (nonatomic, assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@property (nonatomic, strong) NSMutableArray *presets;

- (instancetype)initWithParentWindow:(NSWindow *)parentWindow 
                         mixerEngine:(std::shared_ptr<OSCMixerEngine>)engine;
- (void)showDialog;

@end

// Utility functions for converting between C++ and Objective-C types
OSCDeviceConfig convertFromObjCToDeviceConfig(NSDictionary *dict);
NSDictionary* convertFromDeviceConfigToObjC(const OSCDeviceConfig& config);
