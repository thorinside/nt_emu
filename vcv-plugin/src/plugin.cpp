#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add modules
    p->addModel(modelTestModule);
    p->addModel(modelDistingNT);
    p->addModel(modelPressableWidgetsDebugTest);
    // Temporarily disable test module to isolate crash
    // p->addModel(modelPressableWidgetsTest);

    // Any other plugin initialization code goes here
}