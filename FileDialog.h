#pragma once

#include <string>
#include <vector>

/**
 * Cross-platform file dialog utilities
 * Provides native file dialogs for opening and saving files
 */
class FileDialog {
public:
    struct Filter {
        std::string name;        // "Configuration Files"
        std::string pattern;     // "*.json"
        
        Filter(const std::string& n, const std::string& p) : name(n), pattern(p) {}
    };
    
    /**
     * Show open file dialog
     * @param title Dialog title
     * @param filters File type filters
     * @param defaultPath Default directory path
     * @return Selected file path, empty if cancelled
     */
    static std::string openFile(
        const std::string& title = "Open File",
        const std::vector<Filter>& filters = {},
        const std::string& defaultPath = ""
    );
    
    /**
     * Show save file dialog  
     * @param title Dialog title
     * @param filters File type filters
     * @param defaultPath Default directory path
     * @param defaultName Default file name
     * @return Selected file path, empty if cancelled
     */
    static std::string saveFile(
        const std::string& title = "Save File",
        const std::vector<Filter>& filters = {},
        const std::string& defaultPath = "",
        const std::string& defaultName = ""
    );
    
    /**
     * Show folder selection dialog
     * @param title Dialog title
     * @param defaultPath Default directory path
     * @return Selected folder path, empty if cancelled
     */
    static std::string selectFolder(
        const std::string& title = "Select Folder",
        const std::string& defaultPath = ""
    );
    
    /**
     * Get the user's Documents directory
     */
    static std::string getDocumentsPath();
    
    /**
     * Get the application's config directory
     */
    static std::string getConfigPath();
    
    /**
     * Check if a file exists
     */
    static bool fileExists(const std::string& path);
    
    /**
     * Get file extension
     */
    static std::string getExtension(const std::string& path);
    
    /**
     * Extract filename from path
     */
    static std::string getFilename(const std::string& path);
    
    /**
     * Extract directory from path
     */
    static std::string getDirectory(const std::string& path);

private:
    // Platform-specific implementations
#ifdef __APPLE__
    static std::string openFileNative_macOS(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath);
    static std::string saveFileNative_macOS(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName);
    static std::string selectFolderNative_macOS(const std::string& title, const std::string& defaultPath);
#elif defined(_WIN32)
    static std::string openFileNative_Windows(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath);
    static std::string saveFileNative_Windows(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName);
    static std::string selectFolderNative_Windows(const std::string& title, const std::string& defaultPath);
#else
    // Linux/GTK implementation
    static std::string openFileNative_Linux(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath);
    static std::string saveFileNative_Linux(const std::string& title, const std::vector<Filter>& filters, const std::string& defaultPath, const std::string& defaultName);
    static std::string selectFolderNative_Linux(const std::string& title, const std::string& defaultPath);
#endif
};
