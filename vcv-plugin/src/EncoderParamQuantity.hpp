#pragma once
#include <rack.hpp>

using namespace rack;

struct EncoderParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        return "Turn";  // Don't show accumulated value which confuses users
    }
    
    std::string getString() override {
        return getLabel();  // Use getLabel() instead of getParamInfo()->label
    }
    
    std::string getUnit() override {
        return "";  // No unit for encoder turns
    }
};