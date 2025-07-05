#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import "ProfessionalMixerWindow.h"
#include "Version.h"
#include "OSCMixerEngine.h"
#include <memory>

@interface CVToOSCAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) ProfessionalMixerWindow *oscMixerWindow;
@property std::shared_ptr<OSCMixerEngine> mixerEngine;
@end

@implementation CVToOSCAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSLog(@"Starting CV to OSC Converter v2.0.0 - OSC Mixer Mode");
    
    // Initialize mixer engine with 8 channels
    self.mixerEngine = std::make_shared<OSCMixerEngine>(8);
    
    // Initialize the mixer engine
    if (self.mixerEngine->initialize()) {
        NSLog(@"✅ OSC Mixer Engine initialized successfully");
    } else {
        NSLog(@"❌ Failed to initialize OSC Mixer Engine");
        
        // Show error dialog
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Engine Initialization Error"];
        [alert setInformativeText:@"Failed to initialize the OSC Mixer Engine. Please check your audio and network configuration."];
        [alert addButtonWithTitle:@"OK"];
        [alert runModal];
        
        // Exit application
        [NSApp terminate:nil];
        return;
    }
    
    // Create and show OSC Mixer window directly
    self.oscMixerWindow = [[ProfessionalMixerWindow alloc] initWithEngine:self.mixerEngine];
    
    if (self.oscMixerWindow) {
        [self.oscMixerWindow showWindow:nil];
        
        // Set the mixer window as the main window
        [self.oscMixerWindow.window makeMainWindow];
        [self.oscMixerWindow.window makeKeyAndOrderFront:nil];
        
        // Center the window
        [self.oscMixerWindow.window center];
        
        NSLog(@"🎛️ OSC Mixer interface launched successfully");
    } else {
        NSLog(@"❌ Failed to create OSC Mixer window");
        
        // Show error dialog
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Interface Error"];
        [alert setInformativeText:@"Failed to create the OSC Mixer interface. Please check your system configuration."];
        [alert addButtonWithTitle:@"OK"];
        [alert runModal];
        
        // Exit application
        [NSApp terminate:nil];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    NSLog(@"Shutting down CV to OSC Converter...");
    
    if (self.mixerEngine) {
        NSLog(@"Shutting down OSC Mixer Engine...");
        self.mixerEngine->shutdown();
        NSLog(@"✅ OSC Mixer Engine shutdown complete");
    }
    
    return NSTerminateNow;
}

- (void)dealloc {
    // Clean up mixer engine
    if (self.mixerEngine) {
        self.mixerEngine->shutdown();
    }
}

@end

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;
    
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        // Set application properties
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        CVToOSCAppDelegate *delegate = [[CVToOSCAppDelegate alloc] init];
        [app setDelegate:delegate];
        
        // Create menu bar
        NSMenu *menubar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:appMenuItem];
        [app setMainMenu:menubar];
        
        NSMenu *appMenu = [[NSMenu alloc] init];
        NSString *appName = @"CV to OSC Converter";
        NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", appName];
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
                                                              action:@selector(terminate:)
                                                       keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];
        
        // Add File menu
        NSMenuItem *fileMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:fileMenuItem];
        NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
        
        NSMenuItem *loadConfigItem = [[NSMenuItem alloc] initWithTitle:@"Load Configuration..."
                                                               action:nil
                                                        keyEquivalent:@"o"];
        [fileMenu addItem:loadConfigItem];
        
        NSMenuItem *saveConfigItem = [[NSMenuItem alloc] initWithTitle:@"Save Configuration..."
                                                               action:nil
                                                        keyEquivalent:@"s"];
        [fileMenu addItem:saveConfigItem];
        
        [fileMenuItem setSubmenu:fileMenu];
        
        // Add Window menu
        NSMenuItem *windowMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:windowMenuItem];
        NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        
        NSMenuItem *minimizeItem = [[NSMenuItem alloc] initWithTitle:@"Minimize"
                                                              action:@selector(performMiniaturize:)
                                                       keyEquivalent:@"m"];
        [windowMenu addItem:minimizeItem];
        
        [windowMenuItem setSubmenu:windowMenu];
        
        NSLog(@"Starting CV to OSC Converter application...");
        [app run];
    }
    
    return 0;
}
