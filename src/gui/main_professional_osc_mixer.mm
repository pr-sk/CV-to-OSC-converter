#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include "ProfessionalOSCMixer.h"
#include "OSCMixerEngine.h"
#include "Version.h"
#include <memory>
#include <iostream>

@interface ProfessionalOSCMixerAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) ProfessionalOSCMixer *mainMixer;
@property (assign) std::shared_ptr<OSCMixerEngine> mixerEngine;
@end

@implementation ProfessionalOSCMixerAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    std::cout << "🎵 " << Version::getAppTitle() << " - Professional OSC Mixer" << std::endl;
    std::cout << "Version: " << Version::getVersionWithGit() << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Create mixer engine with 8 channels
    _mixerEngine = std::make_shared<OSCMixerEngine>(8);
    
    if (!_mixerEngine->initialize()) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Initialization Error"];
        [alert setInformativeText:@"Failed to initialize OSC Mixer Engine"];
        [alert addButtonWithTitle:@"Exit"];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }
    
    // Create main mixer window
    _mainMixer = [[ProfessionalOSCMixer alloc] initWithMixerEngine:_mixerEngine];
    [_mainMixer showWindow:nil];
    
    std::cout << "✅ Professional OSC Mixer initialized successfully" << std::endl;
    std::cout << "📊 Features:" << std::endl;
    std::cout << "   • 8 Channel Professional Mixer" << std::endl;
    std::cout << "   • Full OSC Protocol Support (UDP/TCP/Multicast/Broadcast)" << std::endl;
    std::cout << "   • Up to 8 Input and 8 Output devices per channel" << std::endl;
    std::cout << "   • Real-time Signal Metering with LED visualization" << std::endl;
    std::cout << "   • Professional Mixer Controls (Gain, Offset, Filter, Mix)" << std::endl;
    std::cout << "   • Individual Channel Start/Stop, Mute, Solo, Link" << std::endl;
    std::cout << "   • Advanced OSC Device Configuration" << std::endl;
    std::cout << "   • Support for all OSC Message Types" << std::endl;
    std::cout << "   • Time Tags, Bundles, Pattern Matching" << std::endl;
    std::cout << "   • Device Discovery and Auto-Reconnection" << std::endl;
    std::cout << "   • Configuration Save/Load" << std::endl;
    std::cout << "   • External Device Mapping (MIDI Control)" << std::endl;
    std::cout << "   • Learning Mode for automatic mapping" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    std::cout << "🛑 Shutting down Professional OSC Mixer..." << std::endl;
    
    if (_mixerEngine) {
        _mixerEngine->shutdown();
        std::cout << "✅ OSC Mixer Engine shutdown complete" << std::endl;
    }
    
    std::cout << "👋 Professional OSC Mixer terminated successfully" << std::endl;
    return NSTerminateNow;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Set up application
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        // Set application delegate
        ProfessionalOSCMixerAppDelegate *delegate = [[ProfessionalOSCMixerAppDelegate alloc] init];
        [app setDelegate:delegate];
        
        // Create menu bar
        NSMenu *mainMenu = [[NSMenu alloc] init];
        
        // Application menu
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [mainMenu addItem:appMenuItem];
        NSMenu *appMenu = [[NSMenu alloc] init];
        [appMenuItem setSubmenu:appMenu];
        
        NSString *appName = @"Professional OSC Mixer";
        NSMenuItem *aboutMenuItem = [[NSMenuItem alloc] initWithTitle:[@"About " stringByAppendingString:appName]
                                                               action:@selector(orderFrontStandardAboutPanel:)
                                                        keyEquivalent:@""];
        [appMenu addItem:aboutMenuItem];
        [appMenu addItem:[NSMenuItem separatorItem]];
        
        NSMenuItem *hideMenuItem = [[NSMenuItem alloc] initWithTitle:[@"Hide " stringByAppendingString:appName]
                                                              action:@selector(hide:)
                                                       keyEquivalent:@"h"];
        [appMenu addItem:hideMenuItem];
        
        NSMenuItem *hideOthersMenuItem = [[NSMenuItem alloc] initWithTitle:@"Hide Others"
                                                                    action:@selector(hideOtherApplications:)
                                                             keyEquivalent:@"h"];
        [hideOthersMenuItem setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
        [appMenu addItem:hideOthersMenuItem];
        
        NSMenuItem *showAllMenuItem = [[NSMenuItem alloc] initWithTitle:@"Show All"
                                                                 action:@selector(unhideAllApplications:)
                                                          keyEquivalent:@""];
        [appMenu addItem:showAllMenuItem];
        [appMenu addItem:[NSMenuItem separatorItem]];
        
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:appName]
                                                              action:@selector(terminate:)
                                                       keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        
        // File menu
        NSMenuItem *fileMenuItem = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
        [mainMenu addItem:fileMenuItem];
        NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
        [fileMenuItem setSubmenu:fileMenu];
        
        NSMenuItem *newMenuItem = [[NSMenuItem alloc] initWithTitle:@"New Configuration"
                                                             action:nil
                                                      keyEquivalent:@"n"];
        [fileMenu addItem:newMenuItem];
        
        NSMenuItem *openMenuItem = [[NSMenuItem alloc] initWithTitle:@"Open Configuration..."
                                                              action:nil
                                                       keyEquivalent:@"o"];
        [fileMenu addItem:openMenuItem];
        
        NSMenuItem *saveMenuItem = [[NSMenuItem alloc] initWithTitle:@"Save Configuration"
                                                              action:nil
                                                       keyEquivalent:@"s"];
        [fileMenu addItem:saveMenuItem];
        
        NSMenuItem *saveAsMenuItem = [[NSMenuItem alloc] initWithTitle:@"Save Configuration As..."
                                                                action:nil
                                                         keyEquivalent:@"S"];
        [saveAsMenuItem setKeyEquivalentModifierMask:NSEventModifierFlagShift | NSEventModifierFlagCommand];
        [fileMenu addItem:saveAsMenuItem];
        
        // Mixer menu
        NSMenuItem *mixerMenuItem = [[NSMenuItem alloc] initWithTitle:@"Mixer" action:nil keyEquivalent:@""];
        [mainMenu addItem:mixerMenuItem];
        NSMenu *mixerMenu = [[NSMenu alloc] initWithTitle:@"Mixer"];
        [mixerMenuItem setSubmenu:mixerMenu];
        
        NSMenuItem *startAllMenuItem = [[NSMenuItem alloc] initWithTitle:@"Start All Channels"
                                                                  action:nil
                                                           keyEquivalent:@""];
        [mixerMenu addItem:startAllMenuItem];
        
        NSMenuItem *stopAllMenuItem = [[NSMenuItem alloc] initWithTitle:@"Stop All Channels"
                                                                 action:nil
                                                          keyEquivalent:@""];
        [mixerMenu addItem:stopAllMenuItem];
        [mixerMenu addItem:[NSMenuItem separatorItem]];
        
        NSMenuItem *clearSoloMenuItem = [[NSMenuItem alloc] initWithTitle:@"Clear All Solo"
                                                                   action:nil
                                                            keyEquivalent:@""];
        [mixerMenu addItem:clearSoloMenuItem];
        
        NSMenuItem *clearMuteMenuItem = [[NSMenuItem alloc] initWithTitle:@"Clear All Mute"
                                                                   action:nil
                                                            keyEquivalent:@""];
        [mixerMenu addItem:clearMuteMenuItem];
        [mixerMenu addItem:[NSMenuItem separatorItem]];
        
        NSMenuItem *scanDevicesMenuItem = [[NSMenuItem alloc] initWithTitle:@"Scan for Devices"
                                                                     action:nil
                                                              keyEquivalent:@"r"];
        [mixerMenu addItem:scanDevicesMenuItem];
        
        // Device menu
        NSMenuItem *deviceMenuItem = [[NSMenuItem alloc] initWithTitle:@"Devices" action:nil keyEquivalent:@""];
        [mainMenu addItem:deviceMenuItem];
        NSMenu *deviceMenu = [[NSMenu alloc] initWithTitle:@"Devices"];
        [deviceMenuItem setSubmenu:deviceMenu];
        
        NSMenuItem *addInputMenuItem = [[NSMenuItem alloc] initWithTitle:@"Add Input Device..."
                                                                  action:nil
                                                           keyEquivalent:@"i"];
        [deviceMenu addItem:addInputMenuItem];
        
        NSMenuItem *addOutputMenuItem = [[NSMenuItem alloc] initWithTitle:@"Add Output Device..."
                                                                   action:nil
                                                            keyEquivalent:@"o"];
        [addOutputMenuItem setKeyEquivalentModifierMask:NSEventModifierFlagShift | NSEventModifierFlagCommand];
        [deviceMenu addItem:addOutputMenuItem];
        [deviceMenu addItem:[NSMenuItem separatorItem]];
        
        NSMenuItem *deviceListMenuItem = [[NSMenuItem alloc] initWithTitle:@"Device Status"
                                                                     action:nil
                                                              keyEquivalent:@"d"];
        [deviceMenu addItem:deviceListMenuItem];
        
        // Window menu
        NSMenuItem *windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
        [mainMenu addItem:windowMenuItem];
        NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        [windowMenuItem setSubmenu:windowMenu];
        [app setWindowsMenu:windowMenu];
        
        NSMenuItem *minimizeMenuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
                                                                  action:@selector(performMiniaturize:)
                                                           keyEquivalent:@"m"];
        [windowMenu addItem:minimizeMenuItem];
        
        NSMenuItem *zoomMenuItem = [[NSMenuItem alloc] initWithTitle:@"Zoom"
                                                              action:@selector(performZoom:)
                                                       keyEquivalent:@""];
        [windowMenu addItem:zoomMenuItem];
        
        // Help menu
        NSMenuItem *helpMenuItem = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
        [mainMenu addItem:helpMenuItem];
        NSMenu *helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
        [helpMenuItem setSubmenu:helpMenu];
        [app setHelpMenu:helpMenu];
        
        NSMenuItem *helpDocMenuItem = [[NSMenuItem alloc] initWithTitle:@"Professional OSC Mixer Help"
                                                                 action:nil
                                                          keyEquivalent:@"?"];
        [helpMenu addItem:helpDocMenuItem];
        
        [app setMainMenu:mainMenu];
        
        // Run the application
        [app run];
    }
    return 0;
}
