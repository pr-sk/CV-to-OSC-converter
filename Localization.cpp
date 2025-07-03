#include "Localization.h"
#include <iostream>

void Localization::setLanguage(Language lang) {
    if (lang >= Language::COUNT) {
        std::cerr << "Invalid language selection" << std::endl;
        return;
    }
    currentLanguage_ = lang;
}

const std::string& Localization::getText(const std::string& key) const {
    auto it = translations_.find(key);
    if (it != translations_.end()) {
        auto langIt = it->second.find(currentLanguage_);
        if (langIt != it->second.end()) {
            return langIt->second;
        }
        // Fallback to English
        auto englishIt = it->second.find(Language::English);
        if (englishIt != it->second.end()) {
            return englishIt->second;
        }
    }
    
    static std::string missing = "[MISSING: " + key + "]";
    return missing;
}

const char* Localization::getTextC(const std::string& key) const {
    return getText(key).c_str();
}

std::vector<std::pair<Language, std::string>> Localization::getAvailableLanguages() const {
    return {
        {Language::English, "English"},
        {Language::Russian, "Русский"},
        {Language::Japanese, "日本語"},
        {Language::ChineseSimplified, "简体中文"},
        {Language::German, "Deutsch"},
        {Language::French, "Français"},
        {Language::Italian, "Italiano"}
    };
}

std::string Localization::getLanguageName(Language lang) const {
    switch (lang) {
        case Language::English: return "English";
        case Language::Russian: return "Russian";
        case Language::Japanese: return "Japanese";
        case Language::ChineseSimplified: return "Chinese (Simplified)";
        case Language::German: return "German";
        case Language::French: return "French";
        case Language::Italian: return "Italian";
        default: return "Unknown";
    }
}

std::string Localization::getLanguageNativeName(Language lang) const {
    switch (lang) {
        case Language::English: return "English";
        case Language::Russian: return "Русский";
        case Language::Japanese: return "日本語";
        case Language::ChineseSimplified: return "简体中文";
        case Language::German: return "Deutsch";
        case Language::French: return "Français";
        case Language::Italian: return "Italiano";
        default: return "Unknown";
    }
}

bool Localization::needsCustomFont(Language lang) const {
    return lang == Language::Japanese || 
           lang == Language::ChineseSimplified || 
           lang == Language::Russian;
}

const char* Localization::getFontPath(Language lang) const {
    switch (lang) {
        case Language::Japanese:
        case Language::ChineseSimplified:
            return "/System/Library/Fonts/PingFang.ttc"; // macOS CJK font
        case Language::Russian:
            return "/System/Library/Fonts/Helvetica.ttc"; // Helvetica supports Cyrillic
        default:
            return nullptr;
    }
}

void Localization::initialize() {
    initializeTranslations();
}

void Localization::addTranslation(const std::string& key, const std::unordered_map<Language, std::string>& translations) {
    translations_[key] = translations;
}

void Localization::initializeTranslations() {
    // Main Menu
    addTranslation("menu.file", {
        {Language::English, "File"},
        {Language::Russian, "Файл"},
        {Language::Japanese, "ファイル"},
        {Language::ChineseSimplified, "文件"},
        {Language::German, "Datei"},
        {Language::French, "Fichier"},
        {Language::Italian, "File"}
    });
    
    addTranslation("menu.edit", {
        {Language::English, "Edit"},
        {Language::Russian, "Правка"},
        {Language::Japanese, "編集"},
        {Language::ChineseSimplified, "编辑"},
        {Language::German, "Bearbeiten"},
        {Language::French, "Édition"},
        {Language::Italian, "Modifica"}
    });
    
    addTranslation("menu.view", {
        {Language::English, "View"},
        {Language::Russian, "Вид"},
        {Language::Japanese, "表示"},
        {Language::ChineseSimplified, "查看"},
        {Language::German, "Ansicht"},
        {Language::French, "Affichage"},
        {Language::Italian, "Visualizza"}
    });
    
    addTranslation("menu.settings", {
        {Language::English, "Settings"},
        {Language::Russian, "Настройки"},
        {Language::Japanese, "設定"},
        {Language::ChineseSimplified, "设置"},
        {Language::German, "Einstellungen"},
        {Language::French, "Paramètres"},
        {Language::Italian, "Impostazioni"}
    });
    
    addTranslation("menu.help", {
        {Language::English, "Help"},
        {Language::Russian, "Справка"},
        {Language::Japanese, "ヘルプ"},
        {Language::ChineseSimplified, "帮助"},
        {Language::German, "Hilfe"},
        {Language::French, "Aide"},
        {Language::Italian, "Aiuto"}
    });
    
    // Settings submenu
    addTranslation("menu.language", {
        {Language::English, "Language"},
        {Language::Russian, "Язык"},
        {Language::Japanese, "言語"},
        {Language::ChineseSimplified, "语言"},
        {Language::German, "Sprache"},
        {Language::French, "Langue"},
        {Language::Italian, "Lingua"}
    });
    
    // Window titles
    addTranslation("window.main", {
        {Language::English, "CV to OSC Converter"},
        {Language::Russian, "Конвертер CV в OSC"},
        {Language::Japanese, "CV to OSC コンバーター"},
        {Language::ChineseSimplified, "CV转OSC转换器"},
        {Language::German, "CV zu OSC Konverter"},
        {Language::French, "Convertisseur CV vers OSC"},
        {Language::Italian, "Convertitore CV a OSC"}
    });
    
    addTranslation("window.channels", {
        {Language::English, "Channel Configuration"},
        {Language::Russian, "Настройка каналов"},
        {Language::Japanese, "チャンネル設定"},
        {Language::ChineseSimplified, "通道配置"},
        {Language::German, "Kanal-Konfiguration"},
        {Language::French, "Configuration des canaux"},
        {Language::Italian, "Configurazione canali"}
    });
    
    addTranslation("window.osc", {
        {Language::English, "OSC Configuration"},
        {Language::Russian, "Настройка OSC"},
        {Language::Japanese, "OSC設定"},
        {Language::ChineseSimplified, "OSC配置"},
        {Language::German, "OSC-Konfiguration"},
        {Language::French, "Configuration OSC"},
        {Language::Italian, "Configurazione OSC"}
    });
    
    addTranslation("window.audio", {
        {Language::English, "Audio Configuration"},
        {Language::Russian, "Настройка аудио"},
        {Language::Japanese, "オーディオ設定"},
        {Language::ChineseSimplified, "音频配置"},
        {Language::German, "Audio-Konfiguration"},
        {Language::French, "Configuration audio"},
        {Language::Italian, "Configurazione audio"}
    });
    
    addTranslation("window.performance", {
        {Language::English, "Performance Monitor"},
        {Language::Russian, "Монитор производительности"},
        {Language::Japanese, "パフォーマンスモニター"},
        {Language::ChineseSimplified, "性能监视器"},
        {Language::German, "Leistungsmonitor"},
        {Language::French, "Moniteur de performance"},
        {Language::Italian, "Monitor prestazioni"}
    });
    
    // Common buttons
    addTranslation("button.start", {
        {Language::English, "Start"},
        {Language::Russian, "Запуск"},
        {Language::Japanese, "開始"},
        {Language::ChineseSimplified, "开始"},
        {Language::German, "Start"},
        {Language::French, "Démarrer"},
        {Language::Italian, "Avvia"}
    });
    
    addTranslation("button.stop", {
        {Language::English, "Stop"},
        {Language::Russian, "Стоп"},
        {Language::Japanese, "停止"},
        {Language::ChineseSimplified, "停止"},
        {Language::German, "Stopp"},
        {Language::French, "Arrêter"},
        {Language::Italian, "Ferma"}
    });
    
    addTranslation("button.ok", {
        {Language::English, "OK"},
        {Language::Russian, "ОК"},
        {Language::Japanese, "OK"},
        {Language::ChineseSimplified, "确定"},
        {Language::German, "OK"},
        {Language::French, "OK"},
        {Language::Italian, "OK"}
    });
    
    addTranslation("button.cancel", {
        {Language::English, "Cancel"},
        {Language::Russian, "Отмена"},
        {Language::Japanese, "キャンセル"},
        {Language::ChineseSimplified, "取消"},
        {Language::German, "Abbrechen"},
        {Language::French, "Annuler"},
        {Language::Italian, "Annulla"}
    });
    
    addTranslation("button.apply", {
        {Language::English, "Apply"},
        {Language::Russian, "Применить"},
        {Language::Japanese, "適用"},
        {Language::ChineseSimplified, "应用"},
        {Language::German, "Anwenden"},
        {Language::French, "Appliquer"},
        {Language::Italian, "Applica"}
    });
    
    // Audio configuration
    addTranslation("audio.device", {
        {Language::English, "Audio Device"},
        {Language::Russian, "Аудио устройство"},
        {Language::Japanese, "オーディオデバイス"},
        {Language::ChineseSimplified, "音频设备"},
        {Language::German, "Audio-Gerät"},
        {Language::French, "Périphérique audio"},
        {Language::Italian, "Dispositivo audio"}
    });
    
    addTranslation("audio.current_device", {
        {Language::English, "Current Device"},
        {Language::Russian, "Текущее устройство"},
        {Language::Japanese, "現在のデバイス"},
        {Language::ChineseSimplified, "当前设备"},
        {Language::German, "Aktuelles Gerät"},
        {Language::French, "Périphérique actuel"},
        {Language::Italian, "Dispositivo corrente"}
    });
    
    addTranslation("audio.sample_rate", {
        {Language::English, "Sample Rate"},
        {Language::Russian, "Частота дискретизации"},
        {Language::Japanese, "サンプルレート"},
        {Language::ChineseSimplified, "采样率"},
        {Language::German, "Abtastrate"},
        {Language::French, "Taux d'échantillonnage"},
        {Language::Italian, "Frequenza di campionamento"}
    });
    
    // OSC configuration
    addTranslation("osc.host", {
        {Language::English, "Host"},
        {Language::Russian, "Хост"},
        {Language::Japanese, "ホスト"},
        {Language::ChineseSimplified, "主机"},
        {Language::German, "Host"},
        {Language::French, "Hôte"},
        {Language::Italian, "Host"}
    });
    
    addTranslation("osc.port", {
        {Language::English, "Port"},
        {Language::Russian, "Порт"},
        {Language::Japanese, "ポート"},
        {Language::ChineseSimplified, "端口"},
        {Language::German, "Port"},
        {Language::French, "Port"},
        {Language::Italian, "Porta"}
    });
    
    addTranslation("osc.connected", {
        {Language::English, "Connected"},
        {Language::Russian, "Подключено"},
        {Language::Japanese, "接続済み"},
        {Language::ChineseSimplified, "已连接"},
        {Language::German, "Verbunden"},
        {Language::French, "Connecté"},
        {Language::Italian, "Connesso"}
    });
    
    addTranslation("osc.disconnected", {
        {Language::English, "Disconnected"},
        {Language::Russian, "Отключено"},
        {Language::Japanese, "未接続"},
        {Language::ChineseSimplified, "未连接"},
        {Language::German, "Getrennt"},
        {Language::French, "Déconnecté"},
        {Language::Italian, "Disconnesso"}
    });
    
    // Channel configuration
    addTranslation("channel.name", {
        {Language::English, "Channel Name"},
        {Language::Russian, "Название канала"},
        {Language::Japanese, "チャンネル名"},
        {Language::ChineseSimplified, "通道名称"},
        {Language::German, "Kanalname"},
        {Language::French, "Nom du canal"},
        {Language::Italian, "Nome canale"}
    });
    
    addTranslation("channel.enabled", {
        {Language::English, "Enabled"},
        {Language::Russian, "Включено"},
        {Language::Japanese, "有効"},
        {Language::ChineseSimplified, "启用"},
        {Language::German, "Aktiviert"},
        {Language::French, "Activé"},
        {Language::Italian, "Abilitato"}
    });
    
    addTranslation("channel.range_min", {
        {Language::English, "Range Min"},
        {Language::Russian, "Мин. значение"},
        {Language::Japanese, "範囲最小"},
        {Language::ChineseSimplified, "范围最小值"},
        {Language::German, "Bereich Min"},
        {Language::French, "Plage Min"},
        {Language::Italian, "Range Min"}
    });
    
    addTranslation("channel.range_max", {
        {Language::English, "Range Max"},
        {Language::Russian, "Макс. значение"},
        {Language::Japanese, "範囲最大"},
        {Language::ChineseSimplified, "范围最大值"},
        {Language::German, "Bereich Max"},
        {Language::French, "Plage Max"},
        {Language::Italian, "Range Max"}
    });
    
    addTranslation("channel.osc_address", {
        {Language::English, "OSC Address"},
        {Language::Russian, "OSC адрес"},
        {Language::Japanese, "OSCアドレス"},
        {Language::ChineseSimplified, "OSC地址"},
        {Language::German, "OSC-Adresse"},
        {Language::French, "Adresse OSC"},
        {Language::Italian, "Indirizzo OSC"}
    });
    
    // Performance monitor
    addTranslation("performance.fps", {
        {Language::English, "FPS"},
        {Language::Russian, "FPS"},
        {Language::Japanese, "FPS"},
        {Language::ChineseSimplified, "FPS"},
        {Language::German, "FPS"},
        {Language::French, "FPS"},
        {Language::Italian, "FPS"}
    });
    
    addTranslation("performance.cpu", {
        {Language::English, "CPU Usage"},
        {Language::Russian, "Использование ЦП"},
        {Language::Japanese, "CPU使用率"},
        {Language::ChineseSimplified, "CPU使用率"},
        {Language::German, "CPU-Auslastung"},
        {Language::French, "Utilisation CPU"},
        {Language::Italian, "Utilizzo CPU"}
    });
    
    // Status messages
    addTranslation("status.running", {
        {Language::English, "Running"},
        {Language::Russian, "Работает"},
        {Language::Japanese, "実行中"},
        {Language::ChineseSimplified, "运行中"},
        {Language::German, "Läuft"},
        {Language::French, "En cours"},
        {Language::Italian, "In esecuzione"}
    });
    
    addTranslation("status.stopped", {
        {Language::English, "Stopped"},
        {Language::Russian, "Остановлено"},
        {Language::Japanese, "停止中"},
        {Language::ChineseSimplified, "已停止"},
        {Language::German, "Gestoppt"},
        {Language::French, "Arrêté"},
        {Language::Italian, "Fermato"}
    });
}
