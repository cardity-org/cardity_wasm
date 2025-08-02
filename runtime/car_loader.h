#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace cardity {

using json = nlohmann::json;

// 状态变量定义
struct StateVariable {
    std::string type;
    std::string default_value;
    
    StateVariable() = default;
    StateVariable(const std::string& t, const std::string& def) 
        : type(t), default_value(def) {}
};

// 方法定义
struct Method {
    std::vector<std::string> params;
    std::string logic;
    std::string returns;
    
    Method() = default;
    Method(const std::vector<std::string>& p, const std::string& l, const std::string& r)
        : params(p), logic(l), returns(r) {}
};

// 事件定义
struct Event {
    std::vector<std::string> params;
    
    Event() = default;
    Event(const std::vector<std::string>& p) : params(p) {}
};

// CPL (Cardity Protocol Logic) 结构
struct CPL {
    std::map<std::string, StateVariable> state;
    std::map<std::string, Method> methods;
    std::map<std::string, Event> events;
    std::string owner;
    
    CPL() = default;
};

// 完整的 .car 协议结构
struct CarProtocol {
    std::string p;           // "cardinals"
    std::string op;          // "deploy"
    std::string protocol;    // 协议名称
    std::string version;     // 版本
    CPL cpl;                 // 协议逻辑
    json abi;                // ABI 接口
    std::string hash;        // 文件哈希
    std::string signature;   // 签名（可选）
    
    CarProtocol() = default;
};

// .car 文件加载器
class CarLoader {
public:
    // 从文件加载协议
    static std::unique_ptr<CarProtocol> load_from_file(const std::string& file_path);
    
    // 从 JSON 字符串加载协议
    static std::unique_ptr<CarProtocol> load_from_json(const std::string& json_str);
    
    // 从 base64 编码的字符串加载协议
    static std::unique_ptr<CarProtocol> load_from_base64(const std::string& base64_str);
    
    // 验证协议格式
    static bool validate_protocol(const CarProtocol& protocol);
    
    // 导出协议为 JSON
    static json export_to_json(const CarProtocol& protocol);
    
    // 导出协议为 base64
    static std::string export_to_base64(const CarProtocol& protocol);

private:
    // 解析状态定义
    static void parse_state(const json& state_json, CPL& cpl);
    
    // 解析方法定义
    static void parse_methods(const json& methods_json, CPL& cpl);
    
    // 解析事件定义
    static void parse_events(const json& events_json, CPL& cpl);
    
    // 生成 ABI
    static json generate_abi(const CPL& cpl, const std::string& protocol_name, const std::string& version);
    
    // 计算哈希
    static std::string calculate_hash(const json& data);
};

} // namespace cardity 