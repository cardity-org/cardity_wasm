#include "car_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace cardity {

// CarLoader 实现
std::unique_ptr<CarProtocol> CarLoader::load_from_file(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            return nullptr;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return load_from_json(buffer.str());
    } catch (const std::exception& e) {
        std::cerr << "Error loading file: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<CarProtocol> CarLoader::load_from_json(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        auto protocol = std::make_unique<CarProtocol>();
        
        // 解析基本字段
        protocol->p = j.value("p", "");
        protocol->op = j.value("op", "");
        protocol->protocol = j.value("protocol", "");
        protocol->version = j.value("version", "");
        protocol->hash = j.value("hash", "");
        protocol->signature = j.value("signature", "");
        
        // 解析 CPL
        if (j.contains("cpl")) {
            const auto& cpl_json = j["cpl"];
            
            // 解析状态
            if (cpl_json.contains("state")) {
                parse_state(cpl_json["state"], protocol->cpl);
            }
            
            // 解析方法
            if (cpl_json.contains("methods")) {
                parse_methods(cpl_json["methods"], protocol->cpl);
            }
            
            // 解析事件
            if (cpl_json.contains("events")) {
                parse_events(cpl_json["events"], protocol->cpl);
            }
            
            // 解析所有者
            protocol->cpl.owner = cpl_json.value("owner", "");
        }
        
        // 生成 ABI
        protocol->abi = generate_abi(protocol->cpl, protocol->protocol, protocol->version);
        
        // 计算哈希（如果没有提供）
        if (protocol->hash.empty()) {
            protocol->hash = calculate_hash(j);
        }
        
        return protocol;
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return nullptr;
    } catch (const std::exception& e) {
        std::cerr << "Error loading from JSON: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<CarProtocol> CarLoader::load_from_base64(const std::string& base64_str) {
    // 简单的 base64 解码实现
    std::string decoded;
    try {
        // 这里应该使用 proper base64 解码库
        // 为了简化，我们假设输入已经是 JSON 字符串
        decoded = base64_str;
        return load_from_json(decoded);
    } catch (const std::exception& e) {
        std::cerr << "Base64 decoding error: " << e.what() << std::endl;
        return nullptr;
    }
}

bool CarLoader::validate_protocol(const CarProtocol& protocol) {
    // 验证基本字段
    if (protocol.p != "cardinals") {
        std::cerr << "Invalid protocol type: " << protocol.p << std::endl;
        return false;
    }
    
    if (protocol.op != "deploy") {
        std::cerr << "Invalid operation: " << protocol.op << std::endl;
        return false;
    }
    
    if (protocol.protocol.empty()) {
        std::cerr << "Protocol name is empty" << std::endl;
        return false;
    }
    
    if (protocol.version.empty()) {
        std::cerr << "Protocol version is empty" << std::endl;
        return false;
    }
    
    if (protocol.cpl.owner.empty()) {
        std::cerr << "Protocol owner is empty" << std::endl;
        return false;
    }
    
    // 验证状态变量
    for (const auto& [name, var] : protocol.cpl.state) {
        if (var.type.empty()) {
            std::cerr << "State variable " << name << " has empty type" << std::endl;
            return false;
        }
    }
    
    // 验证方法
    for (const auto& [name, method] : protocol.cpl.methods) {
        if (method.logic.empty() && method.returns.empty()) {
            std::cerr << "Method " << name << " has no logic or return value" << std::endl;
            return false;
        }
    }
    
    return true;
}

json CarLoader::export_to_json(const CarProtocol& protocol) {
    json j;
    j["p"] = protocol.p;
    j["op"] = protocol.op;
    j["protocol"] = protocol.protocol;
    j["version"] = protocol.version;
    j["hash"] = protocol.hash;
    j["signature"] = protocol.signature;
    
    // 导出 CPL
    json cpl;
    
    // 导出状态
    json state;
    for (const auto& [name, var] : protocol.cpl.state) {
        state[name]["type"] = var.type;
        state[name]["default"] = var.default_value;
    }
    cpl["state"] = state;
    
    // 导出方法
    json methods;
    for (const auto& [name, method] : protocol.cpl.methods) {
        json method_json;
        method_json["params"] = method.params;
        if (!method.logic.empty()) {
            method_json["logic"] = method.logic;
        }
        if (!method.returns.empty()) {
            method_json["returns"] = method.returns;
        }
        methods[name] = method_json;
    }
    cpl["methods"] = methods;
    
    // 导出事件
    json events;
    for (const auto& [name, event] : protocol.cpl.events) {
        events[name]["params"] = event.params;
    }
    cpl["events"] = events;
    
    cpl["owner"] = protocol.cpl.owner;
    j["cpl"] = cpl;
    
    // 导出 ABI
    j["abi"] = protocol.abi;
    
    return j;
}

std::string CarLoader::export_to_base64(const CarProtocol& protocol) {
    json j = export_to_json(protocol);
    std::string json_str = j.dump();
    // 这里应该使用 proper base64 编码库
    // 为了简化，直接返回 JSON 字符串
    return json_str;
}

void CarLoader::parse_state(const json& state_json, CPL& cpl) {
    for (const auto& [name, var_json] : state_json.items()) {
        StateVariable var;
        var.type = var_json.value("type", "string");
        var.default_value = var_json.value("default", "");
        cpl.state[name] = var;
    }
}

void CarLoader::parse_methods(const json& methods_json, CPL& cpl) {
    for (const auto& [name, method_json] : methods_json.items()) {
        Method method;
        
        // 解析参数
        if (method_json.contains("params")) {
            method.params = method_json["params"].get<std::vector<std::string>>();
        }
        
        // 解析逻辑
        if (method_json.contains("logic")) {
            if (method_json["logic"].is_array()) {
                // 如果是数组，连接所有逻辑语句
                std::string logic;
                for (const auto& stmt : method_json["logic"]) {
                    if (!logic.empty()) logic += "; ";
                    logic += stmt.get<std::string>();
                }
                method.logic = logic;
            } else {
                method.logic = method_json["logic"].get<std::string>();
            }
            
            // 调试信息
            if (name == "increment") {
                std::cerr << "DEBUG: Increment method logic: '" << method.logic << "'" << std::endl;
            }
        }
        
        // 解析返回值
        if (method_json.contains("returns")) {
            if (method_json["returns"].is_object()) {
                // 如果是对象，提取表达式
                if (method_json["returns"].contains("expr")) {
                    method.returns = method_json["returns"]["expr"].get<std::string>();
                }
            } else {
                method.returns = method_json["returns"].get<std::string>();
            }
        }
        
        cpl.methods[name] = method;
    }
}

void CarLoader::parse_events(const json& events_json, CPL& cpl) {
    for (const auto& [name, event_json] : events_json.items()) {
        Event event;
        if (event_json.contains("params")) {
            for (const auto& param : event_json["params"]) {
                if (param.is_object() && param.contains("name")) {
                    event.params.push_back(param["name"].get<std::string>());
                } else if (param.is_string()) {
                    event.params.push_back(param.get<std::string>());
                }
            }
        }
        cpl.events[name] = event;
    }
}

json CarLoader::generate_abi(const CPL& cpl, const std::string& protocol_name, const std::string& version) {
    json abi;
    abi["protocol"] = protocol_name;
    abi["version"] = version;
    abi["methods"] = json::array();
    abi["events"] = json::array();
    abi["state"] = json::array();
    
    // 生成方法 ABI
    for (const auto& [name, method] : cpl.methods) {
        json method_abi;
        method_abi["name"] = name;
        method_abi["params"] = method.params;
        if (!method.returns.empty()) {
            method_abi["returns"] = method.returns;
        }
        abi["methods"].push_back(method_abi);
    }
    
    // 生成事件 ABI
    for (const auto& [name, event] : cpl.events) {
        json event_abi;
        event_abi["name"] = name;
        event_abi["params"] = event.params;
        abi["events"].push_back(event_abi);
    }
    
    // 生成状态 ABI
    for (const auto& [name, var] : cpl.state) {
        json state_abi;
        state_abi["name"] = name;
        state_abi["type"] = var.type;
        state_abi["default"] = var.default_value;
        abi["state"].push_back(state_abi);
    }
    
    return abi;
}

std::string CarLoader::calculate_hash(const json& data) {
    // 简单的哈希计算实现
    // 在实际应用中应该使用 SHA256 等加密哈希
    std::string data_str = data.dump();
    std::hash<std::string> hasher;
    return std::to_string(hasher(data_str));
}

} // namespace cardity 