// Wrapper to load test_customui_plugin.cpp through VCV
// This should be included in the VCV plugin to test it

#include "test_customui_plugin.cpp"

// Export the test factory for loading
extern "C" _NT_factory* getTestCustomUiFactory() {
    return &testCustomUiFactory;
}