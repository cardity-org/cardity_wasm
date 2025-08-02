#include "runtime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace cardity {

// CardityRuntime 实现
CardityRuntime::CardityRuntime() : config() {
    initialize_runtime();
}

CardityRuntime::CardityRuntime(const RuntimeConfig& cfg) : config(cfg) {
    initialize_runtime();
}

void CardityRuntime::initialize_runtime() {
    state_manager = std::make_unique<StateManager>();
    variable_resolver = std::make_unique<StateVariableResolver>(state_manager.get());
    logic_engine = std::make_unique<LogicEngine>(std::move(variable_resolver));
    variable_resolver = std::make_unique<StateVariableResolver>(state_manager.get());
    logic_engine->set_resolver(std::move(variable_resolver));
}

bool CardityRuntime::load_protocol(const std::string& car_file_path) {
    protocol = CarLoader::load_from_file(car_file_path);
    if (!protocol) {
        std::cerr << "Failed to load protocol from file: " << car_file_path << std::endl;
        return false;
    }
    
    if (!CarLoader::validate_protocol(*protocol)) {
        std::cerr << "Invalid protocol format" << std::endl;
        return false;
    }
    
    // 初始化状态
    reset_state();
    return true;
}

bool CardityRuntime::load_protocol_from_json(const std::string& json_str) {
    protocol = CarLoader::load_from_json(json_str);
    if (!protocol) {
        std::cerr << "Failed to load protocol from JSON" << std::endl;
        return false;
    }
    
    if (!CarLoader::validate_protocol(*protocol)) {
        std::cerr << "Invalid protocol format" << std::endl;
        return false;
    }
    
    // 初始化状态
    reset_state();
    return true;
}

bool CardityRuntime::load_protocol_from_base64(const std::string& base64_str) {
    protocol = CarLoader::load_from_base64(base64_str);
    if (!protocol) {
        std::cerr << "Failed to load protocol from base64" << std::endl;
        return false;
    }
    
    if (!CarLoader::validate_protocol(*protocol)) {
        std::cerr << "Invalid protocol format" << std::endl;
        return false;
    }
    
    // 初始化状态
    reset_state();
    return true;
}

MethodResult CardityRuntime::call_method(const std::string& method_name, const std::vector<std::string>& args) {
    MethodResult result;
    
    if (!protocol) {
        result.error_message = "No protocol loaded";
        return result;
    }
    
    auto method_it = protocol->cpl.methods.find(method_name);
    if (method_it == protocol->cpl.methods.end()) {
        result.error_message = "Method not found: " + method_name;
        return result;
    }
    
    const Method& method = method_it->second;
    
    // 验证参数数量
    if (args.size() != method.params.size()) {
        result.error_message = "Parameter count mismatch. Expected " + 
                              std::to_string(method.params.size()) + ", got " + 
                              std::to_string(args.size());
        return result;
    }
    
    try {
        // 设置参数
        auto resolver = dynamic_cast<StateVariableResolver*>(logic_engine->get_resolver());
        if (resolver) {
            for (size_t i = 0; i < method.params.size(); ++i) {
                resolver->set_parameter(method.params[i], args[i]);
            }
        }
        
        // 执行逻辑
        if (!method.logic.empty()) {
            std::string logic_result = logic_engine->execute_method_logic(method.logic, args);
            result.return_value = logic_result;
        }
        
        // 处理返回值
        if (!method.returns.empty()) {
            result.return_value = process_return_value(method_name, method.returns);
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error_message = "Error executing method: " + std::string(e.what());
    }
    
    return result;
}

MethodResult CardityRuntime::call_method_with_json(const std::string& method_name, const json& args) {
    std::vector<std::string> string_args = parse_method_args(method_name, args);
    return call_method(method_name, string_args);
}

bool CardityRuntime::set_state(const std::string& key, const std::string& value) {
    if (!state_manager) {
        return false;
    }
    return state_manager->set(key, value);
}

std::string CardityRuntime::get_state(const std::string& key, const std::string& default_value) const {
    if (!state_manager) {
        return default_value;
    }
    return state_manager->get_string(key, default_value);
}

json CardityRuntime::get_all_state() const {
    if (!state_manager) {
        return json::object();
    }
    
    auto all_state = state_manager->get_all_strings();
    json result = json::object();
    for (const auto& [key, value] : all_state) {
        result[key] = value;
    }
    return result;
}

void CardityRuntime::emit_event(const std::string& event_name, const std::vector<std::string>& values) {
    if (!config.enable_events) {
        return;
    }
    
    EventInstance event;
    event.name = event_name;
    event.values = values;
    event.timestamp = generate_timestamp();
    
    event_log.push_back(event);
}

std::vector<EventInstance> CardityRuntime::get_event_log() const {
    return event_log;
}

void CardityRuntime::clear_event_log() {
    event_log.clear();
}

Snapshot CardityRuntime::create_snapshot(const std::string& block_height) const {
    Snapshot snapshot;
    
    if (protocol) {
        snapshot.protocol_name = protocol->protocol;
        snapshot.version = protocol->version;
    }
    
    snapshot.state = get_all_state();
    snapshot.event_log = event_log;
    snapshot.timestamp = generate_timestamp();
    snapshot.block_height = block_height;
    
    return snapshot;
}

bool CardityRuntime::restore_from_snapshot(const Snapshot& snapshot) {
    try {
        // 恢复状态
        if (state_manager) {
            for (const auto& [key, value] : snapshot.state.items()) {
                state_manager->set(key, value.get<std::string>());
            }
        }
        
        // 恢复事件日志
        event_log = snapshot.event_log;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error restoring from snapshot: " << e.what() << std::endl;
        return false;
    }
}

bool CardityRuntime::save_snapshot_to_file(const std::string& file_path) const {
    try {
        Snapshot snapshot = create_snapshot();
        json snapshot_json;
        snapshot_json["protocol_name"] = snapshot.protocol_name;
        snapshot_json["version"] = snapshot.version;
        snapshot_json["state"] = snapshot.state;
        snapshot_json["timestamp"] = snapshot.timestamp;
        snapshot_json["block_height"] = snapshot.block_height;
        
        // 序列化事件日志
        json events_array = json::array();
        for (const auto& event : snapshot.event_log) {
            json event_json;
            event_json["name"] = event.name;
            event_json["values"] = event.values;
            event_json["timestamp"] = event.timestamp;
            events_array.push_back(event_json);
        }
        snapshot_json["event_log"] = events_array;
        
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
            return false;
        }
        
        file << snapshot_json.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving snapshot: " << e.what() << std::endl;
        return false;
    }
}

bool CardityRuntime::load_snapshot_from_file(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        json snapshot_json = json::parse(buffer.str());
        
        Snapshot snapshot;
        snapshot.protocol_name = snapshot_json.value("protocol_name", "");
        snapshot.version = snapshot_json.value("version", "");
        snapshot.state = snapshot_json["state"];
        snapshot.timestamp = snapshot_json.value("timestamp", "");
        snapshot.block_height = snapshot_json.value("block_height", "");
        
        // 反序列化事件日志
        if (snapshot_json.contains("event_log")) {
            for (const auto& event_json : snapshot_json["event_log"]) {
                EventInstance event;
                event.name = event_json["name"].get<std::string>();
                event.values = event_json["values"].get<std::vector<std::string>>();
                event.timestamp = event_json["timestamp"].get<std::string>();
                snapshot.event_log.push_back(event);
            }
        }
        
        return restore_from_snapshot(snapshot);
    } catch (const std::exception& e) {
        std::cerr << "Error loading snapshot: " << e.what() << std::endl;
        return false;
    }
}

bool CardityRuntime::save_state_to_file(const std::string& file_path) const {
    if (!state_manager) {
        return false;
    }
    return state_manager->save(file_path);
}

bool CardityRuntime::load_state_from_file(const std::string& file_path) {
    if (!state_manager) {
        return false;
    }
    return state_manager->load(file_path);
}

bool CardityRuntime::validate_protocol() const {
    if (!protocol) {
        return false;
    }
    return CarLoader::validate_protocol(*protocol);
}

bool CardityRuntime::validate_method(const std::string& method_name) const {
    if (!protocol) {
        return false;
    }
    return protocol->cpl.methods.find(method_name) != protocol->cpl.methods.end();
}

std::string CardityRuntime::get_protocol_name() const {
    if (!protocol) {
        return "";
    }
    return protocol->protocol;
}

std::string CardityRuntime::get_protocol_version() const {
    if (!protocol) {
        return "";
    }
    return protocol->version;
}

json CardityRuntime::get_abi() const {
    if (!protocol) {
        return json::object();
    }
    return protocol->abi;
}

std::vector<std::string> CardityRuntime::get_method_names() const {
    std::vector<std::string> names;
    if (protocol) {
        for (const auto& [name, method] : protocol->cpl.methods) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<std::string> CardityRuntime::get_state_variables() const {
    std::vector<std::string> names;
    if (protocol) {
        for (const auto& [name, var] : protocol->cpl.state) {
            names.push_back(name);
        }
    }
    return names;
}

void CardityRuntime::set_config(const RuntimeConfig& cfg) {
    config = cfg;
}

RuntimeConfig CardityRuntime::get_config() const {
    return config;
}

void CardityRuntime::reset() {
    protocol.reset();
    reset_state();
    clear_event_log();
}

void CardityRuntime::reset_state() {
    if (!protocol || !state_manager) {
        return;
    }
    
    // 清空当前状态
    state_manager->clear();
    
    // 初始化默认状态
    for (const auto& [name, var] : protocol->cpl.state) {
        state_manager->set(name, var.default_value);
    }
}

std::vector<std::string> CardityRuntime::parse_method_args(const std::string& method_name, const json& args) {
    std::vector<std::string> string_args;
    
    if (!protocol) {
        return string_args;
    }
    
    auto method_it = protocol->cpl.methods.find(method_name);
    if (method_it == protocol->cpl.methods.end()) {
        return string_args;
    }
    
    const Method& method = method_it->second;
    
    if (args.is_array()) {
        for (const auto& arg : args) {
            if (arg.is_string()) {
                string_args.push_back(arg.get<std::string>());
            } else {
                string_args.push_back(arg.dump());
            }
        }
    } else if (args.is_object()) {
        // 如果是对象，按参数名匹配
        for (const auto& param_name : method.params) {
            if (args.contains(param_name)) {
                const auto& arg = args[param_name];
                if (arg.is_string()) {
                    string_args.push_back(arg.get<std::string>());
                } else {
                    string_args.push_back(arg.dump());
                }
            } else {
                string_args.push_back(""); // 默认空值
            }
        }
    }
    
    return string_args;
}

std::string CardityRuntime::execute_method_logic(const std::string& method_name, const std::vector<std::string>& args) {
    if (!logic_engine) {
        return "";
    }
    
    auto method_it = protocol->cpl.methods.find(method_name);
    if (method_it == protocol->cpl.methods.end()) {
        return "";
    }
    
    return logic_engine->execute_method_logic(method_it->second.logic, args);
}

std::string CardityRuntime::process_return_value(const std::string& method_name, const std::string& return_expr) {
    (void)method_name; // 避免未使用参数警告
    if (!logic_engine) {
        return "";
    }
    
    return logic_engine->evaluate_expression(return_expr);
}

std::string CardityRuntime::generate_timestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// WASM 导出接口实现
#ifdef __EMSCRIPTEN__
extern "C" {
    void* create_runtime() {
        return new CardityRuntime();
    }
    
    void destroy_runtime(void* runtime) {
        delete static_cast<CardityRuntime*>(runtime);
    }
    
    bool load_protocol(void* runtime, const char* car_data) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        return rt->load_protocol_from_json(car_data);
    }
    
    const char* call_method(void* runtime, const char* method_name, const char* args_json) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        json args = json::parse(args_json);
        MethodResult result = rt->call_method_with_json(method_name, args);
        
        json response;
        response["success"] = result.success;
        response["return_value"] = result.return_value;
        response["error_message"] = result.error_message;
        
        std::string* response_str = new std::string(response.dump());
        return response_str->c_str();
    }
    
    const char* get_state(void* runtime, const char* key) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        std::string* value = new std::string(rt->get_state(key));
        return value->c_str();
    }
    
    bool set_state(void* runtime, const char* key, const char* value) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        return rt->set_state(key, value);
    }
    
    const char* get_event_log(void* runtime) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        auto events = rt->get_event_log();
        
        json events_json = json::array();
        for (const auto& event : events) {
            json event_json;
            event_json["name"] = event.name;
            event_json["values"] = event.values;
            event_json["timestamp"] = event.timestamp;
            events_json.push_back(event_json);
        }
        
        std::string* events_str = new std::string(events_json.dump());
        return events_str->c_str();
    }
    
    const char* create_snapshot(void* runtime) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        Snapshot snapshot = rt->create_snapshot();
        
        json snapshot_json;
        snapshot_json["protocol_name"] = snapshot.protocol_name;
        snapshot_json["version"] = snapshot.version;
        snapshot_json["state"] = snapshot.state;
        snapshot_json["timestamp"] = snapshot.timestamp;
        snapshot_json["block_height"] = snapshot.block_height;
        
        json events_array = json::array();
        for (const auto& event : snapshot.event_log) {
            json event_json;
            event_json["name"] = event.name;
            event_json["values"] = event.values;
            event_json["timestamp"] = event.timestamp;
            events_array.push_back(event_json);
        }
        snapshot_json["event_log"] = events_array;
        
        std::string* snapshot_str = new std::string(snapshot_json.dump());
        return snapshot_str->c_str();
    }
    
    const char* get_abi(void* runtime) {
        auto* rt = static_cast<CardityRuntime*>(runtime);
        json abi = rt->get_abi();
        std::string* abi_str = new std::string(abi.dump());
        return abi_str->c_str();
    }
}
#endif

} // namespace cardity 