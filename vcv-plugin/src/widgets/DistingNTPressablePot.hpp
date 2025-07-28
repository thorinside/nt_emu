#pragma once
#include <componentlibrary.hpp>
#include "../EmulatorCore.hpp"
#include <GLFW/glfw3.h>

using namespace rack;

struct DistingNTPressablePot : componentlibrary::RoundBigBlackKnob {
    EmulatorCore* emulatorCore = nullptr;
    int potIndex = 0;  // 0, 1, or 2 for left, center, right pot
    
    DistingNTPressablePot() : componentlibrary::RoundBigBlackKnob() {
        // Constructor automatically sets up the correct SVGs via parent class
    }
    
    void onDragStart(const event::DragStart& e) override {
        // Call parent first
        componentlibrary::RoundBigBlackKnob::onDragStart(e);
        
        // Press when drag starts
        if (emulatorCore) {
            emulatorCore->pressPot(potIndex);
        }
    }
    
    void onDragEnd(const event::DragEnd& e) override {
        // Release when drag ends
        if (emulatorCore) {
            emulatorCore->releasePot(potIndex);
        }
        
        // Call parent after
        componentlibrary::RoundBigBlackKnob::onDragEnd(e);
    }
    
    void setEmulatorCore(EmulatorCore* core, int index) {
        emulatorCore = core;
        potIndex = index;
    }
};