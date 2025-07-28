#pragma once
#include <rack.hpp>
#include <functional>
#include <array>

using namespace rack;

// Forward declarations
class ParameterSystem;

// Menu state observer interface
class IMenuObserver {
public:
    virtual ~IMenuObserver() = default;
    virtual void onMenuStateChanged() = 0;
    virtual void onMenuModeChanged(int newMode) = 0;
    virtual void onMenuParameterChanged(int paramIndex, int16_t value) = 0;
};

// Menu system state machine for DistingNT
class MenuSystem {
public:
    enum class State {
        OFF,
        PAGE_SELECT,
        PARAM_SELECT,
        VALUE_EDIT
    };
    
    MenuSystem(ParameterSystem* paramSystem);
    ~MenuSystem() = default;
    
    // State management
    State getCurrentState() const { return currentState; }
    bool isMenuActive() const { return currentState != State::OFF; }
    
    void toggleMenu();
    void exitMenu();
    void enterState(State newState);
    
    // Navigation input processing
    void processNavigation(const std::array<float, 3>& potValues, 
                          const std::array<int, 2>& encoderValues,
                          const std::array<bool, 2>& encoderPressed);
    
    // Direct navigation
    void navigateToPage(int pageIndex);
    void navigateToParameter(int paramIndex);
    void editParameterValue(int delta);
    void setParameterValue(int value);
    void confirmParameterEdit();
    
    // Current editing state
    int getCurrentEditValue() const { return parameterEditValue; }
    bool isEditingParameter() const { return currentState == State::VALUE_EDIT; }
    
    // Input sensitivity
    void setPotSensitivity(float sensitivity) { potSensitivity = sensitivity; }
    void setEncoderSensitivity(float sensitivity) { encoderSensitivity = sensitivity; }
    
    float getPotSensitivity() const { return potSensitivity; }
    float getEncoderSensitivity() const { return encoderSensitivity; }
    
    // Observer pattern
    void addObserver(IMenuObserver* observer);
    void removeObserver(IMenuObserver* observer);
    
    // Validation
    bool canNavigate() const;
    bool hasParametersToEdit() const;
    
    // State queries
    const char* getStateString() const;
    json_t* saveMenuState();
    void loadMenuState(json_t* rootJ);
    
private:
    ParameterSystem* parameterSystem;
    
    // State machine
    State currentState = State::OFF;
    
    // Current editing state
    int parameterEditValue = 0;
    
    // Input tracking for change detection
    struct InputTracker {
        std::array<float, 3> lastPotValues = {-1.0f, -1.0f, -1.0f};
        std::array<int, 2> lastEncoderValues = {0, 0};
        std::array<bool, 2> lastEncoderPressed = {false, false};
    };
    InputTracker inputTracker;
    
    // Configuration
    float potSensitivity = 0.00001f;
    float encoderSensitivity = 1.0f;
    
    // Observers
    std::vector<IMenuObserver*> observers;
    
    // State machine implementation
    void onStateEnter(State state);
    void onStateExit(State state);
    void transition(State newState);
    
    // Input processing by state
    void processPageSelection(float potValue);
    void processParameterSelection(float potValue, int encoderDelta);
    void processValueEditing(float potValue, int encoderDelta);
    
    // Input change detection
    bool hasPotChanged(int potIndex, float newValue);
    bool hasEncoderChanged(int encoderIndex, int newValue);
    bool hasEncoderPressChanged(int encoderIndex, bool pressed);
    
    // Navigation helpers
    int calculatePageFromPot(float potValue) const;
    int calculateParamFromPot(float potValue) const;
    int calculateValueFromPot(float potValue, int paramIndex) const;
    
    // Validation helpers
    bool isValidPageIndex(int index) const;
    bool isValidParamIndex(int index) const;
    
    // Observer notifications
    void notifyStateChanged();
    void notifyModeChanged();
    void notifyParameterChanged(int paramIndex, int16_t value);
    
    // Helper to get the actual parameter index from page-relative index
    int getActualParameterIndex() const;
    
    // Utility
    template<typename T>
    T clamp(T value, T min, T max) const {
        return (value < min) ? min : (value > max) ? max : value;
    }
};