#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "Localization.h"

enum class TransliterationScheme {
    GOST_1983,      // GOST 16876-71 (Russian)
    BGN_PCGN,       // BGN/PCGN (Russian)
    ISO_9,          // ISO 9 (Russian)
    HEPBURN,        // Hepburn (Japanese)
    KUNREI,         // Kunrei-shiki (Japanese)
    PINYIN,         // Pinyin (Chinese)
    WADE_GILES,     // Wade-Giles (Chinese)
    CUSTOM          // User-defined mapping
};

struct TransliterationOptions {
    TransliterationScheme scheme = TransliterationScheme::GOST_1983;
    bool preserveCase = true;
    bool preserveSpaces = true;
    bool preservePunctuation = true;
    bool fallbackToOriginal = true;  // If character can't be transliterated, keep original
    std::string customSeparator = "";  // For compound transliterations
};

class Transliterator {
public:
    static Transliterator& getInstance() {
        static Transliterator instance;
        return instance;
    }
    
    // Main transliteration function
    std::string transliterate(const std::string& input, Language sourceLanguage, 
                             const TransliterationOptions& options = TransliterationOptions{}) const;
    
    // Auto-detect language and transliterate
    std::string autoTransliterate(const std::string& input, 
                                 const TransliterationOptions& options = TransliterationOptions{}) const;
    
    // Get available schemes for a language
    std::vector<TransliterationScheme> getAvailableSchemes(Language language) const;
    
    // Get scheme description
    std::string getSchemeDescription(TransliterationScheme scheme) const;
    
    // Language detection
    Language detectLanguage(const std::string& input) const;
    
    // Custom mapping management
    void addCustomMapping(Language language, const std::string& from, const std::string& to);
    void removeCustomMapping(Language language, const std::string& from);
    void clearCustomMappings(Language language);
    
    // Utility functions
    bool isTransliterationNeeded(const std::string& input) const;
    bool isLatinScript(const std::string& input) const;
    
    // Initialize with default mappings
    void initialize();
    
    // Batch processing
    std::vector<std::string> transliterateMultiple(const std::vector<std::string>& inputs,
                                                   Language sourceLanguage,
                                                   const TransliterationOptions& options = TransliterationOptions{}) const;

private:
    Transliterator() = default;
    ~Transliterator() = default;
    Transliterator(const Transliterator&) = delete;
    Transliterator& operator=(const Transliterator&) = delete;
    
    // Character mapping tables for different languages and schemes
    std::unordered_map<Language, std::unordered_map<TransliterationScheme, std::unordered_map<std::string, std::string>>> mappings_;
    
    // Custom user mappings
    std::unordered_map<Language, std::unordered_map<std::string, std::string>> customMappings_;
    
    // Language detection helpers
    std::unordered_map<Language, std::function<bool(const std::string&)>> languageDetectors_;
    
    // Initialize mapping tables
    void initializeRussianMappings();
    void initializeJapaneseMappings();
    void initializeChineseMappings();
    void initializeGermanMappings();
    void initializeFrenchMappings();
    void initializeItalianMappings();
    void initializeLanguageDetectors();
    
    // Helper functions
    std::string processText(const std::string& input, 
                           const std::unordered_map<std::string, std::string>& mapping,
                           const TransliterationOptions& options) const;
    
    std::string processCharacter(const std::string& character,
                                const std::unordered_map<std::string, std::string>& mapping,
                                const TransliterationOptions& options) const;
    
    bool isUpperCase(const std::string& character) const;
    std::string toLowerCase(const std::string& character) const;
    std::string toUpperCase(const std::string& character) const;
    
    // Unicode utilities
    std::vector<std::string> splitIntoCharacters(const std::string& input) const;
    bool isInUnicodeRange(uint32_t codepoint, uint32_t start, uint32_t end) const;
    uint32_t getFirstCodepoint(const std::string& character) const;
    
    // Language detection by Unicode ranges
    bool isCyrillic(const std::string& input) const;
    bool isHiragana(const std::string& input) const;
    bool isKatakana(const std::string& input) const;
    bool isKanji(const std::string& input) const;
    bool isCJK(const std::string& input) const;
};

// Convenience functions
std::string transliterateTo(const std::string& input, Language sourceLanguage = Language::English);
std::string autoTransliterate(const std::string& input);

// Helper macro for transliteration in UI
#define TRANSLITERATE(text, lang) Transliterator::getInstance().transliterate(text, lang)
#define AUTO_TRANSLITERATE(text) Transliterator::getInstance().autoTransliterate(text)
