#include "json_bridge.h"
#include <iostream>
#include <rack.hpp>

using json = nlohmann::json;

// JsonStreamBridge implementation
JsonStreamBridge::JsonStreamBridge() : current_context(&root), has_pending_member(false) {
    root = json::object();
}

JsonStreamBridge::~JsonStreamBridge() = default;

void JsonStreamBridge::pushContext(json* new_context) {
    context_stack.push(std::make_pair(current_context, pending_member_name));
    current_context = new_context;
    pending_member_name.clear();
    has_pending_member = false;
}

void JsonStreamBridge::popContext() {
    if (!context_stack.empty()) {
        std::pair<json*, std::string> prev = context_stack.top();
        context_stack.pop();
        current_context = prev.first;
        pending_member_name = prev.second;
        has_pending_member = !prev.second.empty();
    }
}

void JsonStreamBridge::addValue(const json& value) {
    if (current_context->is_object()) {
        if (has_pending_member) {
            (*current_context)[pending_member_name] = value;
            pending_member_name.clear();
            has_pending_member = false;
        } else {
            std::cerr << "JsonStreamBridge: Attempted to add value to object without member name" << std::endl;
        }
    } else if (current_context->is_array()) {
        current_context->push_back(value);
    } else {
        *current_context = value;
    }
}

json JsonStreamBridge::getJson() const {
    return root;
}

void JsonStreamBridge::openArray() {
    json new_array = json::array();
    
    if (current_context->is_object() && has_pending_member) {
        (*current_context)[pending_member_name] = new_array;
        json* array_ptr = &(*current_context)[pending_member_name];
        pushContext(array_ptr);
    } else if (current_context->is_array()) {
        current_context->push_back(new_array);
        json* array_ptr = &current_context->back();
        pushContext(array_ptr);
    } else {
        *current_context = new_array;
        pushContext(current_context);
    }
}

void JsonStreamBridge::closeArray() {
    popContext();
}

void JsonStreamBridge::openObject() {
    json new_object = json::object();
    
    if (current_context->is_object() && has_pending_member) {
        (*current_context)[pending_member_name] = new_object;
        json* object_ptr = &(*current_context)[pending_member_name];
        pushContext(object_ptr);
    } else if (current_context->is_array()) {
        current_context->push_back(new_object);
        json* object_ptr = &current_context->back();
        pushContext(object_ptr);
    } else {
        *current_context = new_object;
        pushContext(current_context);
    }
}

void JsonStreamBridge::closeObject() {
    popContext();
}

void JsonStreamBridge::addMemberName(const char* name) {
    printf("JsonStreamBridge::addMemberName called with: %s\n", name);
    pending_member_name = name;
    has_pending_member = true;
}

void JsonStreamBridge::addNumber(int value) {
    addValue(json(value));
}

void JsonStreamBridge::addNumber(float value) {
    printf("JsonStreamBridge::addNumber(float) called with: %f\n", value);
    addValue(json(value));
}

void JsonStreamBridge::addString(const char* str) {
    addValue(json(std::string(str)));
}

void JsonStreamBridge::addFourCC(uint32_t fourcc) {
    // Convert FourCC to string representation
    char fourcc_str[5];
    fourcc_str[0] = (fourcc >> 24) & 0xFF;
    fourcc_str[1] = (fourcc >> 16) & 0xFF;
    fourcc_str[2] = (fourcc >> 8) & 0xFF;
    fourcc_str[3] = fourcc & 0xFF;
    fourcc_str[4] = '\0';
    addValue(json(std::string(fourcc_str)));
}

void JsonStreamBridge::addBoolean(bool value) {
    addValue(json(value));
}

void JsonStreamBridge::addNull() {
    addValue(json(nullptr));
}

// JsonParseBridge implementation
JsonParseBridge::JsonParseBridge(const json& data) : root(data), current_context(&root), current_index(0) {
    INFO("JsonParseBridge: Constructor called with JSON type: %s", 
         root.type_name());
    INFO("JsonParseBridge: JSON is_object: %s, size: %zu", 
         root.is_object() ? "true" : "false",
         root.is_object() ? root.size() : 0);
    INFO("JsonParseBridge: root address: %p, current_context: %p", 
         (void*)&root, (void*)current_context);
    if (root.is_object() && !root.empty()) {
        std::string keys;
        int count = 0;
        for (auto it = root.begin(); it != root.end() && count < 3; ++it, ++count) {
            if (count > 0) keys += ", ";
            keys += "'" + it.key() + "'";
        }
        INFO("JsonParseBridge: First few keys: %s", keys.c_str());
    }
}

JsonParseBridge::~JsonParseBridge() = default;

void JsonParseBridge::pushContext(json* new_context, int start_index) {
    INFO("JsonParseBridge::pushContext - saving context %p at index %d, entering new context %p", 
         (void*)current_context, current_index, (void*)new_context);
    context_stack.push(std::make_pair(current_context, current_index));
    current_context = new_context;
    current_index = start_index;
}

void JsonParseBridge::popContext() {
    if (!context_stack.empty()) {
        std::pair<json*, int> prev = context_stack.top();
        context_stack.pop();
        INFO("JsonParseBridge::popContext - restoring context %p at index %d", 
             (void*)prev.first, prev.second);
        current_context = prev.first;
        current_index = prev.second;
    } else {
        WARN("JsonParseBridge::popContext - context stack is empty!");
    }
}

json* JsonParseBridge::getCurrentElement() {
    if (current_context->is_array()) {
        if (current_index < static_cast<int>(current_context->size())) {
            return &(*current_context)[current_index];
        }
    } else if (current_context->is_object()) {
        auto it = current_context->begin();
        std::advance(it, current_index);
        if (it != current_context->end()) {
            return &(it.value());
        }
    }
    return current_context;
}

std::string JsonParseBridge::getCurrentMemberName() {
    if (current_context->is_object()) {
        auto it = current_context->begin();
        std::advance(it, current_index);
        if (it != current_context->end()) {
            return it.key();
        }
    }
    return "";
}

bool JsonParseBridge::numberOfArrayElements(int& num) {
    INFO("JsonParseBridge::numberOfArrayElements - context type: %s", 
         current_context ? current_context->type_name() : "null");
    if (current_context->is_array()) {
        num = static_cast<int>(current_context->size());
        INFO("JsonParseBridge::numberOfArrayElements - returning true, size=%d", num);
        return true;
    }
    INFO("JsonParseBridge::numberOfArrayElements - returning false (not an array)");
    return false;
}

bool JsonParseBridge::numberOfObjectMembers(int& num) {
    INFO("JsonParseBridge::numberOfObjectMembers - current_context: %p", (void*)current_context);
    if (!current_context) {
        WARN("JsonParseBridge::numberOfObjectMembers - ERROR: current_context is NULL!");
        num = 0;
        return false;
    }
    INFO("JsonParseBridge::numberOfObjectMembers - type: %s, is_object: %s", 
         current_context->type_name(),
         current_context->is_object() ? "true" : "false");
    if (current_context->is_object()) {
        num = static_cast<int>(current_context->size());
        INFO("JsonParseBridge::numberOfObjectMembers - returning true, num=%d", num);
        return true;
    }
    INFO("JsonParseBridge::numberOfObjectMembers - returning false (not an object)");
    return false;
}

bool JsonParseBridge::matchName(const char* name) {
    INFO("JsonParseBridge::matchName - looking for '%s', current_index=%d", name, current_index);
    
    // If we're in an array context and at the end, pop back to parent
    if (current_context->is_array()) {
        if (current_index >= static_cast<int>(current_context->size())) {
            INFO("JsonParseBridge::matchName - at end of array, popping back to parent");
            popContext();
            // After processing an array that was an object member, advance to next member
            if (current_context->is_object()) {
                current_index++;
                INFO("JsonParseBridge::matchName - incremented object index to %d after array", current_index);
            }
        } else {
            INFO("JsonParseBridge::matchName - still in array at index %d of %zu, not object context", 
                 current_index, current_context->size());
            return false;
        }
    }
    
    if (current_context->is_object()) {
        std::string current_name = getCurrentMemberName();
        INFO("JsonParseBridge::matchName - current member name: '%s'", current_name.c_str());
        if (current_name == name) {
            // Enter the value of this member
            json* element = getCurrentElement();
            if (element) {
                INFO("JsonParseBridge::matchName - found match, entering context for '%s'", name);
                pushContext(element);
                return true;
            }
        }
    }
    INFO("JsonParseBridge::matchName - no match for '%s'", name);
    return false;
}

bool JsonParseBridge::skipMember() {
    INFO("JsonParseBridge::skipMember - current_index=%d", current_index);
    if (current_context->is_object() || current_context->is_array()) {
        current_index++;
        INFO("JsonParseBridge::skipMember - incremented index to %d", current_index);
        return true;
    }
    return false;
}

bool JsonParseBridge::number(int& value) {
    INFO("JsonParseBridge::number(int) - current_context type: %s, current_index: %d", 
         current_context->type_name(), current_index);
    json* element = getCurrentElement();
    if (element && element->is_number_integer()) {
        value = element->get<int>();
        INFO("JsonParseBridge::number(int) - got value %d", value);
        
        // Only pop context if we're in a nested value context (not in array)
        if (current_context->is_array()) {
            // In array context, just advance to next element
            current_index++;
            INFO("JsonParseBridge::number(int) - incremented array index to %d", current_index);
        } else {
            // In object member value context, pop back to object
            INFO("JsonParseBridge::number(int) - popping context");
            popContext();
            if (current_context->is_object()) {
                current_index++; // Move to next member in object
                INFO("JsonParseBridge::number(int) - incremented object index to %d", current_index);
            }
        }
        return true;
    }
    return false;
}

bool JsonParseBridge::number(float& value) {
    json* element = getCurrentElement();
    if (element && element->is_number()) {
        value = element->get<float>();
        
        // Only pop context if we're in a nested value context (not in array)
        if (current_context->is_array()) {
            // In array context, just advance to next element
            current_index++;
        } else {
            // In object member value context, pop back to object
            popContext();
            if (current_context->is_object()) {
                current_index++; // Move to next member in object
            }
        }
        return true;
    }
    return false;
}

bool JsonParseBridge::string(const char*& str) {
    INFO("JsonParseBridge::string - called, current_context type: %s, index: %d", 
         current_context ? current_context->type_name() : "null", current_index);
    json* element = getCurrentElement();
    if (element && element->is_string()) {
        // Store string in our storage to ensure lifetime
        string_storage.push_back(element->get<std::string>());
        str = string_storage.back().c_str();
        INFO("JsonParseBridge::string - got string: '%s'", str);
        
        // Only pop context if we're in a nested value context (not in array)
        if (current_context->is_array()) {
            // In array context, just advance to next element
            current_index++;
            INFO("JsonParseBridge::string - incremented array index to %d", current_index);
        } else {
            // In object member value context, pop back to object
            popContext();
            if (current_context->is_object()) {
                current_index++; // Move to next member in object
            }
        }
        return true;
    }
    INFO("JsonParseBridge::string - returning false (not a string or null element)");
    return false;
}

bool JsonParseBridge::boolean(bool& value) {
    json* element = getCurrentElement();
    if (element && element->is_boolean()) {
        value = element->get<bool>();
        popContext(); // Exit the current element context
        // After processing a value, advance to next element/member
        if (current_context->is_object()) {
            current_index++; // Move to next member in object
        } else if (current_context->is_array()) {
            current_index++; // Move to next element in array
        }
        return true;
    }
    return false;
}

bool JsonParseBridge::null() {
    json* element = getCurrentElement();
    if (element && element->is_null()) {
        popContext(); // Exit the current element context
        current_index++; // Move to next element in parent
        return true;
    }
    return false;
}