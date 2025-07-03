#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;
    
    // Create window with OpenGL context
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Cyrillic Font Test", NULL, NULL);
    if (window == NULL) return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader" << std::endl;
        return 1;
    }
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Setup fonts with Cyrillic support
    io.Fonts->Clear();
    
    // Load default font first
    io.Fonts->AddFontDefault();
    
    // Add Cyrillic font
    static const ImWchar cyrillic_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    
    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = true;
    
    // Try to load system font with Cyrillic support
    const char* font_paths[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Geneva.ttf"
    };
    
    ImFont* cyrillic_font = nullptr;
    for (const char* font_path : font_paths) {
        cyrillic_font = io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &font_config, cyrillic_ranges);
        if (cyrillic_font) {
            std::cout << "Loaded font: " << font_path << std::endl;
            break;
        }
    }
    
    if (!cyrillic_font) {
        std::cout << "Warning: Could not load Cyrillic font, using default" << std::endl;
    }
    
    // Build font atlas
    io.Fonts->Build();
    
    // Style
    ImGui::StyleColorsDark();
    
    // Test strings
    const char* test_strings[] = {
        "English: Audio Device",
        "Русский: Микрофон MacBook Pro", 
        "Русский: Аудио устройство",
        "Русский: Текущее устройство",
        "Test: АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ",
        "Test: абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
    };
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Test window
        ImGui::Begin("Cyrillic Font Test");
        
        ImGui::Text("Font Test Results:");
        ImGui::Separator();
        
        // Test with default font
        ImGui::Text("Default Font:");
        for (const char* str : test_strings) {
            ImGui::BulletText("%s", str);
        }
        
        ImGui::Spacing();
        
        // Test with Cyrillic font if loaded
        if (cyrillic_font) {
            ImGui::PushFont(cyrillic_font);
            ImGui::Text("Cyrillic Font:");
            for (const char* str : test_strings) {
                ImGui::BulletText("%s", str);
            }
            ImGui::PopFont();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Audio device simulation
        ImGui::Text("Audio Device Info:");
        if (cyrillic_font) ImGui::PushFont(cyrillic_font);
        ImGui::Text("Current Device: %s", "Микрофон MacBook Pro");
        ImGui::Text("Status: %s", "Подключено");
        if (cyrillic_font) ImGui::PopFont();
        
        ImGui::End();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
