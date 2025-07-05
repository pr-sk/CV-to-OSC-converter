#pragma once

#include <string>
#include <unordered_map>
#include <vector>

enum class Language {
    English = 0,
    Russian,
    Japanese,
    ChineseSimplified,
    German,
    French,
    Italian,
    COUNT
};

class Localization {
public:
    static Localization& getInstance() {
        static Localization instance;
        return instance;
    }
    
    // Language management
    void setLanguage(Language lang);
    Language getCurrentLanguage() const { return currentLanguage_; }
    
    // Text retrieval
    const std::string& getText(const std::string& key) const;
    const char* getTextC(const std::string& key) const;
    
    // Language info
    std::vector<std::pair<Language, std::string>> getAvailableLanguages() const;
    std::string getLanguageName(Language lang) const;
    std::string getLanguageNativeName(Language lang) const;
    
    // Font support
    bool needsCustomFont(Language lang) const;
    const char* getFontPath(Language lang) const;
    
    // Initialize with default texts
    void initialize();
    
private:
    Localization() = default;
    ~Localization() = default;
    Localization(const Localization&) = delete;
    Localization& operator=(const Localization&) = delete;
    
    Language currentLanguage_ = Language::English;
    std::unordered_map<std::string, std::unordered_map<Language, std::string>> translations_;
    
    void initializeTranslations();
    void addTranslation(const std::string& key, const std::unordered_map<Language, std::string>& translations);
};

// Convenience macro for text retrieval
#define _(key) Localization::getInstance().getTextC(key)
#define TEXT(key) Localization::getInstance().getText(key)
