#include <iostream>
#include <string>
#include <memory>

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// GLFW and OpenGL
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl3w.h>
#endif

// Emulator includes
#include "core/emulator.h"
#include "ui/main_window.h"

class Application {
public:
    Application() = default;
    ~Application() = default;
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Set OpenGL version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        // Create window
        window_ = glfwCreateWindow(1200, 800, "Disting NT Emulator", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();
        
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize emulator
        emulator_ = std::make_shared<Emulator>();
        if (!emulator_->initialize()) {
            std::cerr << "Failed to initialize emulator" << std::endl;
            return false;
        }
        
        // Initialize main window
        main_window_ = std::make_unique<DistingNTMainWindow>();
        main_window_->setEmulator(emulator_);
        main_window_->setHardwareInterface(emulator_->getHardwareInterface());
        
        std::cout << "Application initialized successfully" << std::endl;
        return true;
    }
    
    void shutdown() {
        if (emulator_) {
            emulator_->shutdown();
        }
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        if (window_) {
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
        
        std::cout << "Application shutdown complete" << std::endl;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Update emulator
            emulator_->update();
            
            // Render authentic Disting NT UI
            main_window_->render();
            
            // Render ImGui
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window_);
        }
    }
    
private:
    GLFWwindow* window_ = nullptr;
    std::shared_ptr<Emulator> emulator_;
    std::unique_ptr<DistingNTMainWindow> main_window_;
};

int main(int argc, char* argv[]) {
    std::cout << "Disting NT Emulator starting..." << std::endl;
    
    Application app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    // If a plugin path was provided as argument, try to load it
    if (argc > 1) {
        std::cout << "Loading plugin from command line: " << argv[1] << std::endl;
        // Note: We'll need to defer this until after the main loop starts
        // since the emulator needs to be fully initialized
    }
    
    app.run();
    app.shutdown();
    
    std::cout << "Disting NT Emulator exiting" << std::endl;
    return 0;
}