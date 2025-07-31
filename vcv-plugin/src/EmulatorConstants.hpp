#pragma once

namespace EmulatorConstants {
    enum ParamIds {
        // Pots
        POT_L_PARAM, POT_C_PARAM, POT_R_PARAM,
        
        // Buttons 
        BUTTON_1_PARAM, BUTTON_2_PARAM, BUTTON_3_PARAM, BUTTON_4_PARAM,
        
        // Encoders (simplified for widget placement)
        ENCODER_L_PARAM, ENCODER_R_PARAM,
        ENCODER_L_PRESS_PARAM, ENCODER_R_PRESS_PARAM,
        
        // Algorithm selection
        ALGORITHM_PARAM,
        
        NUM_PARAMS
    };
    
    enum InputIds {
        // 12 inputs (as per hardware specification)
        AUDIO_INPUT_1, AUDIO_INPUT_2, AUDIO_INPUT_3, AUDIO_INPUT_4,
        AUDIO_INPUT_5, AUDIO_INPUT_6, AUDIO_INPUT_7, AUDIO_INPUT_8,
        AUDIO_INPUT_9, AUDIO_INPUT_10, AUDIO_INPUT_11, AUDIO_INPUT_12,
        
        NUM_INPUTS
    };
    
    enum OutputIds {
        // 8 outputs (as per hardware specification)
        AUDIO_OUTPUT_1, AUDIO_OUTPUT_2, AUDIO_OUTPUT_3, 
        AUDIO_OUTPUT_4, AUDIO_OUTPUT_5, AUDIO_OUTPUT_6,
        AUDIO_OUTPUT_7, AUDIO_OUTPUT_8,
        
        NUM_OUTPUTS
    };
    
    enum LightIds {
        BUTTON_1_LIGHT, BUTTON_2_LIGHT, BUTTON_3_LIGHT, BUTTON_4_LIGHT,
        INPUT_LIGHT_1, INPUT_LIGHT_2, INPUT_LIGHT_3, INPUT_LIGHT_4,
        INPUT_LIGHT_5, INPUT_LIGHT_6, INPUT_LIGHT_7, INPUT_LIGHT_8,
        INPUT_LIGHT_9, INPUT_LIGHT_10, INPUT_LIGHT_11, INPUT_LIGHT_12,
        OUTPUT_LIGHT_1, OUTPUT_LIGHT_2, OUTPUT_LIGHT_3, 
        OUTPUT_LIGHT_4, OUTPUT_LIGHT_5, OUTPUT_LIGHT_6,
        OUTPUT_LIGHT_7, OUTPUT_LIGHT_8,
        MIDI_INPUT_LIGHT, MIDI_OUTPUT_LIGHT,
        NUM_LIGHTS
    };
}