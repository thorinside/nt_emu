#include <iostream>
#include <dlfcn.h>
#include <string>

int main() {
    std::cout << "Testing symbol resolution for gainCustomUI plugin..." << std::endl;
    
    // First, load the main plugin to make symbols available
    std::cout << "Loading VCV plugin..." << std::endl;
    void* vcvHandle = dlopen("./plugin.dylib", RTLD_NOW | RTLD_GLOBAL);
    if (!vcvHandle) {
        std::cerr << "Failed to load VCV plugin: " << dlerror() << std::endl;
        return 1;
    }
    std::cout << "VCV plugin loaded successfully" << std::endl;
    
    // Check if key symbols are available
    void* ntScreen = dlsym(RTLD_DEFAULT, "_NT_screen");
    void* ntDrawText = dlsym(RTLD_DEFAULT, "_NT_drawText");
    void* ntAlgorithmIndex = dlsym(RTLD_DEFAULT, "_NT_algorithmIndex");
    void* ntParameterOffset = dlsym(RTLD_DEFAULT, "_NT_parameterOffset");
    void* ntSetParameterFromUi = dlsym(RTLD_DEFAULT, "_NT_setParameterFromUi");
    
    std::cout << "Symbol availability:" << std::endl;
    std::cout << "  _NT_screen: " << (ntScreen ? "FOUND" : "MISSING") << std::endl;
    std::cout << "  _NT_drawText: " << (ntDrawText ? "FOUND" : "MISSING") << std::endl;
    std::cout << "  _NT_algorithmIndex: " << (ntAlgorithmIndex ? "FOUND" : "MISSING") << std::endl;
    std::cout << "  _NT_parameterOffset: " << (ntParameterOffset ? "FOUND" : "MISSING") << std::endl;
    std::cout << "  _NT_setParameterFromUi: " << (ntSetParameterFromUi ? "FOUND" : "MISSING") << std::endl;
    
    // Now try to load the gainCustomUI plugin
    std::cout << std::endl << "Loading gainCustomUI plugin..." << std::endl;
    void* pluginHandle = dlopen("../emulator/test_plugins/examples/gainCustomUI.dylib", RTLD_NOW);
    if (!pluginHandle) {
        std::cerr << "Failed to load gainCustomUI plugin: " << dlerror() << std::endl;
        dlclose(vcvHandle);
        return 1;
    }
    std::cout << "gainCustomUI plugin loaded successfully!" << std::endl;
    
    // Try to get the plugin entry point
    typedef uintptr_t (*PluginEntryFunc)(int selector, uint32_t data);
    PluginEntryFunc pluginEntry = (PluginEntryFunc)dlsym(pluginHandle, "pluginEntry");
    if (pluginEntry) {
        std::cout << "pluginEntry function found" << std::endl;
        // Test calling it
        try {
            uintptr_t version = pluginEntry(0, 0); // kNT_selector_version
            std::cout << "Plugin API version: " << version << std::endl;
        } catch (...) {
            std::cout << "Exception calling pluginEntry" << std::endl;
        }
    } else {
        std::cout << "pluginEntry function not found, trying NT_getFactoryPtr..." << std::endl;
        void* factory = dlsym(pluginHandle, "NT_getFactoryPtr");
        if (factory) {
            std::cout << "NT_getFactoryPtr found" << std::endl;
        } else {
            std::cout << "NT_getFactoryPtr not found either" << std::endl;
        }
    }
    
    std::cout << std::endl << "SUCCESS: Symbol resolution issue appears to be fixed!" << std::endl;
    
    // Cleanup
    dlclose(pluginHandle);
    dlclose(vcvHandle);
    
    return 0;
}