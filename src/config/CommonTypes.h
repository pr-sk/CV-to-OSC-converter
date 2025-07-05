#pragma once

/**
 * @brief Common types and enums used across the CV to OSC Converter application
 */

/**
 * @brief Permission status for platform-specific permission systems
 */
enum class PermissionStatus {
    Granted,
    Denied,
    NotDetermined,
    Restricted,
    Unknown
};

/**
 * @brief Convert PermissionStatus to human-readable string
 */
inline const char* permissionStatusToString(PermissionStatus status) {
    switch (status) {
        case PermissionStatus::Granted:
            return "Granted";
        case PermissionStatus::Denied:
            return "Denied";
        case PermissionStatus::NotDetermined:
            return "Not Determined";
        case PermissionStatus::Restricted:
            return "Restricted";
        case PermissionStatus::Unknown:
        default:
            return "Unknown";
    }
}
