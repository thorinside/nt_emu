#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>

// Emulator includes
#include "core/emulator_console.h"

class ConsoleUI {
public:
    ConsoleUI(std::shared_ptr<EmulatorConsole> emulator) : emulator_(emulator) {}
    
    void run() {
        printHeader();
        printHelp();
        
        std::string input;
        while (running_) {
            std::cout << "\n> ";
            std::cout.flush();
            
            if (!std::getline(std::cin, input)) {
                // EOF or stream error
                std::cout << "\nInput stream closed. Exiting...\n";
                break;
            }
            
            if (input.empty()) continue;
            
            processCommand(input);
            emulator_->update();
        }
    }
    
private:
    std::shared_ptr<EmulatorConsole> emulator_;
    bool running_ = true;
    
    void printHeader() {
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                 Disting NT Emulator v1.0                 ║\n";
        std::cout << "║              Expert Sleepers Plugin Emulator             ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
        std::cout << "\nEmulator initialized successfully!\n";
    }
    
    void printHelp() {
        std::cout << "\nCommands:\n";
        std::cout << "  load <plugin.dylib>  - Load a plugin\n";
        std::cout << "  unload               - Unload current plugin\n";
        std::cout << "  start                - Start audio processing\n";
        std::cout << "  stop                 - Stop audio processing\n";
        std::cout << "  pot <n> <value>      - Set pot value (n=1-3, value=0-1)\n";
        std::cout << "  button <n> <state>   - Set button state (n=1-4, state=0/1)\n";
        std::cout << "  encoder <n> <value>  - Set encoder value (n=1-2)\n";
        std::cout << "  status               - Show current status\n";
        std::cout << "  help                 - Show this help\n";
        std::cout << "  quit                 - Exit emulator\n";
    }
    
    void processCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "quit" || cmd == "exit") {
            running_ = false;
            std::cout << "Shutting down emulator...\n";
            
        } else if (cmd == "help") {
            printHelp();
            
        } else if (cmd == "load") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                std::cout << "Loading plugin: " << path << "\n";
                if (emulator_->loadPlugin(path)) {
                    std::cout << "✓ Plugin loaded successfully\n";
                } else {
                    std::cout << "✗ Failed to load plugin\n";
                }
            } else {
                std::cout << "Usage: load <plugin.dylib>\n";
            }
            
        } else if (cmd == "unload") {
            emulator_->unloadPlugin();
            std::cout << "Plugin unloaded\n";
            
        } else if (cmd == "start") {
            if (emulator_->startAudio()) {
                std::cout << "✓ Audio started\n";
            } else {
                std::cout << "✗ Failed to start audio\n";
            }
            
        } else if (cmd == "stop") {
            emulator_->stopAudio();
            std::cout << "Audio stopped\n";
            
        } else if (cmd == "status") {
            printStatus();
            
        } else if (cmd == "pot") {
            int pot;
            float value;
            if (iss >> pot >> value) {
                if (pot >= 1 && pot <= 3 && value >= 0.0f && value <= 1.0f) {
                    emulator_->setPotValue(pot - 1, value);  // Convert to 0-based
                    std::cout << "Set pot " << pot << " to " << value << "\n";
                } else {
                    std::cout << "Invalid pot number (1-3) or value (0-1)\n";
                }
            } else {
                std::cout << "Usage: pot <1-3> <0-1>\n";
            }
            
        } else if (cmd == "button") {
            int button, state;
            if (iss >> button >> state) {
                if (button >= 1 && button <= 4 && (state == 0 || state == 1)) {
                    emulator_->setButtonState(button - 1, state == 1);  // Convert to 0-based
                    std::cout << "Set button " << button << " to " << (state ? "ON" : "OFF") << "\n";
                } else {
                    std::cout << "Invalid button number (1-4) or state (0/1)\n";
                }
            } else {
                std::cout << "Usage: button <1-4> <0/1>\n";
            }
            
        } else if (cmd == "encoder") {
            int encoder, value;
            if (iss >> encoder >> value) {
                if (encoder >= 1 && encoder <= 2) {
                    emulator_->setEncoderValue(encoder - 1, value);  // Convert to 0-based
                    std::cout << "Set encoder " << encoder << " to " << value << "\n";
                } else {
                    std::cout << "Invalid encoder number (1-2)\n";
                }
            } else {
                std::cout << "Usage: encoder <1-2> <value>\n";
            }
            
        } else {
            std::cout << "Unknown command: " << cmd << "\n";
            std::cout << "Type 'help' for available commands\n";
        }
    }
    
    void printStatus() {
        std::cout << "\n--- Emulator Status ---\n";
        std::cout << "Plugin: " << (emulator_->isPluginLoaded() ? 
                                   emulator_->getPluginPath() : "None") << "\n";
        std::cout << "Audio: " << (emulator_->isAudioRunning() ? "Running" : "Stopped") << "\n";
        std::cout << "CPU Load: " << (emulator_->getAudioCpuLoad() * 100.0f) << "%\n";
        std::cout << "Sample Rate: 96 kHz\n";
        std::cout << "Block Size: 4 samples\n";
        std::cout << "Audio Buses: 28\n";
        std::cout << "Display: 256x64 (console mode)\n";
        std::cout << "Controls: 3 pots, 4 buttons, 2 encoders\n";
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Initializing Disting NT Emulator...\n";
    
    auto emulator = std::make_shared<EmulatorConsole>();
    
    if (!emulator->initialize()) {
        std::cerr << "Failed to initialize emulator\n";
        return 1;
    }
    
    // Auto-load plugin if provided as argument
    if (argc > 1) {
        std::string plugin_path = argv[1];
        std::cout << "Auto-loading plugin: " << plugin_path << "\n";
        if (emulator->loadPlugin(plugin_path)) {
            std::cout << "✓ Plugin loaded successfully\n";
            if (emulator->startAudio()) {
                std::cout << "✓ Audio started automatically\n";
            }
        } else {
            std::cout << "✗ Failed to auto-load plugin\n";
        }
    }
    
    ConsoleUI ui(emulator);
    ui.run();
    
    emulator->shutdown();
    std::cout << "Goodbye!\n";
    
    return 0;
}