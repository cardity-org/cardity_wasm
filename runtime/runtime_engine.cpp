#include "runtime_engine.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

RuntimeEngine::RuntimeEngine(const std::string& car_json_path) {
    load_car_file(car_json_path);
    init_default_state();
}

void RuntimeEngine::load_car_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open CAR file: " + path);
    }
    
    try {
        file >> car_data;
    } catch (const json::exception& e) {
        throw std::runtime_error("Invalid JSON in CAR file: " + std::string(e.what()));
    }
    
    // 验证基本结构
    if (!car_data.contains("p") || !car_data.contains("cpl")) {
        throw std::runtime_error("Invalid CAR file structure");
    }
}

void RuntimeEngine::init_default_state() {
    state = {};
    
    if (!car_data.contains("cpl") || !car_data["cpl"].contains("state")) {
        return; // 没有状态定义
    }
    
    auto defaults = car_data["cpl"]["state"];
    for (auto& [key, val] : defaults.items()) {
        if (val.contains("default")) {
            state[key] = val["default"];
        } else {
            state[key] = "";
        }
    }
}

json RuntimeEngine::get_state() const {
    return state;
}

void RuntimeEngine::set_state(const std::string& key, const std::string& value) {
    state[key] = value;
}

std::string RuntimeEngine::get_state_value(const std::string& key) const {
    if (state.contains(key)) {
        return state[key].get<std::string>();
    }
    return "";
}

std::string RuntimeEngine::get_protocol_name() const {
    return car_data.value("protocol", "");
}

std::string RuntimeEngine::get_protocol_version() const {
    return car_data.value("version", "");
}

std::vector<std::string> RuntimeEngine::get_method_names() const {
    std::vector<std::string> names;
    
    if (!car_data.contains("cpl") || !car_data["cpl"].contains("methods")) {
        return names;
    }
    
    auto methods = car_data["cpl"]["methods"];
    for (auto& [name, method] : methods.items()) {
        names.push_back(name);
    }
    
    return names;
}

bool RuntimeEngine::has_method(const std::string& method_name) const {
    if (!car_data.contains("cpl") || !car_data["cpl"].contains("methods")) {
        return false;
    }
    
    return car_data["cpl"]["methods"].contains(method_name);
}

std::string RuntimeEngine::invoke(const std::string& method_name, const std::vector<std::string>& params) {
    if (!car_data.contains("cpl") || !car_data["cpl"].contains("methods")) {
        throw std::runtime_error("No methods defined in protocol");
    }
    
    auto methods = car_data["cpl"]["methods"];
    if (!methods.contains(method_name)) {
        throw std::runtime_error("Method not found: " + method_name);
    }
    
    auto method = methods[method_name];
    
    // 创建参数映射
    std::map<std::string, std::string> param_map;
    if (method.contains("params")) {
        auto expected_params = method["params"];
        if (expected_params.size() != params.size()) {
            throw std::runtime_error("Parameter count mismatch. Expected " + 
                                   std::to_string(expected_params.size()) + 
                                   ", got " + std::to_string(params.size()));
        }
        
        // 创建参数名到值的映射
        for (size_t i = 0; i < expected_params.size(); ++i) {
            std::string param_name = expected_params[i].get<std::string>();
            param_map[param_name] = params[i];
        }
    }
    
    // 执行逻辑
    if (method.contains("logic")) {
        std::string logic;
        if (method["logic"].is_array()) {
            // 如果是数组，连接所有逻辑语句
            for (const auto& stmt : method["logic"]) {
                if (!logic.empty()) logic += "; ";
                logic += stmt.get<std::string>();
            }
        } else {
            logic = method["logic"].get<std::string>();
        }
        
        execute_logic(logic, param_map);
    }
    
    // 处理返回值
    if (method.contains("returns")) {
        std::string return_expr;
        if (method["returns"].is_object() && method["returns"].contains("expr")) {
            return_expr = method["returns"]["expr"].get<std::string>();
        } else {
            return_expr = method["returns"].get<std::string>();
        }
        
        // 解析返回值表达式
        if (return_expr.substr(0, 6) == "state.") {
            std::string state_key = return_expr.substr(6);
            return get_state_value(state_key);
        } else {
            // 如果不是 state. 格式，直接作为状态键名处理
            return get_state_value(return_expr);
        }
        
        return return_expr;
    }
    
    return "OK";
}

void RuntimeEngine::execute_logic(const std::string& logic, const std::map<std::string, std::string>& param_map) {
    // 分割逻辑语句
    std::istringstream iss(logic);
    std::string line;
    
    while (std::getline(iss, line, ';')) {
        // 去除空白字符
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // 跳过 emit 语句（暂时）
        if (line.substr(0, 4) == "emit") {
            continue;
        }
        
        // 处理赋值语句: state.X = params.Y 或 state.X = value
        std::regex assign_pattern(R"(state\.(\w+)\s*=\s*(.+))");
        std::smatch match;
        
        if (std::regex_search(line, match, assign_pattern)) {
            std::string state_key = match[1];
            std::string value_expr = match[2];
            
            // 去除空白字符
            value_expr.erase(0, value_expr.find_first_not_of(" \t"));
            value_expr.erase(value_expr.find_last_not_of(" \t") + 1);
            
            std::string value;
            
            // 检查是否是参数引用: params.new_msg, params.0, etc.
            if (value_expr.substr(0, 7) == "params.") {
                value = resolve_param(value_expr, param_map);
            }
            // 检查是否是状态引用: state.msg, state.count, etc.
            else if (value_expr.substr(0, 6) == "state.") {
                value = resolve_state(value_expr);
            }
            // 检查是否是字面量（去除引号）
            else if ((value_expr.front() == '"' && value_expr.back() == '"') ||
                     (value_expr.front() == '\'' && value_expr.back() == '\'')) {
                value = value_expr.substr(1, value_expr.length() - 2);
            }
            // 其他情况作为字面量
            else {
                value = value_expr;
            }
            
            state[state_key] = value;
        }
    }
}

std::string RuntimeEngine::resolve_param(const std::string& param_ref, const std::map<std::string, std::string>& param_map) {
    // 解析 params.new_msg, params.0, etc.
    std::regex param_pattern(R"(params\.(\w+))");
    std::smatch match;
    
    if (std::regex_search(param_ref, match, param_pattern)) {
        std::string param_name = match[1];
        
        // 首先尝试按名称查找
        auto it = param_map.find(param_name);
        if (it != param_map.end()) {
            return it->second;
        }
        
        // 如果不是名称，尝试按数字索引查找
        try {
            int index = std::stoi(param_name);
            if (index >= 0 && static_cast<size_t>(index) < param_map.size()) {
                // 按索引查找（这里需要转换）
                auto it2 = param_map.begin();
                std::advance(it2, index);
                return it2->second;
            }
        } catch (const std::exception&) {
            // 不是数字，忽略
        }
    }
    
    return "";
}

std::string RuntimeEngine::resolve_state(const std::string& state_ref) {
    // 解析 state.msg, state.count, etc.
    std::regex state_pattern(R"(state\.(\w+))");
    std::smatch match;
    
    if (std::regex_search(state_ref, match, state_pattern)) {
        std::string state_key = match[1];
        return get_state_value(state_key);
    }
    
    return "";
} 