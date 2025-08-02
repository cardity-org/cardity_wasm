#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RuntimeEngine {
public:
    RuntimeEngine(const std::string& car_json_path);

    // 运行指定方法
    std::string invoke(const std::string& method_name, const std::vector<std::string>& params);

    // 获取当前状态
    json get_state() const;

    // 设置状态值
    void set_state(const std::string& key, const std::string& value);

    // 获取状态值
    std::string get_state_value(const std::string& key) const;

    // 获取协议信息
    std::string get_protocol_name() const;
    std::string get_protocol_version() const;

    // 获取可用方法列表
    std::vector<std::string> get_method_names() const;

    // 验证方法是否存在
    bool has_method(const std::string& method_name) const;

private:
    json car_data;
    json state;

    void load_car_file(const std::string& path);
    void init_default_state();
    void execute_logic(const std::string& logic, const std::map<std::string, std::string>& param_map);
    
    // 解析参数引用 (params.new_msg, params.0, etc.)
    std::string resolve_param(const std::string& param_ref, const std::map<std::string, std::string>& param_map);
    
    // 解析状态引用 (state.msg, state.count, etc.)
    std::string resolve_state(const std::string& state_ref);
}; 