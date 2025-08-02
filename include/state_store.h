#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace cardity {

using json = nlohmann::json;

// 状态值类型
enum class ValueType {
    STRING,
    INT,
    BOOL,
    FLOAT
};

// 状态值
struct StateValue {
    ValueType type;
    std::string value;
    
    StateValue() : type(ValueType::STRING), value("") {}
    StateValue(ValueType t, const std::string& v) : type(t), value(v) {}
    
    // 类型转换
    std::string to_string() const { return value; }
    int to_int() const;
    bool to_bool() const;
    double to_float() const;
    
    // 从字符串创建
    static StateValue from_string(const std::string& val);
    static StateValue from_int(int val);
    static StateValue from_bool(bool val);
    static StateValue from_float(double val);
};

// 状态存储接口
class StateStore {
public:
    virtual ~StateStore() = default;
    
    // 基本操作
    virtual bool set_value(const std::string& key, const StateValue& value) = 0;
    virtual StateValue get_value(const std::string& key) const = 0;
    virtual bool has_key(const std::string& key) const = 0;
    virtual bool remove_key(const std::string& key) = 0;
    
    // 批量操作
    virtual void set_multiple(const std::map<std::string, StateValue>& values) = 0;
    virtual std::map<std::string, StateValue> get_all() const = 0;
    
    // 持久化
    virtual bool save_to_file(const std::string& file_path) const = 0;
    virtual bool load_from_file(const std::string& file_path) = 0;
    
    // 快照
    virtual json create_snapshot() const = 0;
    virtual bool restore_from_snapshot(const json& snapshot) = 0;
    
    // 清空
    virtual void clear() = 0;
    
    // 获取大小
    virtual size_t size() const = 0;
};

// 内存状态存储（默认实现）
class MemoryStateStore : public StateStore {
private:
    std::map<std::string, StateValue> state;
    
public:
    MemoryStateStore() = default;
    
    // 实现 StateStore 接口
    bool set_value(const std::string& key, const StateValue& value) override;
    StateValue get_value(const std::string& key) const override;
    bool has_key(const std::string& key) const override;
    bool remove_key(const std::string& key) override;
    
    void set_multiple(const std::map<std::string, StateValue>& values) override;
    std::map<std::string, StateValue> get_all() const override;
    
    bool save_to_file(const std::string& file_path) const override;
    bool load_from_file(const std::string& file_path) override;
    
    json create_snapshot() const override;
    bool restore_from_snapshot(const json& snapshot) override;
    
    void clear() override;
    size_t size() const override;
    
    // 初始化状态
    void initialize_from_protocol(const std::map<std::string, std::string>& state_def);
};

// 状态管理器
class StateManager {
private:
    std::unique_ptr<StateStore> store;
    
public:
    StateManager();
    explicit StateManager(std::unique_ptr<StateStore> state_store);
    
    // 基本操作
    bool set(const std::string& key, const std::string& value);
    bool set_int(const std::string& key, int value);
    bool set_bool(const std::string& key, bool value);
    bool set_float(const std::string& key, double value);
    
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    double get_float(const std::string& key, double default_value = 0.0) const;
    
    bool has(const std::string& key) const;
    bool remove(const std::string& key);
    
    // 批量操作
    void set_multiple(const std::map<std::string, std::string>& values);
    std::map<std::string, std::string> get_all_strings() const;
    
    // 持久化
    bool save(const std::string& file_path) const;
    bool load(const std::string& file_path);
    
    // 快照
    json snapshot() const;
    bool restore(const json& snapshot);
    
    // 清空
    void clear();
    
    // 获取大小
    size_t size() const;
    
    // 获取底层存储
    StateStore* get_store() { return store.get(); }
    const StateStore* get_store() const { return store.get(); }
};

} // namespace cardity 