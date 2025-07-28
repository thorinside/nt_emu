#pragma once
#include <rack.hpp>
#include <vector>
#include <array>
#include <functional>
#include "../nt_api_interface.h"

using namespace rack;

// Forward declarations
class PluginManager;
class IParameterObserver;

// Observer interface for parameter changes
class IParameterObserver {
public:
    virtual ~IParameterObserver() = default;
    virtual void onParameterChanged(int index, int16_t value) = 0;
    virtual void onParameterPageChanged(int pageIndex) = 0;
    virtual void onParametersExtracted() = 0;
};

// Parameter management system
class ParameterSystem {
public:
    ParameterSystem(PluginManager* manager);
    ~ParameterSystem() = default;
    
    // Parameter extraction and management
    void extractParameterData();
    void clearParameters();
    
    // Parameter pages
    int getCurrentPageIndex() const { return currentPageIndex; }
    int getCurrentParamIndex() const { return currentParamIndex; }
    void setCurrentPage(int pageIndex);
    void setCurrentParam(int paramIndex);
    
    // Parameter access
    const std::vector<_NT_parameter>& getParameters() const { return parameters; }
    const std::vector<_NT_parameterPage>& getParameterPages() const { return parameterPages; }
    
    bool hasParameters() const { return !parameters.empty(); }
    bool hasParameterPages() const { return !parameterPages.empty(); }
    
    size_t getParameterCount() const { return parameters.size(); }
    size_t getPageCount() const { return parameterPages.size(); }
    
    // Parameter value management
    void setParameterValue(int paramIdx, int16_t value);
    int16_t getParameterValue(int paramIdx) const;
    void confirmParameterValue();
    
    // Batch parameter operations
    void resetParametersToDefaults();
    void clampParameterValues();
    
    // Navigation helpers
    bool canNavigateToNextPage() const;
    bool canNavigateToPrevPage() const;
    bool canNavigateToNextParam() const;
    bool canNavigateToPrevParam() const;
    
    void navigateToNextPage();
    void navigateToPrevPage();
    void navigateToNextParam();
    void navigateToPrevParam();
    
    // Parameter validation
    bool isValidParameterIndex(int index) const;
    bool isValidPageIndex(int index) const;
    bool isValidParameterValue(int paramIdx, int16_t value) const;
    
    // Get parameter info
    const _NT_parameter* getParameterInfo(int index) const;
    const _NT_parameterPage* getPageInfo(int index) const;
    
    // Observer pattern
    void addObserver(IParameterObserver* observer);
    void removeObserver(IParameterObserver* observer);
    
    // Routing matrix access (parameter values storage)
    const std::array<int16_t, 256>& getRoutingMatrix() const { return routingMatrix; }
    std::array<int16_t, 256>& getRoutingMatrix() { return routingMatrix; }
    void setRoutingMatrixValue(int index, int16_t value);
    
    // State persistence
    json_t* saveParameterState();
    void loadParameterState(json_t* rootJ);
    
private:
    PluginManager* pluginManager;
    
    // Parameter data
    std::vector<_NT_parameter> parameters;
    std::vector<_NT_parameterPage> parameterPages;
    
    // Current navigation state
    int currentPageIndex = 0;
    int currentParamIndex = 0;
    
    // Parameter values storage (routing matrix)
    std::array<int16_t, 256> routingMatrix;
    
    // Observers
    std::vector<IParameterObserver*> observers;
    
    // Internal helpers
    void notifyParameterChanged(int index, int16_t value);
    void notifyPageChanged(int pageIndex);
    void notifyParametersExtracted();
    
    bool isValidPointer(void* ptr) const;
    void validateParameterAccess(const _NT_parameter* param, uint32_t index) const;
    void validatePageAccess(const _NT_parameterPage* page, uint32_t index) const;
    
    // Safe parameter extraction helpers
    bool extractSingleParameter(const _NT_parameter* param, uint32_t index);
    bool extractSinglePage(const _NT_parameterPage* page, uint32_t index);
};