#include "MenuSystem.hpp"
#include "../parameter/ParameterSystem.hpp"
#include <rack.hpp>
#include <algorithm>
#include <cmath>

using namespace rack;

MenuSystem::MenuSystem(ParameterSystem* paramSystem) : parameterSystem(paramSystem) {
    // Initialize input tracker
    inputTracker.lastPotValues.fill(-1.0f);
    inputTracker.lastEncoderValues.fill(0);
    inputTracker.lastEncoderPressed.fill(false);
}

void MenuSystem::toggleMenu() {
    if (currentState == State::OFF) {
        if (canNavigate()) {
            enterState(State::PAGE_SELECT);
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
    if (newState != currentState) {
        transition(newState);
    }
}

void MenuSystem::processNavigation(const std::array<float, 3>& potValues, 
                                  const std::array<int, 2>& encoderValues,
                                  const std::array<bool, 2>& encoderPressed) {
    if (!canNavigate()) {
        exitMenu();
        return;
    }
    
    // Check for encoder press changes first
    for (int i = 0; i < 2; i++) {
        if (hasEncoderPressChanged(i, encoderPressed[i]) && encoderPressed[i]) {
            // Handle encoder press based on current state
            switch (currentState) {
                case State::PAGE_SELECT:
                    if (parameterSystem->hasParameters()) {
                        enterState(State::PARAM_SELECT);
                    }
                    break;
                case State::PARAM_SELECT:
                    parameterEditValue = parameterSystem->getParameterValue(parameterSystem->getCurrentParamIndex());
                    enterState(State::VALUE_EDIT);
                    break;
                case State::VALUE_EDIT:
                    confirmParameterEdit();
                    enterState(State::PARAM_SELECT);
                    break;
                case State::OFF:
                default:
                    break;
            }
        }
    }
    
    // Process navigation based on current state
    switch (currentState) {
        case State::PAGE_SELECT:
            processPageSelection(potValues[0]); // Left pot
            break;
            
        case State::PARAM_SELECT:
            processParameterSelection(potValues[1], // Center pot
                                    encoderValues[0] - inputTracker.lastEncoderValues[0]); // Left encoder delta
            break;
            
        case State::VALUE_EDIT:
            processValueEditing(potValues[2], // Right pot
                              encoderValues[1] - inputTracker.lastEncoderValues[1]); // Right encoder delta
            break;
            
        case State::OFF:
        default:
            break;
    }
    
    // Update input tracking
    inputTracker.lastPotValues = potValues;
    inputTracker.lastEncoderValues = encoderValues;
    inputTracker.lastEncoderPressed = encoderPressed;
}

void MenuSystem::navigateToPage(int pageIndex) {
    if (isValidPageIndex(pageIndex)) {
        parameterSystem->setCurrentPage(pageIndex);
        notifyStateChanged();
    }
}

void MenuSystem::navigateToParameter(int paramIndex) {
    if (isValidParamIndex(paramIndex)) {
        parameterSystem->setCurrentParam(paramIndex);
        notifyStateChanged();
    }
}

void MenuSystem::editParameterValue(int delta) {
    if (currentState != State::VALUE_EDIT) return;
    
    int paramIndex = parameterSystem->getCurrentParamIndex();
    const _NT_parameter* param = parameterSystem->getParameterInfo(paramIndex);
    
    if (param) {
        int newValue = clamp(parameterEditValue + delta, (int)param->min, (int)param->max);
        if (newValue != parameterEditValue) {
            parameterEditValue = newValue;
            notifyParameterChanged(paramIndex, (int16_t)parameterEditValue);
        }
    }
}

void MenuSystem::setParameterValue(int value) {
    if (currentState != State::VALUE_EDIT) return;
    
    int paramIndex = parameterSystem->getCurrentParamIndex();
    const _NT_parameter* param = parameterSystem->getParameterInfo(paramIndex);
    
    if (param) {
        int clampedValue = clamp(value, (int)param->min, (int)param->max);
        if (clampedValue != parameterEditValue) {
            parameterEditValue = clampedValue;
            notifyParameterChanged(paramIndex, (int16_t)parameterEditValue);
        }
    }
}

void MenuSystem::confirmParameterEdit() {
    if (currentState == State::VALUE_EDIT) {
        int paramIndex = parameterSystem->getCurrentParamIndex();
        parameterSystem->setParameterValue(paramIndex, (int16_t)parameterEditValue);
        parameterSystem->confirmParameterValue();
    }
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
            inputTracker.lastEncoderValues.fill(0);
            break;
            
        case State::PAGE_SELECT:
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
            // Initialize edit value with current parameter value
            parameterEditValue = parameterSystem->getParameterValue(parameterSystem->getCurrentParamIndex());
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
    onStateExit(currentState);
    previousState = currentState;
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
        int currentParam = parameterSystem->getCurrentParamIndex();
        int maxParams = parameterSystem->getParameterCount();
        int newParamIndex = clamp(currentParam + encoderDelta, 0, maxParams - 1);
        if (newParamIndex != currentParam) {
            navigateToParameter(newParamIndex);
        }
    }
}

void MenuSystem::processValueEditing(float potValue, int encoderDelta) {
    int paramIndex = parameterSystem->getCurrentParamIndex();
    const _NT_parameter* param = parameterSystem->getParameterInfo(paramIndex);
    
    if (!param) return;
    
    // Handle right pot (absolute)
    if (hasPotChanged(2, potValue)) {
        int newValue = calculateValueFromPot(potValue, paramIndex);
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
    
    bool changed = newValue != inputTracker.lastEncoderValues[encoderIndex];
    return changed;
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
    
    int pageIndex = (int)(potValue * (pageCount - 1));
    return clamp(pageIndex, 0, pageCount - 1);
}

int MenuSystem::calculateParamFromPot(float potValue) const {
    if (!parameterSystem) return 0;
    
    int paramCount = parameterSystem->getParameterCount();
    if (paramCount <= 1) return 0;
    
    int paramIndex = (int)(potValue * (paramCount - 1));
    return clamp(paramIndex, 0, paramCount - 1);
}

int MenuSystem::calculateValueFromPot(float potValue, int paramIndex) const {
    const _NT_parameter* param = parameterSystem->getParameterInfo(paramIndex);
    if (!param) return 0;
    
    int range = param->max - param->min;
    int value = param->min + (int)(potValue * range);
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