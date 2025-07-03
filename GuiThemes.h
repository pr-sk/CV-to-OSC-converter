#pragma once
#include "imgui.h"

namespace GuiThemes {

// Dark Professional Theme (текущая)
void ApplyDarkProfessional() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    
    // Зеленые акцентные цвета
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.65f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.75f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.75f, 0.45f, 1.00f);
    
    // Закругленные углы
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
}

// Neon Synthwave Theme
void ApplyNeonSynthwave() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    ImVec4* colors = style.Colors;
    // Темный фон с фиолетовыми оттенками
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.02f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.03f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.03f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.8f, 0.2f, 0.8f, 0.8f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.05f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.08f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.10f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.01f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.03f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.03f, 0.12f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.08f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.15f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.25f, 0.45f, 1.00f);
    
    // Неоновые акценты
    colors[ImGuiCol_SliderGrab] = ImVec4(0.9f, 0.1f, 0.9f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.2f, 1.0f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 1.0f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9f, 0.1f, 0.9f, 0.3f);
    
    // Более круглые формы
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 6.0f;
}

// Light Professional Theme  
void ApplyLightProfessional() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    
    // Синие акценты для светлой темы
    colors[ImGuiCol_SliderGrab] = ImVec4(0.2f, 0.4f, 0.8f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1f, 0.3f, 0.9f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.1f, 0.3f, 0.9f, 1.00f);
    
    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;
}

// Retro Green Terminal Theme
void ApplyRetroTerminal() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    ImVec4* colors = style.Colors;
    // Черный фон как у терминала
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.0f, 0.8f, 0.0f, 0.8f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.1f, 0.0f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.2f, 0.0f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.3f, 0.0f, 1.00f);
    
    // Зеленый текст как в старых терминалах
    colors[ImGuiCol_Text] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2f, 1.0f, 0.2f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.00f);
    
    // Никаких закруглений - четкие углы
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
}

// Modern macOS Theme
void ApplyMacOSModern() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    ImVec4* colors = style.Colors;
    // macOS inspired colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    
    // macOS blue accent
    colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1f, 0.58f, 1.0f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.0f, 0.48f, 1.0f, 0.8f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.58f, 1.0f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.38f, 0.9f, 1.0f);
    
    // Большие закругления как в macOS
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 12.0f;
    style.TabRounding = 8.0f;
    
    // Увеличенные отступы
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
}

} // namespace GuiThemes
