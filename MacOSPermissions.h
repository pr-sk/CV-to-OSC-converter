#pragma once

#include <string>
#include <functional>

#ifdef __APPLE__
// Only include Objective-C headers in .mm files
// Forward declarations for C++ headers
struct objc_object;
#endif

enum class PermissionStatus {
    NotDetermined,    // Permission hasn't been requested yet
    Denied,          // User explicitly denied permission
    Restricted,      // Permission restricted by system/parental controls
    Granted,         // Permission granted
    Unknown          // Status cannot be determined
};

enum class PermissionType {
    Microphone,
    Files,           // For reading config files, audio files, etc.
    Camera,          // If needed for future features
    ScreenRecording  // If needed for future GUI features
};

class MacOSPermissions {
public:
    // Static methods for easy access
    static PermissionStatus checkMicrophonePermission();
    static PermissionStatus checkFilePermission();
    static void requestMicrophonePermission(std::function<void(bool)> callback = nullptr);
    static void requestFilePermission(std::function<void(bool)> callback = nullptr);
    
    // Comprehensive permission check
    static bool checkAllRequiredPermissions();
    static void requestAllRequiredPermissions(std::function<void(bool)> callback = nullptr);
    
    // Utility methods
    static std::string permissionStatusToString(PermissionStatus status);
    static std::string permissionTypeToString(PermissionType type);
    static bool isPermissionGranted(PermissionStatus status);
    
    // User guidance
    static void showPermissionDialog(PermissionType type);
    static void openSystemPreferences(PermissionType type);
    
    // App Store / Distribution helpers
    static bool isAppSandboxed();
    static std::string getAppName();
    static std::string getBundleIdentifier();
    
    // Recovery and troubleshooting
    static void resetPermissions(); // For debugging/development
    static std::string generatePermissionReport();
    
private:
    // Internal helpers
    static PermissionStatus convertAVAuthorizationStatus(int status);
    static void showNativeAlert(const std::string& title, const std::string& message, const std::string& buttonText = "OK");
    static void executeAppleScript(const std::string& script);
};

// Convenience macros for permission checking
#define CHECK_MICROPHONE_PERMISSION() (MacOSPermissions::checkMicrophonePermission() == PermissionStatus::Granted)
#define CHECK_FILE_PERMISSION() (MacOSPermissions::checkFilePermission() == PermissionStatus::Granted)
#define REQUEST_MICROPHONE_IF_NEEDED() \
    if (MacOSPermissions::checkMicrophonePermission() != PermissionStatus::Granted) { \
        MacOSPermissions::requestMicrophonePermission(); \
    }

// Permission error codes for better error handling
namespace PermissionErrors {
    const int MICROPHONE_DENIED = 1001;
    const int FILE_ACCESS_DENIED = 1002;
    const int PERMISSION_RESTRICTED = 1003;
    const int SYSTEM_ERROR = 1004;
}
