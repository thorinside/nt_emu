#include "ParameterSystem.hpp"
#include "../plugin/PluginManager.hpp"
#include <rack.hpp>
#include <algorithm>

using namespace rack;

ParameterSystem::ParameterSystem(PluginManager* manager) : pluginManager(manager) {
    routingMatrix.fill(0);
    currentPageIndex = 0;
    currentParamIndex = 0;
}

void ParameterSystem::extractParameterData() {
    if (!pluginManager || !pluginManager->isLoaded()) {
        WARN("ParameterSystem: Cannot extract parameters - plugin not loaded");
        return;
    }
    
    _NT_factory* factory = pluginManager->getFactory();
    _NT_algorithm* algorithm = pluginManager->getAlgorithm();
    
    if (!algorithm) {
        WARN("ParameterSystem: pluginAlgorithm is null");
        return;
    }
    
    // Clear existing data first
    clearParameters();
    
    try {
        // Get algorithm requirements to know expected parameter count
        _NT_algorithmRequirements reqs;
        memset(&reqs, 0, sizeof(reqs));
        
        if (factory && factory->calculateRequirements) {
            try {
                // Prepare specifications array
                std::vector<int32_t> specValues;
                const int32_t* specifications = nullptr;
                
                const auto& pluginSpecs = pluginManager->getSpecifications();
                if (factory->numSpecifications > 0 && factory->specifications) {
                    if (!pluginSpecs.empty()) {
                        specValues = pluginSpecs;
                        INFO("ParameterSystem: Using loaded specifications for parameter extraction");
                    } else {
                        // Fallback to default values
                        specValues.resize(factory->numSpecifications);
                        for (uint32_t i = 0; i < factory->numSpecifications; i++) {
                            specValues[i] = factory->specifications[i].def;
                        }
                        INFO("ParameterSystem: Using default specifications for parameter extraction");
                    }
                    specifications = specValues.data();
                }
                
                factory->calculateRequirements(reqs, specifications);
                INFO("ParameterSystem: Plugin expects %u parameters", reqs.numParameters);
            } catch (...) {
                WARN("ParameterSystem: Failed to get algorithm requirements");
                return;
            }
        } else {
            WARN("ParameterSystem: No calculateRequirements function available");
            return;
        }
        
        // Extract parameters
        const _NT_parameter* parametersPtr = nullptr;
        const _NT_parameterPages* parameterPagesPtr = nullptr;
        
        try {
            // Safely read the parameters pointer
            volatile const _NT_parameter* volatile* ptrToPtr = 
                (volatile const _NT_parameter* volatile*)&algorithm->parameters;
            parametersPtr = (const _NT_parameter*)*ptrToPtr;
            
            // Safely read the parameter pages pointer
            volatile const _NT_parameterPages* volatile* pagesPtrToPtr = 
                (volatile const _NT_parameterPages* volatile*)&algorithm->parameterPages;
            parameterPagesPtr = (const _NT_parameterPages*)*pagesPtrToPtr;
        } catch (...) {
            WARN("ParameterSystem: Cannot read algorithm structure fields");
            return;
        }
        
        // Extract parameters
        if (reqs.numParameters > 0) {
            if (parametersPtr == nullptr) {
                INFO("ParameterSystem: parameters pointer is NULL (plugin may have no parameters)");
            } else if (!isValidPointer((void*)parametersPtr)) {
                WARN("ParameterSystem: parameters pointer is invalid");
                return;
            } else {
                // Extract each parameter safely
                for (uint32_t i = 0; i < reqs.numParameters; i++) {
                    const _NT_parameter* param = &parametersPtr[i];
                    if (extractSingleParameter(param, i)) {
                        // Set default value in routing matrix
                        if (i < routingMatrix.size()) {
                            routingMatrix[i] = parameters.back().def;
                        }
                    }
                }
            }
        }
        
        INFO("ParameterSystem: Extracted %zu parameters", parameters.size());
        
        // Debug logging disabled for performance
        
        // Extract parameter pages if available
        if (parameterPagesPtr != nullptr && isValidPointer((void*)parameterPagesPtr)) {
            try {
                uint32_t numPages = parameterPagesPtr->numPages;
                const _NT_parameterPage* pagesArray = parameterPagesPtr->pages;
                
                if (numPages > 0 && numPages <= 32 && isValidPointer((void*)pagesArray)) {
                    for (uint32_t pageIdx = 0; pageIdx < numPages; pageIdx++) {
                        const _NT_parameterPage* page = &pagesArray[pageIdx];
                        extractSinglePage(page, pageIdx);
                    }
                }
            } catch (...) {
                WARN("ParameterSystem: Failed to extract parameter pages");
            }
        }
        
        INFO("ParameterSystem: Extracted %zu parameter pages", parameterPages.size());
        INFO("ParameterSystem: hasParameters=%d, hasParameterPages=%d", 
             hasParameters(), hasParameterPages());
        
        // Set algorithm routing matrix pointer
        try {
            algorithm->v = routingMatrix.data();
        } catch (...) {
            WARN("ParameterSystem: Failed to set algorithm routing matrix pointer");
        }
        
        // Reset navigation to first page/param
        currentPageIndex = 0;
        currentParamIndex = 0;
        
        // Initialize all parameters by calling parameterChanged
        if (factory && factory->parameterChanged && algorithm) {
            for (size_t i = 0; i < parameters.size(); i++) {
                try {
                    factory->parameterChanged(algorithm, i);
                    // Successfully initialized parameter
                } catch (...) {
                    WARN("Failed to initialize parameter %zu", i);
                }
            }
        }
        
        notifyParametersExtracted();
        
    } catch (const std::exception& e) {
        WARN("ParameterSystem: Exception during parameter extraction: %s", e.what());
        clearParameters();
    } catch (...) {
        WARN("ParameterSystem: Unknown exception during parameter extraction");
        clearParameters();
    }
}

void ParameterSystem::clearParameters() {
    parameters.clear();
    parameterPages.clear();
    currentPageIndex = 0;
    currentParamIndex = 0;
}

void ParameterSystem::setCurrentPage(int pageIndex) {
    if (isValidPageIndex(pageIndex)) {
        currentPageIndex = pageIndex;
        currentParamIndex = 0; // Reset to first param of page
        notifyPageChanged(pageIndex);
    }
}

void ParameterSystem::setCurrentParam(int paramIndex) {
    if (isValidParameterIndex(paramIndex)) {
        currentParamIndex = paramIndex;
    }
}

void ParameterSystem::setParameterValue(int paramIdx, int16_t value) {
    if (!isValidParameterIndex(paramIdx)) return;
    
    // Clamp value to parameter bounds
    const _NT_parameter& param = parameters[paramIdx];
    int16_t clampedValue = clamp(value, param.min, param.max);
    
    if (paramIdx < (int)routingMatrix.size()) {
        routingMatrix[paramIdx] = clampedValue;
        notifyParameterChanged(paramIdx, clampedValue);
    }
}

int16_t ParameterSystem::getParameterValue(int paramIdx) const {
    if (isValidParameterIndex(paramIdx) && paramIdx < (int)routingMatrix.size()) {
        return routingMatrix[paramIdx];
    }
    return 0;
}

void ParameterSystem::confirmParameterValue() {
    // Notify observers that current parameter value has been confirmed
    if (isValidParameterIndex(currentParamIndex)) {
        int16_t value = getParameterValue(currentParamIndex);
        notifyParameterChanged(currentParamIndex, value);
    }
}

void ParameterSystem::resetParametersToDefaults() {
    for (size_t i = 0; i < parameters.size() && i < routingMatrix.size(); i++) {
        routingMatrix[i] = parameters[i].def;
    }
}

void ParameterSystem::clampParameterValues() {
    for (size_t i = 0; i < parameters.size() && i < routingMatrix.size(); i++) {
        const _NT_parameter& param = parameters[i];
        routingMatrix[i] = clamp(routingMatrix[i], param.min, param.max);
    }
}

bool ParameterSystem::canNavigateToNextPage() const {
    return !parameterPages.empty() && currentPageIndex < (int)parameterPages.size() - 1;
}

bool ParameterSystem::canNavigateToPrevPage() const {
    return !parameterPages.empty() && currentPageIndex > 0;
}

bool ParameterSystem::canNavigateToNextParam() const {
    return !parameters.empty() && currentParamIndex < (int)parameters.size() - 1;
}

bool ParameterSystem::canNavigateToPrevParam() const {
    return !parameters.empty() && currentParamIndex > 0;
}

void ParameterSystem::navigateToNextPage() {
    if (canNavigateToNextPage()) {
        setCurrentPage(currentPageIndex + 1);
    }
}

void ParameterSystem::navigateToPrevPage() {
    if (canNavigateToPrevPage()) {
        setCurrentPage(currentPageIndex - 1);
    }
}

void ParameterSystem::navigateToNextParam() {
    if (canNavigateToNextParam()) {
        setCurrentParam(currentParamIndex + 1);
    }
}

void ParameterSystem::navigateToPrevParam() {
    if (canNavigateToPrevParam()) {
        setCurrentParam(currentParamIndex - 1);
    }
}

bool ParameterSystem::isValidParameterIndex(int index) const {
    return index >= 0 && index < (int)parameters.size();
}

bool ParameterSystem::isValidPageIndex(int index) const {
    return index >= 0 && index < (int)parameterPages.size();
}

bool ParameterSystem::isValidParameterValue(int paramIdx, int16_t value) const {
    if (!isValidParameterIndex(paramIdx)) return false;
    
    const _NT_parameter& param = parameters[paramIdx];
    return value >= param.min && value <= param.max;
}

const _NT_parameter* ParameterSystem::getParameterInfo(int index) const {
    if (isValidParameterIndex(index)) {
        return &parameters[index];
    }
    return nullptr;
}

const _NT_parameterPage* ParameterSystem::getPageInfo(int index) const {
    if (isValidPageIndex(index)) {
        return &parameterPages[index];
    }
    return nullptr;
}

void ParameterSystem::addObserver(IParameterObserver* observer) {
    if (observer) {
        observers.push_back(observer);
    }
}

void ParameterSystem::removeObserver(IParameterObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void ParameterSystem::setRoutingMatrixValue(int index, int16_t value) {
    if (index >= 0 && index < (int)routingMatrix.size()) {
        routingMatrix[index] = value;
        notifyParameterChanged(index, value);
    }
}

json_t* ParameterSystem::saveParameterState() {
    json_t* rootJ = json_object();
    
    json_object_set_new(rootJ, "currentPageIndex", json_integer(currentPageIndex));
    json_object_set_new(rootJ, "currentParamIndex", json_integer(currentParamIndex));
    
    // Save routing matrix values
    json_t* routingJ = json_array();
    for (size_t i = 0; i < routingMatrix.size(); i++) {
        json_array_append_new(routingJ, json_integer(routingMatrix[i]));
    }
    json_object_set_new(rootJ, "routingMatrix", routingJ);
    
    return rootJ;
}

void ParameterSystem::loadParameterState(json_t* rootJ) {
    if (!rootJ) return;
    
    json_t* pageJ = json_object_get(rootJ, "currentPageIndex");
    if (pageJ) {
        setCurrentPage(json_integer_value(pageJ));
    }
    
    json_t* paramJ = json_object_get(rootJ, "currentParamIndex");
    if (paramJ) {
        setCurrentParam(json_integer_value(paramJ));
    }
    
    json_t* routingJ = json_object_get(rootJ, "routingMatrix");
    if (routingJ && json_is_array(routingJ)) {
        size_t arraySize = json_array_size(routingJ);
        for (size_t i = 0; i < arraySize && i < routingMatrix.size(); i++) {
            json_t* valueJ = json_array_get(routingJ, i);
            if (valueJ) {
                routingMatrix[i] = (int16_t)json_integer_value(valueJ);
            }
        }
    }
}

void ParameterSystem::notifyParameterChanged(int index, int16_t value) {
    for (auto* observer : observers) {
        observer->onParameterChanged(index, value);
    }
}

void ParameterSystem::notifyPageChanged(int pageIndex) {
    for (auto* observer : observers) {
        observer->onParameterPageChanged(pageIndex);
    }
}

void ParameterSystem::notifyParametersExtracted() {
    for (auto* observer : observers) {
        observer->onParametersExtracted();
    }
}

bool ParameterSystem::isValidPointer(void* ptr) const {
    if (!ptr) return false;
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    // Check for null and obviously invalid addresses
    if (addr < 0x1000) return false;
    
    // Check for known corruption patterns
    if (addr == 0x3f800000 || (addr >= 0x3f000000 && addr <= 0x40000000)) {
        return false;
    }
    
#ifdef ARCH_WIN
    if (addr > 0x7FFFFFFFFFFF) return false;
#else
    if (addr > 0x7FFFFFFFFFFF) return false;
#endif
    
    return true;
}

bool ParameterSystem::extractSingleParameter(const _NT_parameter* param, uint32_t index) {
    try {
        validateParameterAccess(param, index);
        
        // Test name access
        if (param->name[0] == '\0') {
            WARN("ParameterSystem: Parameter %u has empty name", index);
            return false;
        }
        
        // Validate parameter ranges
        if (param->min > param->max) {
            WARN("ParameterSystem: Parameter %u has invalid range: %d > %d", 
                 index, param->min, param->max);
            return false;
        }
        
        // Copy parameter safely
        _NT_parameter paramCopy;
        paramCopy.name = param->name;
        paramCopy.min = param->min;
        paramCopy.max = param->max;
        paramCopy.def = param->def;
        paramCopy.unit = param->unit;
        paramCopy.scaling = param->scaling;
        paramCopy.enumStrings = param->enumStrings;
        
        parameters.push_back(paramCopy);
        INFO("ParameterSystem: Extracted parameter %u: '%s' [%d-%d, def=%d]", 
             index, param->name, param->min, param->max, param->def);
        
        return true;
    } catch (...) {
        WARN("ParameterSystem: Failed to extract parameter %u", index);
        return false;
    }
}

bool ParameterSystem::extractSinglePage(const _NT_parameterPage* page, uint32_t index) {
    try {
        validatePageAccess(page, index);
        
        if (page->numParams == 0 || page->numParams > parameters.size()) {
            WARN("ParameterSystem: Page %u has invalid param count: %u", index, page->numParams);
            return false;
        }
        
        // Copy page safely
        _NT_parameterPage pageCopy;
        pageCopy.name = page->name;
        pageCopy.numParams = page->numParams;
        pageCopy.params = page->params; // Keep pointer reference
        
        parameterPages.push_back(pageCopy);
        INFO("ParameterSystem: Extracted page %u: '%s' (%u params)", 
             index, page->name, page->numParams);
        
        return true;
    } catch (...) {
        WARN("ParameterSystem: Failed to extract page %u", index);
        return false;
    }
}

void ParameterSystem::validateParameterAccess(const _NT_parameter* param, uint32_t index) const {
    if (!isValidPointer((void*)param->name)) {
        throw std::runtime_error("Parameter " + std::to_string(index) + " has invalid name pointer");
    }
}

void ParameterSystem::validatePageAccess(const _NT_parameterPage* page, uint32_t index) const {
    if (!isValidPointer((void*)page->name) || !isValidPointer((void*)page->params)) {
        throw std::runtime_error("Page " + std::to_string(index) + " has invalid pointers");
    }
}