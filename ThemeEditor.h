#pragma once
#include "imgui.h"

class ThemeEditor {
public:
    static void ShowThemeEditor(bool* p_open = nullptr) {
        if (!ImGui::Begin("Theme Editor", p_open)) {
            ImGui::End();
            return;
        }

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Основные настройки стиля
        ImGui::Text("Style Settings");
        ImGui::Separator();
        
        ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Child Rounding", &style.ChildRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Popup Rounding", &style.PopupRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 20.0f);
        ImGui::SliderFloat("Tab Rounding", &style.TabRounding, 0.0f, 20.0f);
        
        ImGui::Spacing();
        ImGui::SliderFloat2("Window Padding", (float*)&style.WindowPadding, 0.0f, 20.0f);
        ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f);
        ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f);
        ImGui::SliderFloat2("Item Inner Spacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f);

        ImGui::Spacing();
        ImGui::Text("Colors");
        ImGui::Separator();

        // Основные цвета окон
        if (ImGui::TreeNode("Windows")) {
            ImGui::ColorEdit4("Window Background", (float*)&colors[ImGuiCol_WindowBg]);
            ImGui::ColorEdit4("Child Background", (float*)&colors[ImGuiCol_ChildBg]);
            ImGui::ColorEdit4("Popup Background", (float*)&colors[ImGuiCol_PopupBg]);
            ImGui::ColorEdit4("Border", (float*)&colors[ImGuiCol_Border]);
            ImGui::ColorEdit4("Title Background", (float*)&colors[ImGuiCol_TitleBg]);
            ImGui::ColorEdit4("Title Background Active", (float*)&colors[ImGuiCol_TitleBgActive]);
            ImGui::ColorEdit4("Menu Bar Background", (float*)&colors[ImGuiCol_MenuBarBg]);
            ImGui::TreePop();
        }

        // Цвета элементов управления
        if (ImGui::TreeNode("Controls")) {
            ImGui::ColorEdit4("Frame Background", (float*)&colors[ImGuiCol_FrameBg]);
            ImGui::ColorEdit4("Frame Background Hovered", (float*)&colors[ImGuiCol_FrameBgHovered]);
            ImGui::ColorEdit4("Frame Background Active", (float*)&colors[ImGuiCol_FrameBgActive]);
            ImGui::ColorEdit4("Button", (float*)&colors[ImGuiCol_Button]);
            ImGui::ColorEdit4("Button Hovered", (float*)&colors[ImGuiCol_ButtonHovered]);
            ImGui::ColorEdit4("Button Active", (float*)&colors[ImGuiCol_ButtonActive]);
            ImGui::TreePop();
        }

        // Цвета слайдеров и элементов выбора
        if (ImGui::TreeNode("Sliders & Selection")) {
            ImGui::ColorEdit4("Slider Grab", (float*)&colors[ImGuiCol_SliderGrab]);
            ImGui::ColorEdit4("Slider Grab Active", (float*)&colors[ImGuiCol_SliderGrabActive]);
            ImGui::ColorEdit4("Check Mark", (float*)&colors[ImGuiCol_CheckMark]);
            ImGui::ColorEdit4("Header", (float*)&colors[ImGuiCol_Header]);
            ImGui::ColorEdit4("Header Hovered", (float*)&colors[ImGuiCol_HeaderHovered]);
            ImGui::ColorEdit4("Header Active", (float*)&colors[ImGuiCol_HeaderActive]);
            ImGui::TreePop();
        }

        // Цвета текста
        if (ImGui::TreeNode("Text")) {
            ImGui::ColorEdit4("Text", (float*)&colors[ImGuiCol_Text]);
            ImGui::ColorEdit4("Text Disabled", (float*)&colors[ImGuiCol_TextDisabled]);
            ImGui::ColorEdit4("Text Selected Background", (float*)&colors[ImGuiCol_TextSelectedBg]);
            ImGui::TreePop();
        }

        ImGui::Spacing();
        
        // Кнопки сохранения/загрузки
        if (ImGui::Button("Save Theme")) {
            SaveCurrentTheme();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Default")) {
            ImGui::StyleColorsDark();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Default Dark")) {
            ImGui::StyleColorsDark();
        }

        ImGui::End();
    }

private:
    static void SaveCurrentTheme() {
        // Здесь можно реализовать сохранение темы в файл
        // Для простоты сейчас просто выводим в консоль
        ImGuiStyle& style = ImGui::GetStyle();
        std::cout << "Current theme settings:" << std::endl;
        std::cout << "WindowRounding: " << style.WindowRounding << std::endl;
        std::cout << "FrameRounding: " << style.FrameRounding << std::endl;
        // ... и т.д.
    }
};
