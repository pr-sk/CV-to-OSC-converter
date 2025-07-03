#pragma once

#include "imgui.h"
#include "HotKeyManager.h"
#include "FileDialog.h"
#include <map>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

class HotKeyEditor {
public:
    static void ShowHotKeyEditor(HotKeyManager* manager, bool* p_open = nullptr) {
        if (!manager || !ImGui::Begin("Hot Key Editor", p_open)) {
            if (manager) ImGui::End();
            return;
        }

        ImGui::Text("Keyboard Shortcuts Configuration");
        ImGui::Separator();

        // Информация о системе отмены
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "💡 Tip:");
        ImGui::SameLine();
        ImGui::Text("Use Ctrl+Z to undo, Ctrl+Y or Ctrl+Shift+Z to redo");
        ImGui::Spacing();

        // Группировка по категориям
        const std::vector<std::string> categories = {
            "File", "Edit", "Control", "Windows", "Channels", "Appearance", "Application", "Help"
        };

        for (const auto& category : categories) {
            auto hotkeys = manager->getHotKeysByCategory(category);
            if (hotkeys.empty()) continue;

            if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();

                // Таблица горячих клавиш для данной категории
                if (ImGui::BeginTable(("HotKeys_" + category).c_str(), 4, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    
                    ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableHeadersRow();

                    for (const auto& [id, hotkey] : hotkeys) {
                        ImGui::TableNextRow();
                        
                        // Shortcut
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", hotkey.toString().c_str());
                        
                        // Command description
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", hotkey.description.c_str());
                        
                        // Status
                        ImGui::TableSetColumnIndex(2);
                        if (hotkey.enabled) {
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ON");
                        } else {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OFF");
                        }
                        
                        // Test button
                        ImGui::TableSetColumnIndex(3);
                        ImGui::PushID(id.c_str());
                        if (ImGui::Button("Test") && hotkey.action) {
                            hotkey.action();
                        }
                        ImGui::PopID();
                    }
                    
                    ImGui::EndTable();
                }
                
                ImGui::Unindent();
                ImGui::Spacing();
            }
        }

        // Статистика и настройки
        ImGui::Separator();
        ImGui::Text("Statistics");
        ImGui::Text("Total shortcuts: %zu", manager->getHotKeys().size());
        
        bool enabled = manager->isEnabled();
        if (ImGui::Checkbox("Enable Hot Keys", &enabled)) {
            manager->setEnabled(enabled);
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Disable to prevent accidental shortcuts during text input");
        }

        // Conflict detection
        auto conflicts = manager->findConflicts();
        if (!conflicts.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Conflicts detected:");
            for (const auto& conflict : conflicts) {
                ImGui::BulletText("%s", conflict.c_str());
            }
        }

        // Действия
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Reset to Defaults")) {
            ImGui::OpenPopup("Reset Confirmation");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Export Settings")) {
            exportHotKeySettings(manager);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Import Settings")) {
            importHotKeySettings(manager);
        }

        // Confirmation popup
        if (ImGui::BeginPopupModal("Reset Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to reset all shortcuts to defaults?");
            ImGui::Text("This action cannot be undone.");
            ImGui::Separator();

            if (ImGui::Button("Yes, Reset", ImVec2(120, 0))) {
                // TODO: Reset shortcuts
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Справочная информация
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Quick Reference");
        ImGui::BulletText("Space: Start/Stop conversion");
        ImGui::BulletText("Ctrl+Z: Undo last action");
        ImGui::BulletText("Ctrl+Y: Redo action");
        ImGui::BulletText("Ctrl+S: Save configuration");
        ImGui::BulletText("Ctrl+O: Load configuration");
        ImGui::BulletText("F1-F8: Toggle channels 1-8");
        ImGui::BulletText("Ctrl+1-5: Toggle windows");
        ImGui::BulletText("Ctrl+Q: Exit application");

        ImGui::End();
    }

private:
    static void exportHotKeySettings(HotKeyManager* manager) {
        if (!manager) return;
        
        std::string filePath = FileDialog::saveFile(
            "Export Hot Key Settings",
            {{"Hot Key Settings", "*.json"}, {"All Files", "*.*"}},
            FileDialog::getConfigPath(),
            "hotkeys.json"
        );
        
        if (filePath.empty()) return;
        
        try {
            nlohmann::json j;
            j["version"] = "1.0";
            j["exported_at"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            
            auto hotkeys = manager->getHotKeys();
            for (const auto& [id, hotkey] : hotkeys) {
                nlohmann::json hotkeyJson;
                hotkeyJson["key"] = static_cast<int>(hotkey.key);
                hotkeyJson["ctrl"] = hotkey.ctrl;
                hotkeyJson["shift"] = hotkey.shift;
                hotkeyJson["alt"] = hotkey.alt;
                hotkeyJson["enabled"] = hotkey.enabled;
                hotkeyJson["description"] = hotkey.description;
                hotkeyJson["category"] = hotkey.category;
                
                j["hotkeys"][id] = hotkeyJson;
            }
            
            std::ofstream file(filePath);
            file << j.dump(4);
            file.close();
            
            // Success feedback could be added here
            
        } catch (const std::exception& e) {
            // Error handling could be added here
        }
    }
    
    static void importHotKeySettings(HotKeyManager* manager) {
        if (!manager) return;
        
        std::string filePath = FileDialog::openFile(
            "Import Hot Key Settings",
            {{"Hot Key Settings", "*.json"}, {"All Files", "*.*"}},
            FileDialog::getConfigPath()
        );
        
        if (filePath.empty()) return;
        
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) return;
            
            nlohmann::json j;
            file >> j;
            file.close();
            
            if (!j.contains("hotkeys")) return;
            
            // Import each hotkey
            for (const auto& [id, hotkeyJson] : j["hotkeys"].items()) {
                if (hotkeyJson.contains("key") && 
                    hotkeyJson.contains("ctrl") && 
                    hotkeyJson.contains("shift") && 
                    hotkeyJson.contains("alt")) {
                    
                    HotKey hotkey;
                    hotkey.key = static_cast<ImGuiKey>(hotkeyJson["key"].get<int>());
                    hotkey.ctrl = hotkeyJson["ctrl"].get<bool>();
                    hotkey.shift = hotkeyJson["shift"].get<bool>();
                    hotkey.alt = hotkeyJson["alt"].get<bool>();
                    
                    if (hotkeyJson.contains("enabled")) {
                        hotkey.enabled = hotkeyJson["enabled"].get<bool>();
                    }
                    
                    if (hotkeyJson.contains("description")) {
                        hotkey.description = hotkeyJson["description"].get<std::string>();
                    }
                    
                    if (hotkeyJson.contains("category")) {
                        hotkey.category = hotkeyJson["category"].get<std::string>();
                    }
                    
                    // Note: action cannot be imported as it's a function pointer
                    // The application would need to re-register actions after import
                    
                    manager->registerHotKey(id, hotkey);
                }
            }
            
            // Success feedback could be added here
            
        } catch (const std::exception& e) {
            // Error handling could be added here
        }
    }
};
