#include "FileDialog.h"
#include <filesystem>
#include <iostream>

#ifdef __APPLE__
#include <AppKit/AppKit.h>
#elif defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#else
// Linux implementation will use zenity or kdialog as fallback
#include <cstdlib>
#include <fstream>
#endif

// Public interface implementations
std::string FileDialog::openFile(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath) {
#ifdef __APPLE__
    return openFileNative_macOS(title, filters, defaultPath);
#elif defined(_WIN32)
    return openFileNative_Windows(title, filters, defaultPath);
#else
    return openFileNative_Linux(title, filters, defaultPath);
#endif
}

std::string FileDialog::saveFile(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName) {
#ifdef __APPLE__
    return saveFileNative_macOS(title, filters, defaultPath, defaultName);
#elif defined(_WIN32)
    return saveFileNative_Windows(title, filters, defaultPath, defaultName);
#else
    return saveFileNative_Linux(title, filters, defaultPath, defaultName);
#endif
}

std::string FileDialog::selectFolder(const std::string& title, const std::string& defaultPath) {
#ifdef __APPLE__
    return selectFolderNative_macOS(title, defaultPath);
#elif defined(_WIN32)
    return selectFolderNative_Windows(title, defaultPath);
#else
    return selectFolderNative_Linux(title, defaultPath);
#endif
}

// Utility functions
std::string FileDialog::getDocumentsPath() {
    std::filesystem::path documentsPath;
    
#ifdef __APPLE__
    const char* home = getenv("HOME");
    if (home) {
        documentsPath = std::filesystem::path(home) / "Documents";
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path))) {
        documentsPath = path;
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        documentsPath = std::filesystem::path(home) / "Documents";
    }
#endif
    
    return documentsPath.string();
}

std::string FileDialog::getConfigPath() {
    std::filesystem::path configPath;
    
#ifdef __APPLE__
    const char* home = getenv("HOME");
    if (home) {
        configPath = std::filesystem::path(home) / "Library" / "Application Support" / "CV to OSC Converter";
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path))) {
        configPath = std::filesystem::path(path) / "CV to OSC Converter";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        configPath = std::filesystem::path(home) / ".config" / "cv-to-osc-converter";
    }
#endif
    
    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(configPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create config directory: " << e.what() << std::endl;
    }
    
    return configPath.string();
}

bool FileDialog::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string FileDialog::getExtension(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

std::string FileDialog::getFilename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string FileDialog::getDirectory(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

// Platform-specific implementations

#ifdef __APPLE__
std::string FileDialog::openFileNative_macOS(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        
        // Set default directory
        if (!defaultPath.empty()) {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath.c_str()]];
            [panel setDirectoryURL:url];
        }
        
        // Set file type filters
        if (!filters.empty()) {
            NSMutableArray* allowedTypes = [[NSMutableArray alloc] init];
            for (const auto& filter : filters) {
                // Extract extension from pattern (e.g., "*.json" -> "json")
                std::string ext = filter.pattern;
                if (ext.size() > 2 && ext.substr(0, 2) == "*.") {
                    ext = ext.substr(2);
                    [allowedTypes addObject:[NSString stringWithUTF8String:ext.c_str()]];
                }
            }
            [panel setAllowedFileTypes:allowedTypes];
        }
        
        NSModalResponse result = [panel runModal];
        if (result == NSModalResponseOK) {
            NSURL* url = [[panel URLs] objectAtIndex:0];
            return std::string([[url path] UTF8String]);
        }
        
        return "";
    }
}

std::string FileDialog::saveFileNative_macOS(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        
        // Set default directory
        if (!defaultPath.empty()) {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath.c_str()]];
            [panel setDirectoryURL:url];
        }
        
        // Set default filename
        if (!defaultName.empty()) {
            [panel setNameFieldStringValue:[NSString stringWithUTF8String:defaultName.c_str()]];
        }
        
        // Set file type filters
        if (!filters.empty()) {
            NSMutableArray* allowedTypes = [[NSMutableArray alloc] init];
            for (const auto& filter : filters) {
                std::string ext = filter.pattern;
                if (ext.size() > 2 && ext.substr(0, 2) == "*.") {
                    ext = ext.substr(2);
                    [allowedTypes addObject:[NSString stringWithUTF8String:ext.c_str()]];
                }
            }
            [panel setAllowedFileTypes:allowedTypes];
        }
        
        NSModalResponse result = [panel runModal];
        if (result == NSModalResponseOK) {
            NSURL* url = [panel URL];
            return std::string([[url path] UTF8String]);
        }
        
        return "";
    }
}

std::string FileDialog::selectFolderNative_macOS(const std::string& title, const std::string& defaultPath) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        
        if (!defaultPath.empty()) {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath.c_str()]];
            [panel setDirectoryURL:url];
        }
        
        NSModalResponse result = [panel runModal];
        if (result == NSModalResponseOK) {
            NSURL* url = [[panel URLs] objectAtIndex:0];
            return std::string([[url path] UTF8String]);
        }
        
        return "";
    }
}

#elif defined(_WIN32)

std::string FileDialog::openFileNative_Windows(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Build filter string
    std::string filterStr;
    for (const auto& filter : filters) {
        filterStr += filter.name + "\0" + filter.pattern + "\0";
    }
    filterStr += "\0";
    
    if (!filterStr.empty()) {
        ofn.lpstrFilter = filterStr.c_str();
    }
    
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    
    return "";
}

std::string FileDialog::saveFileNative_Windows(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    // Set default filename
    if (!defaultName.empty()) {
        strncpy_s(szFile, defaultName.c_str(), _TRUNCATE);
    }
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    // Build filter string
    std::string filterStr;
    for (const auto& filter : filters) {
        filterStr += filter.name + "\0" + filter.pattern + "\0";
    }
    filterStr += "\0";
    
    if (!filterStr.empty()) {
        ofn.lpstrFilter = filterStr.c_str();
    }
    
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    
    return "";
}

std::string FileDialog::selectFolderNative_Windows(const std::string& title, const std::string& defaultPath) {
    BROWSEINFOA bi = {0};
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != nullptr) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            CoTaskMemFree(pidl);
            return std::string(path);
        }
        CoTaskMemFree(pidl);
    }
    
    return "";
}

#else
// Linux implementations using zenity/kdialog

std::string FileDialog::openFileNative_Linux(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath) {
    std::string command = "zenity --file-selection --title=\"" + title + "\"";
    
    if (!defaultPath.empty()) {
        command += " --filename=\"" + defaultPath + "/\"";
    }
    
    // Add file filters
    for (const auto& filter : filters) {
        command += " --file-filter=\"" + filter.name + " | " + filter.pattern + "\"";
    }
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::string FileDialog::saveFileNative_Linux(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName) {
    std::string command = "zenity --file-selection --save --title=\"" + title + "\"";
    
    std::string fullPath = defaultPath;
    if (!defaultName.empty()) {
        if (!fullPath.empty() && fullPath.back() != '/') {
            fullPath += '/';
        }
        fullPath += defaultName;
    }
    
    if (!fullPath.empty()) {
        command += " --filename=\"" + fullPath + "\"";
    }
    
    // Add file filters
    for (const auto& filter : filters) {
        command += " --file-filter=\"" + filter.name + " | " + filter.pattern + "\"";
    }
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::string FileDialog::selectFolderNative_Linux(const std::string& title, const std::string& defaultPath) {
    std::string command = "zenity --file-selection --directory --title=\"" + title + "\"";
    
    if (!defaultPath.empty()) {
        command += " --filename=\"" + defaultPath + "/\"";
    }
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

#endif
