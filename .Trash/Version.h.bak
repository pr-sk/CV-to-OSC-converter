#pragma once

#include <string>

// Version information - automatically updated by CI/CD
#define CV_TO_OSC_VERSION_MAJOR 1
#define CV_TO_OSC_VERSION_MINOR 6
#define CV_TO_OSC_VERSION_PATCH 1
#define CV_TO_OSC_VERSION_BUILD "release"

// Git information - populated by build system
#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif

#ifndef GIT_BRANCH
#define GIT_BRANCH "unknown"
#endif

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__ " " __TIME__
#endif

class Version {
public:
    // Get version string in semantic versioning format
    static std::string getVersion() {
        return std::to_string(CV_TO_OSC_VERSION_MAJOR) + "." +
               std::to_string(CV_TO_OSC_VERSION_MINOR) + "." +
               std::to_string(CV_TO_OSC_VERSION_PATCH);
    }
    
    // Get full version string with build info
    static std::string getFullVersion() {
        std::string version = getVersion();
        if (std::string(CV_TO_OSC_VERSION_BUILD) != "release") {
            version += "-" + std::string(CV_TO_OSC_VERSION_BUILD);
        }
        return version;
    }
    
    // Get version with git information
    static std::string getVersionWithGit() {
        return getFullVersion() + " (" + std::string(GIT_COMMIT_HASH).substr(0, 7) + ")";
    }
    
    // Get build information
    static std::string getBuildInfo() {
        return "Built on " + std::string(BUILD_DATE) + 
               " from " + std::string(GIT_BRANCH) + 
               " (" + std::string(GIT_COMMIT_HASH).substr(0, 7) + ")";
    }
    
    // Get major version
    static int getMajor() { return CV_TO_OSC_VERSION_MAJOR; }
    
    // Get minor version
    static int getMinor() { return CV_TO_OSC_VERSION_MINOR; }
    
    // Get patch version
    static int getPatch() { return CV_TO_OSC_VERSION_PATCH; }
    
    // Get build type
    static std::string getBuild() { return CV_TO_OSC_VERSION_BUILD; }
    
    // Check if this is a development build
    static bool isDevelopment() { 
        return std::string(CV_TO_OSC_VERSION_BUILD) != "release"; 
    }
    
    // Get user-friendly application title
    static std::string getAppTitle() {
        return "CV to OSC Converter v" + getFullVersion();
    }
};
