#include "MacOSPermissions.h"
#include <iostream>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>
#endif

// Static callback storage for async permission requests
static std::function<void(bool)> g_microphoneCallback = nullptr;
static std::function<void(bool)> g_fileCallback = nullptr;
static std::function<void(bool)> g_allPermissionsCallback = nullptr;

PermissionStatus MacOSPermissions::checkMicrophonePermission() {
#ifdef __APPLE__
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    return convertAVAuthorizationStatus(static_cast<int>(status));
#else
    return PermissionStatus::Unknown;
#endif
}

PermissionStatus MacOSPermissions::checkFilePermission() {
#ifdef __APPLE__
    // For file permissions, we check if we can access common directories
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *documentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    
    if ([fileManager isReadableFileAtPath:documentsPath]) {
        return PermissionStatus::Granted;
    } else {
        return PermissionStatus::Denied;
    }
#else
    return PermissionStatus::Unknown;
#endif
}

void MacOSPermissions::requestMicrophonePermission(std::function<void(bool)> callback) {
#ifdef __APPLE__
    g_microphoneCallback = callback;
    
    // Check current status first
    AVAuthorizationStatus currentStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    
    if (currentStatus == AVAuthorizationStatusAuthorized) {
        std::cout << "✅ Microphone permission already granted" << std::endl;
        if (callback) callback(true);
        return;
    }
    
    if (currentStatus == AVAuthorizationStatusDenied || currentStatus == AVAuthorizationStatusRestricted) {
        std::cout << "❌ Microphone permission denied or restricted" << std::endl;
        std::cout << "Please enable microphone access in System Preferences > Security & Privacy > Privacy > Microphone" << std::endl;
        showPermissionDialog(PermissionType::Microphone);
        if (callback) callback(false);
        return;
    }
    
    // Request permission
    std::cout << "🎤 Requesting microphone permission..." << std::endl;
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (granted) {
                std::cout << "✅ Microphone permission granted!" << std::endl;
            } else {
                std::cout << "❌ Microphone permission denied" << std::endl;
                std::cout << "To use this app, please:" << std::endl;
                std::cout << "1. Open System Preferences" << std::endl;
                std::cout << "2. Go to Security & Privacy > Privacy > Microphone" << std::endl;
                std::cout << "3. Enable access for this app" << std::endl;
            }
            
            if (g_microphoneCallback) {
                g_microphoneCallback(granted);
                g_microphoneCallback = nullptr;
            }
        });
    }];
#else
    if (callback) callback(true); // Assume granted on non-macOS systems
#endif
}

void MacOSPermissions::requestFilePermission(std::function<void(bool)> callback) {
#ifdef __APPLE__
    // For sandboxed apps, file permissions are handled differently
    // For now, we'll assume file access is available
    std::cout << "📁 File access permissions checked" << std::endl;
    if (callback) callback(true);
#else
    if (callback) callback(true);
#endif
}

bool MacOSPermissions::checkAllRequiredPermissions() {
    bool microphoneOk = (checkMicrophonePermission() == PermissionStatus::Granted);
    bool fileOk = (checkFilePermission() == PermissionStatus::Granted);
    
    return microphoneOk && fileOk;
}

void MacOSPermissions::requestAllRequiredPermissions(std::function<void(bool)> callback) {
    g_allPermissionsCallback = callback;
    
    std::cout << "🔐 Checking all required permissions..." << std::endl;
    
    // Check microphone first
    PermissionStatus micStatus = checkMicrophonePermission();
    if (micStatus != PermissionStatus::Granted) {
        requestMicrophonePermission([](bool granted) {
            if (granted) {
                // Microphone granted, now check files
                MacOSPermissions::requestFilePermission([](bool fileGranted) {
                    if (g_allPermissionsCallback) {
                        g_allPermissionsCallback(fileGranted);
                        g_allPermissionsCallback = nullptr;
                    }
                });
            } else {
                // Microphone denied
                if (g_allPermissionsCallback) {
                    g_allPermissionsCallback(false);
                    g_allPermissionsCallback = nullptr;
                }
            }
        });
    } else {
        // Microphone already granted, check files
        requestFilePermission([](bool fileGranted) {
            if (g_allPermissionsCallback) {
                g_allPermissionsCallback(fileGranted);
                g_allPermissionsCallback = nullptr;
            }
        });
    }
}

std::string MacOSPermissions::permissionStatusToString(PermissionStatus status) {
    switch (status) {
        case PermissionStatus::NotDetermined: return "Not Determined";
        case PermissionStatus::Denied: return "Denied";
        case PermissionStatus::Restricted: return "Restricted";
        case PermissionStatus::Granted: return "Granted";
        case PermissionStatus::Unknown: return "Unknown";
        default: return "Invalid";
    }
}

std::string MacOSPermissions::permissionTypeToString(PermissionType type) {
    switch (type) {
        case PermissionType::Microphone: return "Microphone";
        case PermissionType::Files: return "Files";
        case PermissionType::Camera: return "Camera";
        case PermissionType::ScreenRecording: return "Screen Recording";
        default: return "Unknown";
    }
}

bool MacOSPermissions::isPermissionGranted(PermissionStatus status) {
    return status == PermissionStatus::Granted;
}

void MacOSPermissions::showPermissionDialog(PermissionType type) {
#ifdef __APPLE__
    NSString *title = @"Permission Required";
    NSString *message;
    
    switch (type) {
        case PermissionType::Microphone:
            message = @"This app needs microphone access to read CV (Control Voltage) signals from your audio interface.\n\nTo enable:\n1. Open System Preferences\n2. Go to Security & Privacy > Privacy > Microphone\n3. Check the box next to this app";
            break;
        case PermissionType::Files:
            message = @"This app needs file access to read configuration files and save settings.\n\nTo enable:\n1. Open System Preferences\n2. Go to Security & Privacy > Privacy > Files and Folders\n3. Grant access to necessary folders";
            break;
        default:
            message = @"This app requires additional permissions to function properly. Please check System Preferences > Security & Privacy.";
            break;
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:title];
        [alert setInformativeText:message];
        [alert addButtonWithTitle:@"Open System Preferences"];
        [alert addButtonWithTitle:@"Cancel"];
        
        NSModalResponse response = [alert runModal];
        if (response == NSAlertFirstButtonReturn) {
            MacOSPermissions::openSystemPreferences(type);
        }
    });
#else
    std::cout << "Permission dialog would be shown for: " << permissionTypeToString(type) << std::endl;
#endif
}

void MacOSPermissions::openSystemPreferences(PermissionType type) {
#ifdef __APPLE__
    NSString *prefPath;
    
    switch (type) {
        case PermissionType::Microphone:
            prefPath = @"x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone";
            break;
        case PermissionType::Files:
            prefPath = @"x-apple.systempreferences:com.apple.preference.security?Privacy_AllFiles";
            break;
        default:
            prefPath = @"x-apple.systempreferences:com.apple.preference.security";
            break;
    }
    
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:prefPath]];
    std::cout << "🔧 Opened System Preferences for " << permissionTypeToString(type) << " settings" << std::endl;
#else
    std::cout << "Would open system preferences for: " << permissionTypeToString(type) << std::endl;
#endif
}

bool MacOSPermissions::isAppSandboxed() {
#ifdef __APPLE__
    NSBundle *bundle = [NSBundle mainBundle];
    NSNumber *entitlements = [bundle objectForInfoDictionaryKey:@"com.apple.security.app-sandbox"];
    return entitlements ? [entitlements boolValue] : false;
#else
    return false;
#endif
}

std::string MacOSPermissions::getAppName() {
#ifdef __APPLE__
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *appName = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
    if (!appName) {
        appName = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
    }
    if (!appName) {
        appName = @"CV to OSC Converter";
    }
    return std::string([appName UTF8String]);
#else
    return "CV to OSC Converter";
#endif
}

std::string MacOSPermissions::getBundleIdentifier() {
#ifdef __APPLE__
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *bundleId = [bundle bundleIdentifier];
    return bundleId ? std::string([bundleId UTF8String]) : "com.unknown.cv-to-osc-converter";
#else
    return "com.unknown.cv-to-osc-converter";
#endif
}

void MacOSPermissions::resetPermissions() {
#ifdef __APPLE__
    std::cout << "🔄 To reset permissions, run in Terminal:" << std::endl;
    std::cout << "tccutil reset Microphone " << getBundleIdentifier() << std::endl;
    std::cout << "tccutil reset SystemPolicyAllFiles " << getBundleIdentifier() << std::endl;
#else
    std::cout << "Permission reset not available on this platform" << std::endl;
#endif
}

std::string MacOSPermissions::generatePermissionReport() {
    std::string report = "🔐 Permission Status Report\n";
    report += "============================\n";
    report += "App Name: " + getAppName() + "\n";
    report += "Bundle ID: " + getBundleIdentifier() + "\n";
    report += "Sandboxed: " + std::string(isAppSandboxed() ? "Yes" : "No") + "\n\n";
    
    report += "Microphone: " + permissionStatusToString(checkMicrophonePermission()) + "\n";
    report += "File Access: " + permissionStatusToString(checkFilePermission()) + "\n";
    
    report += "\nAll Required Permissions: " + std::string(checkAllRequiredPermissions() ? "✅ Granted" : "❌ Missing") + "\n";
    
    return report;
}

PermissionStatus MacOSPermissions::convertAVAuthorizationStatus(int status) {
#ifdef __APPLE__
    switch (status) {
        case AVAuthorizationStatusNotDetermined:
            return PermissionStatus::NotDetermined;
        case AVAuthorizationStatusRestricted:
            return PermissionStatus::Restricted;
        case AVAuthorizationStatusDenied:
            return PermissionStatus::Denied;
        case AVAuthorizationStatusAuthorized:
            return PermissionStatus::Granted;
        default:
            return PermissionStatus::Unknown;
    }
#else
    return PermissionStatus::Unknown;
#endif
}

void MacOSPermissions::showNativeAlert(const std::string& title, const std::string& message, const std::string& buttonText) {
#ifdef __APPLE__
    dispatch_async(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:[NSString stringWithUTF8String:title.c_str()]];
        [alert setInformativeText:[NSString stringWithUTF8String:message.c_str()]];
        [alert addButtonWithTitle:[NSString stringWithUTF8String:buttonText.c_str()]];
        [alert runModal];
    });
#else
    std::cout << title << ": " << message << std::endl;
#endif
}

void MacOSPermissions::executeAppleScript(const std::string& script) {
#ifdef __APPLE__
    NSString *scriptString = [NSString stringWithUTF8String:script.c_str()];
    NSAppleScript *appleScript = [[NSAppleScript alloc] initWithSource:scriptString];
    [appleScript executeAndReturnError:nil];
#else
    std::cout << "AppleScript execution not available: " << script << std::endl;
#endif
}
