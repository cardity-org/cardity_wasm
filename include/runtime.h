#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "car_loader.h"
#include "state_store.h"
#include "logic_engine.h"

namespace cardity {

using json = nlohmann::json;

// 事件实例
struct EventInstance {
    std::string name;
    std::vector<std::string> values;
    std::string timestamp;
    
    EventInstance() = default;
    EventInstance(const std::string& n, const std::vector<std::string>& v)
        : name(n), values(v) {}
};

// 方法调用结果
struct MethodResult {
    bool success;
    std::string return_value;
    std::vector<EventInstance> events;
    std::string error_message;
    
    MethodResult() : success(false) {}
    MethodResult(bool s, const std::string& ret) : success(s), return_value(ret) {}
};

// 快照信息
struct Snapshot {
    std::string protocol_name;
    std::string version;
    json state;
    std::vector<EventInstance> event_log;
    std::string timestamp;
    std::string block_height;
    
    Snapshot() = default;
};

// 运行时配置
struct RuntimeConfig {
    bool enable_events;
    bool enable_snapshots;
    bool enable_persistence;
    std::string snapshot_interval;
    std::string storage_path;
    
    RuntimeConfig() : enable_events(true), enable_snapshots(true), 
                     enable_persistence(true), snapshot_interval("7d") {}
};

// 主运行时类
class CardityRuntime {
private:
    std::unique_ptr<CarProtocol> protocol;
    std::unique_ptr<StateManager> state_manager;
    std::unique_ptr<LogicEngine> logic_engine;
    std::unique_ptr<StateVariableResolver> variable_resolver;
    
    std::vector<EventInstance> event_log;
    RuntimeConfig config;
    
public:
    CardityRuntime();
    explicit CardityRuntime(const RuntimeConfig& cfg);
    
    // 加载协议
    bool load_protocol(const std::string& car_file_path);
    bool load_protocol_from_json(const std::string& json_str);
    bool load_protocol_from_base64(const std::string& base64_str);
    
    // 执行方法
    MethodResult call_method(const std::string& method_name, const std::vector<std::string>& args);
    MethodResult call_method_with_json(const std::string& method_name, const json& args);
    
    // 状态管理
    bool set_state(const std::string& key, const std::string& value);
    std::string get_state(const std::string& key, const std::string& default_value = "") const;
    json get_all_state() const;
    
    // 事件管理
    void emit_event(const std::string& event_name, const std::vector<std::string>& values);
    std::vector<EventInstance> get_event_log() const;
    void clear_event_log();
    
    // 快照管理
    Snapshot create_snapshot(const std::string& block_height = "") const;
    bool restore_from_snapshot(const Snapshot& snapshot);
    bool save_snapshot_to_file(const std::string& file_path) const;
    bool load_snapshot_from_file(const std::string& file_path);
    
    // 持久化
    bool save_state_to_file(const std::string& file_path) const;
    bool load_state_from_file(const std::string& file_path);
    
    // 验证
    bool validate_protocol() const;
    bool validate_method(const std::string& method_name) const;
    
    // 信息查询
    std::string get_protocol_name() const;
    std::string get_protocol_version() const;
    json get_abi() const;
    std::vector<std::string> get_method_names() const;
    std::vector<std::string> get_state_variables() const;
    
    // 配置
    void set_config(const RuntimeConfig& cfg);
    RuntimeConfig get_config() const;
    
    // 重置
    void reset();
    void reset_state();
    
    // 获取内部组件
    const CarProtocol* get_protocol() const { return protocol.get(); }
    StateManager* get_state_manager() { return state_manager.get(); }
    const StateManager* get_state_manager() const { return state_manager.get(); }
    LogicEngine* get_logic_engine() { return logic_engine.get(); }
    const LogicEngine* get_logic_engine() const { return logic_engine.get(); }

private:
    // 初始化运行时
    void initialize_runtime();
    
    // 解析方法参数
    std::vector<std::string> parse_method_args(const std::string& method_name, const json& args);
    
    // 执行方法逻辑
    std::string execute_method_logic(const std::string& method_name, const std::vector<std::string>& args);
    
    // 处理返回值
    std::string process_return_value(const std::string& method_name, const std::string& logic_result);
    
    // 生成时间戳
    std::string generate_timestamp() const;
};

// WASM 导出接口
#ifdef __EMSCRIPTEN__
extern "C" {
    // 创建运行时实例
    void* create_runtime();
    
    // 销毁运行时实例
    void destroy_runtime(void* runtime);
    
    // 加载协议
    bool load_protocol(void* runtime, const char* car_data);
    
    // 调用方法
    const char* call_method(void* runtime, const char* method_name, const char* args_json);
    
    // 获取状态
    const char* get_state(void* runtime, const char* key);
    
    // 设置状态
    bool set_state(void* runtime, const char* key, const char* value);
    
    // 获取事件日志
    const char* get_event_log(void* runtime);
    
    // 创建快照
    const char* create_snapshot(void* runtime);
    
    // 获取 ABI
    const char* get_abi(void* runtime);
}
#endif

} // namespace cardity 