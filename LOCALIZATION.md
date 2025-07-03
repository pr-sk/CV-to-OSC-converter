# Localization System

This document describes the localization system implemented in the CV to OSC Converter application.

## Supported Languages

The application supports the following languages:

- **English** (Default)
- **Русский** (Russian)
- **日本語** (Japanese)
- **简体中文** (Chinese Simplified)
- **Deutsch** (German)
- **Français** (French)
- **Italiano** (Italian)

## Changing Language

### In GUI Application

1. Open the application
2. Go to **Settings** menu (Настройки/設定/设置/Einstellungen/Paramètres/Impostazioni)
3. Select **Language** (Язык/言語/语言/Sprache/Langue/Lingua)
4. Choose your preferred language from the list
5. The interface will immediately update to the selected language

The language preference is automatically saved in the configuration file.

### In Configuration File

You can also change the language by editing the `config.json` file:

```json
{
  "active_profile": "default",
  "profiles": {
    "default": {
      "language": 1
    }
  }
}
```

Language codes:
- `0` - English
- `1` - Russian
- `2` - Japanese
- `3` - Chinese Simplified
- `4` - German
- `5` - French
- `6` - Italian

## Font Support

The application automatically handles font selection for different languages:

- **Latin scripts** (English, German, French, Italian): Default system font
- **Cyrillic script** (Russian): Helvetica with Cyrillic support
- **CJK scripts** (Japanese, Chinese): PingFang font on macOS

## Implementation Details

### For Developers

#### Adding New Translations

To add a new text string for translation:

1. Open `Localization.cpp`
2. Add a new translation entry in the `initializeTranslations()` function:

```cpp
addTranslation("your.key", {
    {Language::English, "Your English Text"},
    {Language::Russian, "Ваш русский текст"},
    {Language::Japanese, "あなたの日本語テキスト"},
    {Language::ChineseSimplified, "您的中文文本"},
    {Language::German, "Ihr deutscher Text"},
    {Language::French, "Votre texte français"},
    {Language::Italian, "Il tuo testo italiano"}
});
```

#### Using Translations in Code

In the code, use the `_()` macro for simple text or `TEXT()` for std::string:

```cpp
// For ImGui labels (returns const char*)
ImGui::Text(_("your.key"));
ImGui::Button(_("button.start"));

// For std::string operations
std::string title = TEXT("window.main");
```

#### Adding New Languages

To add a new language:

1. Add the language to the `Language` enum in `Localization.h`
2. Update the `getAvailableLanguages()` function
3. Add cases in `getLanguageName()` and `getLanguageNativeName()`
4. Add font support if needed in `needsCustomFont()` and `getFontPath()`
5. Add translations for all existing keys

### File Structure

- `Localization.h` - Header file with interface
- `Localization.cpp` - Implementation with all translations
- Translations are stored in memory as hash maps for fast lookup

### Error Handling

- Missing translations fall back to English
- Missing English translations show `[MISSING: key]`
- Invalid language selection defaults to English

## Testing

To test the localization system:

```bash
# Compile and run the localization test
g++ -std=c++17 -o test_localization test_localization.cpp Localization.cpp
./test_localization
```

This will display all translations for all supported languages.

## Future Improvements

- **External translation files**: Move translations to JSON files for easier editing
- **Pluralization support**: Handle singular/plural forms
- **RTL language support**: Support for right-to-left languages
- **Dynamic translation loading**: Hot-reload translations without restart
- **Translation validation**: Tools to check for missing translations

## Contributing Translations

If you would like to contribute translations for a new language:

1. Create an issue on GitHub specifying the language
2. Provide native language name and ISO 639-1 code
3. We'll provide the list of strings to translate
4. Submit translations as a pull request

For corrections to existing translations, please create an issue with the specific text that needs correction.
