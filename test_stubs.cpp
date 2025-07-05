// Stub implementations for test_audio_input

#include <string>
#include <functional>

enum class PermissionStatus {
    Granted,
    Denied,
    NotDetermined
};

namespace MacOSPermissions {
    std::string getAppName() { return "TestAudioInput"; }
    bool isAppSandboxed() { return false; }
    bool checkFilePermission() { return true; }
    std::string getBundleIdentifier() { return "com.test.audioinput"; }
    std::string permissionStatusToString(PermissionStatus status) {
        switch(status) {
            case PermissionStatus::Granted: return "Granted";
            case PermissionStatus::Denied: return "Denied";
            case PermissionStatus::NotDetermined: return "Not Determined";
        }
        return "Unknown";
    }
    PermissionStatus checkMicrophonePermission() { return PermissionStatus::Granted; }
    void requestMicrophonePermission(std::function<void(bool)> callback) {
        if (callback) callback(true);
    }
}

class MidiDeviceHandler {
public:
    MidiDeviceHandler() {}
    ~MidiDeviceHandler() {}
};
