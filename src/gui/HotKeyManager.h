#pragma once

#include "imgui.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

// Структура для описания горячей клавиши
struct HotKey {
    ImGuiKey key;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool cmd = false; // Command key на macOS
    std::function<void()> action;
    std::string description;
    std::string category;
    bool enabled = true;
    
    HotKey() = default;
    
    HotKey(ImGuiKey k, bool c = false, bool s = false, bool a = false, bool cm = false)
        : key(k), ctrl(c), shift(s), alt(a), cmd(cm) {}
    
    // Проверка нажатия комбинации
    bool isPressed() const {
        if (!enabled) return false;
        
        ImGuiIO& io = ImGui::GetIO();
        
        // Проверяем основную клавишу
        if (!ImGui::IsKeyPressed(key)) return false;
        
        // Проверяем модификаторы
#ifdef __APPLE__
        // На macOS используем Cmd вместо Ctrl для системных команд
        bool ctrlPressed = cmd ? io.KeySuper : (ctrl ? io.KeyCtrl : false);
        if (ctrl && !ctrlPressed) return false;
        if (cmd && !io.KeySuper) return false;
#else
        if (ctrl && !io.KeyCtrl) return false;
#endif
        
        if (shift && !io.KeyShift) return false;
        if (alt && !io.KeyAlt) return false;
        
        // Проверяем, что лишние модификаторы не нажаты
#ifdef __APPLE__
        if (!cmd && io.KeySuper) return false;
#endif
        if (!ctrl && io.KeyCtrl) return false;
        if (!shift && io.KeyShift) return false;
        if (!alt && io.KeyAlt) return false;
        
        return true;
    }
    
    // Получение строкового представления
    std::string toString() const {
        std::string result;
        
#ifdef __APPLE__
        if (cmd) result += "⌘";
        if (ctrl) result += "^";
        if (alt) result += "⌥";
        if (shift) result += "⇧";
#else
        if (ctrl) result += "Ctrl+";
        if (alt) result += "Alt+";
        if (shift) result += "Shift+";
#endif
        
        // Конвертируем ImGuiKey в строку
        result += keyToString(key);
        
        return result;
    }
    
private:
    std::string keyToString(ImGuiKey key) const {
        switch (key) {
            case ImGuiKey_A: return "A";
            case ImGuiKey_B: return "B";
            case ImGuiKey_C: return "C";
            case ImGuiKey_D: return "D";
            case ImGuiKey_E: return "E";
            case ImGuiKey_F: return "F";
            case ImGuiKey_G: return "G";
            case ImGuiKey_H: return "H";
            case ImGuiKey_I: return "I";
            case ImGuiKey_J: return "J";
            case ImGuiKey_K: return "K";
            case ImGuiKey_L: return "L";
            case ImGuiKey_M: return "M";
            case ImGuiKey_N: return "N";
            case ImGuiKey_O: return "O";
            case ImGuiKey_P: return "P";
            case ImGuiKey_Q: return "Q";
            case ImGuiKey_R: return "R";
            case ImGuiKey_S: return "S";
            case ImGuiKey_T: return "T";
            case ImGuiKey_U: return "U";
            case ImGuiKey_V: return "V";
            case ImGuiKey_W: return "W";
            case ImGuiKey_X: return "X";
            case ImGuiKey_Y: return "Y";
            case ImGuiKey_Z: return "Z";
            case ImGuiKey_0: return "0";
            case ImGuiKey_1: return "1";
            case ImGuiKey_2: return "2";
            case ImGuiKey_3: return "3";
            case ImGuiKey_4: return "4";
            case ImGuiKey_5: return "5";
            case ImGuiKey_6: return "6";
            case ImGuiKey_7: return "7";
            case ImGuiKey_8: return "8";
            case ImGuiKey_9: return "9";
            case ImGuiKey_Space: return "Space";
            case ImGuiKey_Enter: return "Enter";
            case ImGuiKey_Escape: return "Esc";
            case ImGuiKey_Tab: return "Tab";
            case ImGuiKey_Backspace: return "Backspace";
            case ImGuiKey_Delete: return "Delete";
            case ImGuiKey_F1: return "F1";
            case ImGuiKey_F2: return "F2";
            case ImGuiKey_F3: return "F3";
            case ImGuiKey_F4: return "F4";
            case ImGuiKey_F5: return "F5";
            case ImGuiKey_F6: return "F6";
            case ImGuiKey_F7: return "F7";
            case ImGuiKey_F8: return "F8";
            case ImGuiKey_F9: return "F9";
            case ImGuiKey_F10: return "F10";
            case ImGuiKey_F11: return "F11";
            case ImGuiKey_F12: return "F12";
            case ImGuiKey_LeftArrow: return "←";
            case ImGuiKey_RightArrow: return "→";
            case ImGuiKey_UpArrow: return "↑";
            case ImGuiKey_DownArrow: return "↓";
            default: return "Unknown";
        }
    }
};

// Менеджер горячих клавиш
class HotKeyManager {
private:
    std::map<std::string, HotKey> hotKeys_;
    bool enabled_ = true;
    
public:
    // Регистрация горячей клавиши
    void registerHotKey(const std::string& id, const HotKey& hotkey) {
        hotKeys_[id] = hotkey;
    }
    
    // Удаление горячей клавиши
    void unregisterHotKey(const std::string& id) {
        hotKeys_.erase(id);
    }
    
    // Проверка и выполнение горячих клавиш
    void processHotKeys() {
        if (!enabled_) return;
        
        ImGuiIO& io = ImGui::GetIO();
        
        // Не обрабатываем горячие клавиши, если фокус в текстовом поле
        if (io.WantTextInput) return;
        
        for (auto& [id, hotkey] : hotKeys_) {
            if (hotkey.isPressed() && hotkey.action) {
                hotkey.action();
                break; // Выполняем только одну команду за раз
            }
        }
    }
    
    // Включение/выключение обработки
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Получение всех зарегистрированных клавиш
    const std::map<std::string, HotKey>& getHotKeys() const { return hotKeys_; }
    
    // Получение клавиш по категории
    std::vector<std::pair<std::string, HotKey>> getHotKeysByCategory(const std::string& category) const {
        std::vector<std::pair<std::string, HotKey>> result;
        for (const auto& [id, hotkey] : hotKeys_) {
            if (hotkey.category == category) {
                result.emplace_back(id, hotkey);
            }
        }
        return result;
    }
    
    // Проверка конфликтов клавиш
    std::vector<std::string> findConflicts() const {
        std::vector<std::string> conflicts;
        // TODO: Implement conflict detection
        return conflicts;
    }
    
    // Экспорт/импорт настроек
    std::string exportSettings() const {
        // TODO: Implement JSON serialization
        return "";
    }
    
    void importSettings(const std::string& json) {
        // TODO: Implement JSON deserialization
    }
};

// Глобальный менеджер горячих клавиш
extern HotKeyManager* g_hotKeyManager;
