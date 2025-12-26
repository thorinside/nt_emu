#include "MenuSystem.hpp"
#include "../parameter/ParameterSystem.hpp"
#include <rack.hpp>
#include <algorithm>
#include <cmath>

using namespace rack;

MenuSystem::MenuSystem(ParameterSystem* paramSystem) : parameterSystem(paramSystem) {
    // Initialize input tracker
    inputTracker.lastPotValues.fill(-1.0f);
    // Encoder deltas are event-based, no tracking needed
    inputTracker.lastEncoderPressed.fill(false);
}

void MenuSystem::toggleMenu() {
    if (currentState == State::OFF) {
        if (canNavigate()) {
            // Enter menu with all controls active simultaneously
            enterState(State::VALUE_EDIT);
        }
    } else {
        exitMenu();
    }
}

void MenuSystem::exitMenu() {
    if (currentState != State::OFF) {
        enterState(State::OFF);
    }
}

void MenuSystem::enterState(State newState) {
    // Simplified state entry - no state machine transitions
    currentState = newState;
    onStateEnter(currentState);
    notifyStateChanged();
    notifyModeChanged();
}

void MenuSystem::processNavigation(const std::array<float, 3>& potValues, 
                                  const std::array<int, 2>& encoderDeltas,
                                  const std::array<bool, 2>& encoderPressed) {
    if (!canNavigate() || currentState == State::OFF) {
        return;
    }
    
    // All three controls work independently and simultaneously
    // Left Pot: selects parameter page
    processPageSelection(potValues[0]);
    
    // Center Pot and Left Encoder: select parameter
    // encoderDeltas now contains the actual +1/-1 step values
    processParameterSelection(potValues[1], encoderDeltas[0]);
    
    // Right Pot and Right Encoder: modify parameter value
    processValueEditing(potValues[2], encoderDeltas[1]);
    
    // Update input tracking
    inputTracker.lastPotValues = potValues;
    // Encoder deltas don't need to be tracked
    inputTracker.lastEncoderPressed = encoderPressed;
}

void MenuSystem::navigateToPage(int pageIndex) {
    if (isValidPageIndex(pageIndex)) {
        parameterSystem->setCurrentPage(pageIndex);
        notifyStateChanged();
    }
}

void MenuSystem::navigateToParameter(int paramIndex) {
    INFO("MenuSystem: navigateToParameter(%d) - valid=%d", paramIndex, isValidParamIndex(paramIndex));
    if (isValidParamIndex(paramIndex)) {
        parameterSystem->setCurrentParam(paramIndex);
        notifyStateChanged();
    }
}

void MenuSystem::editParameterValue(int delta) {
    if (currentState != State::VALUE_EDIT) return;
    
    int actualParamIndex = getActualParameterIndex();
    if (actualParamIndex < 0) return;
    
    const _NT_parameter* param = parameterSystem->getParameterInfo(actualParamIndex);
    
    if (param) {
        // Simply add the encoder delta to the current value
        int newValue = clamp(parameterEditValue + delta, (int)param->min, (int)param->max);
        
        // Debug logging
        INFO("MenuSystem: editParameterValue - param '%s' delta=%d current=%d new=%d range=[%d,%d]", 
             param->name, delta, parameterEditValue, newValue, (int)param->min, (int)param->max);
        
        if (newValue != parameterEditValue) {
            parameterEditValue = newValue;
            // Set the value immediately in the parameter system
            parameterSystem->setParameterValue(actualParamIndex, (int16_t)parameterEditValue);
            notifyParameterChanged(actualParamIndex, (int16_t)parameterEditValue);
        }
    }
}

void MenuSystem::setParameterValue(int value) {
    if (currentState != State::VALUE_EDIT) return;
    
    int actualParamIndex = getActualParameterIndex();
    if (actualParamIndex < 0) return;
    
    const _NT_parameter* param = parameterSystem->getParameterInfo(actualParamIndex);
    
    if (param) {
        int clampedValue = clamp(value, (int)param->min, (int)param->max);
        if (clampedValue != parameterEditValue) {
            parameterEditValue = clampedValue;
            // Set the value immediately in the parameter system
            parameterSystem->setParameterValue(actualParamIndex, (int16_t)parameterEditValue);
            notifyParameterChanged(actualParamIndex, (int16_t)parameterEditValue);
        }
    }
}

void MenuSystem::confirmParameterEdit() {
    if (currentState == State::VALUE_EDIT) {
        int actualParamIndex = getActualParameterIndex();
        if (actualParamIndex >= 0) {
            parameterSystem->setParameterValue(actualParamIndex, (int16_t)parameterEditValue);
            parameterSystem->confirmParameterValue();
        }
    }
}

int MenuSystem::getActualParameterIndex() const {
    if (!parameterSystem->hasParameterPages()) return -1;
    
    int pageIndex = parameterSystem->getCurrentPageIndex();
    if (pageIndex >= (int)parameterSystem->getPageCount()) return -1;
    
    const _NT_parameterPage* page = parameterSystem->getPageInfo(pageIndex);
    if (!page) return -1;
    
    int paramIndexInPage = parameterSystem->getCurrentParamIndex();
    if (paramIndexInPage >= page->numParams) return -1;
    
    // If page has parameter index array, use it
    if (page->params) {
        return page->params[paramIndexInPage];
    }
    
    // Otherwise, assume sequential indexing
    return paramIndexInPage;
}

void MenuSystem::addObserver(IMenuObserver* observer) {
    if (observer) {
        observers.push_back(observer);
    }
}

void MenuSystem::removeObserver(IMenuObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

bool MenuSystem::canNavigate() const {
    return parameterSystem && 
           parameterSystem->hasParameters() && 
           parameterSystem->hasParameterPages();
}

bool MenuSystem::hasParametersToEdit() const {
    return parameterSystem && parameterSystem->hasParameters();
}

const char* MenuSystem::getStateString() const {
    switch (currentState) {
        case State::OFF: return "OFF";
        case State::PAGE_SELECT: return "PAGE_SELECT";
        case State::PARAM_SELECT: return "PARAM_SELECT";
        case State::VALUE_EDIT: return "VALUE_EDIT";
        default: return "UNKNOWN";
    }
}

json_t* MenuSystem::saveMenuState() {
    json_t* rootJ = json_object();
    
    json_object_set_new(rootJ, "currentState", json_integer((int)currentState));
    json_object_set_new(rootJ, "parameterEditValue", json_integer(parameterEditValue));
    json_object_set_new(rootJ, "potSensitivity", json_real(potSensitivity));
    json_object_set_new(rootJ, "encoderSensitivity", json_real(encoderSensitivity));
    
    return rootJ;
}

void MenuSystem::loadMenuState(json_t* rootJ) {
    if (!rootJ) return;
    
    json_t* stateJ = json_object_get(rootJ, "currentState");
    if (stateJ) {
        int stateValue = json_integer_value(stateJ);
        if (stateValue >= 0 && stateValue <= (int)State::VALUE_EDIT) {
            currentState = (State)stateValue;
        }
    }
    
    json_t* editValueJ = json_object_get(rootJ, "parameterEditValue");
    if (editValueJ) {
        parameterEditValue = json_integer_value(editValueJ);
    }
    
    json_t* potSensJ = json_object_get(rootJ, "potSensitivity");
    if (potSensJ) {
        potSensitivity = json_number_value(potSensJ);
    }
    
    json_t* encSensJ = json_object_get(rootJ, "encoderSensitivity");
    if (encSensJ) {
        encoderSensitivity = json_number_value(encSensJ);
    }
}

void MenuSystem::onStateEnter(State state) {
    switch (state) {
        case State::OFF:
            // Reset input tracking
            inputTracker.lastPotValues.fill(-1.0f);
            // Encoder deltas are event-based, no tracking needed
            break;
            
        case State::PAGE_SELECT:
            // Reset input tracking to prevent spurious encoder press detection
            inputTracker.lastEncoderPressed.fill(false);
            inputTracker.lastPotValues.fill(-1.0f);
            // Encoder deltas are event-based, no tracking needed
            
            // Reset to first page
            if (parameterSystem->getPageCount() > 0) {
                parameterSystem->setCurrentPage(0);
            }
            break;
            
        case State::PARAM_SELECT:
            // Reset to first parameter of current page
            parameterSystem->setCurrentParam(0);
            break;
            
        case State::VALUE_EDIT:
            // Initialize edit value with current parameter value using actual parameter index
            {
                int actualParamIndex = getActualParameterIndex();
                if (actualParamIndex >= 0) {
                    parameterEditValue = parameterSystem->getParameterValue(actualParamIndex);
                    INFO("MenuSystem: Entering VALUE_EDIT - paramIndex=%d, initialValue=%d", 
                         actualParamIndex, parameterEditValue);
                }
            }
            // Reset tracking to ensure all controls are responsive
            inputTracker.lastPotValues.fill(-1.0f);
            // Encoder deltas are event-based, no tracking needed
            break;
    }
}

void MenuSystem::onStateExit(State state) {
    switch (state) {
        case State::VALUE_EDIT:
            // Could save or discard changes here if needed
            break;
        default:
            break;
    }
}

void MenuSystem::transition(State newState) {
    // Simplified transition - no state machine complexity
    currentState = newState;
    onStateEnter(currentState);
    notifyStateChanged();
    notifyModeChanged();
}

void MenuSystem::processPageSelection(float potValue) {
    if (hasPotChanged(0, potValue) && parameterSystem->getPageCount() > 1) {
        int newPageIndex = calculatePageFromPot(potValue);
        if (newPageIndex != parameterSystem->getCurrentPageIndex()) {
            navigateToPage(newPageIndex);
        }
    }
}

void MenuSystem::processParameterSelection(float potValue, int encoderDelta) {
    // Handle center pot
    if (hasPotChanged(1, potValue)) {
        int newParamIndex = calculateParamFromPot(potValue);
        if (newParamIndex != parameterSystem->getCurrentParamIndex()) {
            navigateToParameter(newParamIndex);
        }
    }
    
    // Handle left encoder (relative)
    if (encoderDelta != 0) {
        // Get the current page's parameter count
        int pageIndex = parameterSystem->getCurrentPageIndex();
        const _NT_parameterPage* page = parameterSystem->getPageInfo(pageIndex);
        if (!page) return;
        
        int currentParam = parameterSystem->getCurrentParamIndex();
        int maxParams = page->numParams;
        int newParamIndex = clamp(currentParam + encoderDelta, 0, maxParams - 1);
        if (newParamIndex != currentParam) {
            navigateToParameter(newParamIndex);
        }
    }
}

void MenuSystem::processValueEditing(float potValue, int encoderDelta) {
    // Get the actual parameter index from the current page
    int actualParamIndex = getActualParameterIndex();
    if (actualParamIndex < 0) {
        INFO("MenuSystem: Invalid parameter index");
        return;
    }
    
    const _NT_parameter* param = parameterSystem->getParameterInfo(actualParamIndex);
    if (!param) return;
    
    // Handle right pot (absolute)
    if (hasPotChanged(2, potValue)) {
        int newValue = calculateValueFromPot(potValue, actualParamIndex);
        setParameterValue(newValue);
    }
    
    // Handle right encoder (relative)
    if (encoderDelta != 0) {
        editParameterValue(encoderDelta);
    }
}

bool MenuSystem::hasPotChanged(int potIndex, float newValue) {
    if (potIndex < 0 || potIndex >= 3) return false;
    
    float lastValue = inputTracker.lastPotValues[potIndex];
    bool changed = std::abs(newValue - lastValue) > potSensitivity;
    
    
    if (changed) {
        inputTracker.lastPotValues[potIndex] = newValue;
    }
    
    return changed;
}

bool MenuSystem::hasEncoderChanged(int encoderIndex, int newValue) {
    if (encoderIndex < 0 || encoderIndex >= 2) return false;
    
    // Encoder deltas are always "new" when non-zero
    return newValue != 0;
}

bool MenuSystem::hasEncoderPressChanged(int encoderIndex, bool pressed) {
    if (encoderIndex < 0 || encoderIndex >= 2) return false;
    
    bool lastPressed = inputTracker.lastEncoderPressed[encoderIndex];
    bool changed = pressed != lastPressed;
    
    if (changed) {
        inputTracker.lastEncoderPressed[encoderIndex] = pressed;
    }
    
    return changed;
}

int MenuSystem::calculatePageFromPot(float potValue) const {
    if (!parameterSystem) return 0;

    int pageCount = parameterSystem->getPageCount();
    if (pageCount <= 1) return 0;

    // Use rounding for consistent behavior with param/value selection
    int pageIndex = (int)std::round(potValue * (pageCount - 1));
    return clamp(pageIndex, 0, pageCount - 1);
}

int MenuSystem::calculateParamFromPot(float potValue) const {
    if (!parameterSystem) return 0;
    
    // Get the current page's parameter count
    int pageIndex = parameterSystem->getCurrentPageIndex();
    if (pageIndex >= (int)parameterSystem->getPageCount()) return 0;
    
    const _NT_parameterPage* page = parameterSystem->getPageInfo(pageIndex);
    if (!page) return 0;
    
    int paramCount = page->numParams;
    
    // Special case: if there's only one parameter, always return 0
    if (paramCount <= 1) return 0;
    
    // For multiple parameters, use rounding for better accuracy
    int paramIndex = (int)std::round(potValue * (paramCount - 1));
    return clamp(paramIndex, 0, paramCount - 1);
}

int MenuSystem::calculateValueFromPot(float potValue, int paramIndex) const {
    const _NT_parameter* param = parameterSystem->getParameterInfo(paramIndex);
    if (!param) return 0;
    
    int range = param->max - param->min;
    // Use rounding instead of truncation for better accuracy
    int value = param->min + (int)std::round(potValue * range);
    
    
    return clamp(value, (int)param->min, (int)param->max);
}

bool MenuSystem::isValidPageIndex(int index) const {
    return parameterSystem && parameterSystem->isValidPageIndex(index);
}

bool MenuSystem::isValidParamIndex(int index) const {
    return parameterSystem && parameterSystem->isValidParameterIndex(index);
}

void MenuSystem::notifyStateChanged() {
    for (auto* observer : observers) {
        observer->onMenuStateChanged();
    }
}

void MenuSystem::notifyModeChanged() {
    for (auto* observer : observers) {
        observer->onMenuModeChanged((int)currentState);
    }
}

void MenuSystem::notifyParameterChanged(int paramIndex, int16_t value) {
    for (auto* observer : observers) {
        observer->onMenuParameterChanged(paramIndex, value);
    }
}