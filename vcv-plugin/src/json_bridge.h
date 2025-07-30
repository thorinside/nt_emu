#pragma once

#include <distingnt/serialisation.h>
#include <memory>
#include <stack>
#include <vector>
#include <string>
#include "../../emulator/third_party/json/single_include/nlohmann/json.hpp"

// Bridge class for writing JSON using nlohmann::json
// This class implements the same interface as _NT_jsonStream but doesn't inherit from it
// due to private constructor/destructor in the base class
class JsonStreamBridge {
private:
    nlohmann::json root;
    std::stack<std::pair<nlohmann::json*, std::string>> context_stack;
    nlohmann::json* current_context;
    std::string pending_member_name;
    bool has_pending_member;
    
    void pushContext(nlohmann::json* new_context);
    void popContext();
    void addValue(const nlohmann::json& value);

public:
    JsonStreamBridge();
    ~JsonStreamBridge();
    
    // Get the resulting JSON object
    nlohmann::json getJson() const;
    
    // Implement all methods from _NT_jsonStream interface
    void openArray();
    void closeArray();
    void openObject();
    void closeObject();
    void addMemberName(const char* name);
    void addNumber(int value);
    void addNumber(float value);
    void addString(const char* str);
    void addFourCC(uint32_t fourcc);
    void addBoolean(bool value);
    void addNull();
};

// Bridge class for reading JSON using nlohmann::json  
// This class implements the same interface as _NT_jsonParse but doesn't inherit from it
// due to private constructor/destructor in the base class
class JsonParseBridge {
private:
    nlohmann::json root;
    std::stack<std::pair<nlohmann::json*, int>> context_stack;
    nlohmann::json* current_context;
    int current_index;
    std::vector<std::string> string_storage; // For string lifetime management
    
    void pushContext(nlohmann::json* new_context, int start_index = 0);
    void popContext();
    nlohmann::json* getCurrentElement();
    std::string getCurrentMemberName();

public:
    JsonParseBridge(const nlohmann::json& data);
    ~JsonParseBridge();
    
    // Implement all methods from _NT_jsonParse interface
    bool numberOfArrayElements(int& num);
    bool numberOfObjectMembers(int& num);
    bool matchName(const char* name);
    bool skipMember();
    bool number(int& value);
    bool number(float& value);
    bool string(const char*& str);
    bool boolean(bool& value);
    bool null();
};

// Thread-local storage management functions
void setCurrentJsonParse(std::unique_ptr<JsonParseBridge> bridge);
void clearCurrentJsonParse();
JsonParseBridge* getCurrentJsonParse();

void setCurrentJsonStream(std::unique_ptr<JsonStreamBridge> bridge);
void clearCurrentJsonStream();
JsonStreamBridge* getCurrentJsonStream();