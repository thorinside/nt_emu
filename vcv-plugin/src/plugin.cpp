#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add modules
    p->addModel(modelTestModule);
    p->addModel(modelDistingNT);

    // Any other plugin initialization code goes here
}