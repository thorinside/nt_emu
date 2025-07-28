#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add modules
    p->addModel(modelNtEmu);

    // Any other plugin initialization code goes here
}