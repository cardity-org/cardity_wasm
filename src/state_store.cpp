#include "state_store.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace cardity {

// StateValue 类型转换实现
int StateValue::to_int() const {
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

bool StateValue::to_bool() const {
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return !value.empty();
}

double StateValue::to_float() const {
    try {
        return std::stod(value);
    } catch (...) {
        return 0.0;
    }
}

StateValue StateValue::from_string(const std::string& val) {
    return StateValue(ValueType::STRING, val);
}

StateValue StateValue::from_int(int val) {
    return StateValue(ValueType::INT, std::to_string(val));
}

StateValue StateValue::from_bool(bool val) {
    return StateValue(ValueType::BOOL, val ? "true" : "false");
}

StateValue StateValue::from_float(double val) {
    return StateValue(ValueType::FLOAT, std::to_string(val));
}

// MemoryStateStore 实现
bool MemoryStateStore::set_value(const std::string& key, const StateValue& value) {
    try {
        state[key] = value;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error setting value: " << e.what() << std::endl;
        return false;
    }
}

StateValue MemoryStateStore::get_value(const std::string& key) const {
    auto it = state.find(key);
    if (it != state.end()) {
        return it->second;
    }
    return StateValue(); // 返回默认值
}

bool MemoryStateStore::has_key(const std::string& key) const {
    return state.find(key) != state.end();
}

bool MemoryStateStore::remove_key(const std::string& key) {
    auto it = state.find(key);
    if (it != state.end()) {
        state.erase(it);
        return true;
    }
    return false;
}

void MemoryStateStore::set_multiple(const std::map<std::string, StateValue>& values) {
    for (const auto& [key, value] : values) {
        state[key] = value;
    }
}

std::map<std::string, StateValue> MemoryStateStore::get_all() const {
    return state;
}

bool MemoryStateStore::save_to_file(const std::string& file_path) const {
    try {
        json j;
        for (const auto& [key, value] : state) {
            json value_json;
            value_json["type"] = static_cast<int>(value.type);
            value_json["value"] = value.value;
            j[key] = value_json;
        }
        
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
            return false;
        }
        
        file << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving to file: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryStateStore::load_from_file(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        json j = json::parse(buffer.str());
        
        state.clear();
        for (const auto& [key, value_json] : j.items()) {
            StateValue value;
            value.type = static_cast<ValueType>(value_json["type"].get<int>());
            value.value = value_json["value"].get<std::string>();
            state[key] = value;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading from file: " << e.what() << std::endl;
        return false;
    }
}

json MemoryStateStore::create_snapshot() const {
    json snapshot;
    snapshot["timestamp"] = std::to_string(std::time(nullptr));
    snapshot["state"] = json::object();
    
    for (const auto& [key, value] : state) {
        json value_json;
        value_json["type"] = static_cast<int>(value.type);
        value_json["value"] = value.value;
        snapshot["state"][key] = value_json;
    }
    
    return snapshot;
}

bool MemoryStateStore::restore_from_snapshot(const json& snapshot) {
    try {
        if (!snapshot.contains("state")) {
            std::cerr << "Invalid snapshot format: missing state" << std::endl;
            return false;
        }
        
        state.clear();
        const auto& state_json = snapshot["state"];
        
        for (const auto& [key, value_json] : state_json.items()) {
            StateValue value;
            value.type = static_cast<ValueType>(value_json["type"].get<int>());
            value.value = value_json["value"].get<std::string>();
            state[key] = value;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error restoring from snapshot: " << e.what() << std::endl;
        return false;
    }
}

void MemoryStateStore::clear() {
    state.clear();
}

size_t MemoryStateStore::size() const {
    return state.size();
}

void MemoryStateStore::initialize_from_protocol(const std::map<std::string, std::string>& state_def) {
    for (const auto& [name, default_value] : state_def) {
        StateValue value(ValueType::STRING, default_value);
        state[name] = value;
    }
}

// StateManager 实现
StateManager::StateManager() : store(std::make_unique<MemoryStateStore>()) {}

StateManager::StateManager(std::unique_ptr<StateStore> state_store) : store(std::move(state_store)) {}

bool StateManager::set(const std::string& key, const std::string& value) {
    return store->set_value(key, StateValue::from_string(value));
}

bool StateManager::set_int(const std::string& key, int value) {
    return store->set_value(key, StateValue::from_int(value));
}

bool StateManager::set_bool(const std::string& key, bool value) {
    return store->set_value(key, StateValue::from_bool(value));
}

bool StateManager::set_float(const std::string& key, double value) {
    return store->set_value(key, StateValue::from_float(value));
}

std::string StateManager::get_string(const std::string& key, const std::string& default_value) const {
    StateValue value = store->get_value(key);
    if (value.value.empty() && !store->has_key(key)) {
        return default_value;
    }
    return value.to_string();
}

int StateManager::get_int(const std::string& key, int default_value) const {
    StateValue value = store->get_value(key);
    if (value.value.empty() && !store->has_key(key)) {
        return default_value;
    }
    return value.to_int();
}

bool StateManager::get_bool(const std::string& key, bool default_value) const {
    StateValue value = store->get_value(key);
    if (value.value.empty() && !store->has_key(key)) {
        return default_value;
    }
    return value.to_bool();
}

double StateManager::get_float(const std::string& key, double default_value) const {
    StateValue value = store->get_value(key);
    if (value.value.empty() && !store->has_key(key)) {
        return default_value;
    }
    return value.to_float();
}

bool StateManager::has(const std::string& key) const {
    return store->has_key(key);
}

bool StateManager::remove(const std::string& key) {
    return store->remove_key(key);
}

void StateManager::set_multiple(const std::map<std::string, std::string>& values) {
    std::map<std::string, StateValue> state_values;
    for (const auto& [key, value] : values) {
        state_values[key] = StateValue::from_string(value);
    }
    store->set_multiple(state_values);
}

std::map<std::string, std::string> StateManager::get_all_strings() const {
    std::map<std::string, StateValue> all_values = store->get_all();
    std::map<std::string, std::string> result;
    for (const auto& [key, value] : all_values) {
        result[key] = value.to_string();
    }
    return result;
}

bool StateManager::save(const std::string& file_path) const {
    return store->save_to_file(file_path);
}

bool StateManager::load(const std::string& file_path) {
    return store->load_from_file(file_path);
}

json StateManager::snapshot() const {
    return store->create_snapshot();
}

bool StateManager::restore(const json& snapshot) {
    return store->restore_from_snapshot(snapshot);
}

void StateManager::clear() {
    store->clear();
}

size_t StateManager::size() const {
    return store->size();
}

} // namespace cardity 