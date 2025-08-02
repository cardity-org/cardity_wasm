#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "state_store.h"

namespace cardity {

// 表达式类型
enum class ExpressionType {
    LITERAL,
    VARIABLE,
    BINARY_OP,
    UNARY_OP,
    FUNCTION_CALL
};

// 操作符类型
enum class OperatorType {
    ADD,        // +
    SUB,        // -
    MUL,        // *
    DIV,        // /
    MOD,        // %
    EQ,         // ==
    NE,         // !=
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    AND,        // &&
    OR,         // ||
    NOT,        // !
    ASSIGN      // =
};

// 表达式节点
struct ExpressionNode {
    ExpressionType type;
    std::string value;
    OperatorType op;
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    
    ExpressionNode() : type(ExpressionType::LITERAL), op(OperatorType::ADD) {}
    ExpressionNode(ExpressionType t, const std::string& v) 
        : type(t), value(v), op(OperatorType::ADD) {}
};

// 变量解析器
class VariableResolver {
public:
    virtual ~VariableResolver() = default;
    
    // 解析变量值
    virtual std::string resolve_variable(const std::string& name) const = 0;
    
    // 设置变量值
    virtual void set_variable(const std::string& name, const std::string& value) = 0;
    
    // 检查变量是否存在
    virtual bool has_variable(const std::string& name) const = 0;
};

// 逻辑引擎
class LogicEngine {
private:
    std::unique_ptr<VariableResolver> resolver;
    
public:
    LogicEngine();
    explicit LogicEngine(std::unique_ptr<VariableResolver> var_resolver);
    
    // 解析表达式
    std::unique_ptr<ExpressionNode> parse_expression(const std::string& expression);
    
    // 执行表达式
    std::string evaluate_expression(const std::string& expression);
    std::string evaluate_node(const ExpressionNode& node);
    
    // 执行赋值语句
    bool execute_assignment(const std::string& assignment);
    
    // 执行条件语句
    bool execute_condition(const std::string& condition);
    
    // 执行方法逻辑
    std::string execute_method_logic(const std::string& logic, const std::vector<std::string>& args);
    
    // 解析参数
    std::vector<std::string> parse_parameters(const std::string& param_str);
    
    // 设置变量解析器
    void set_resolver(std::unique_ptr<VariableResolver> var_resolver);
    
    // 获取变量解析器
    VariableResolver* get_resolver() { return resolver.get(); }
    const VariableResolver* get_resolver() const { return resolver.get(); }

private:
    // 解析操作符
    OperatorType parse_operator(const std::string& op_str);
    
    // 解析字面量
    std::string parse_literal(const std::string& literal);
    
    // 解析变量引用
    std::string parse_variable(const std::string& var_name);
    
    // 执行二元操作
    std::string execute_binary_op(OperatorType op, const std::string& left, const std::string& right);
    
    // 执行一元操作
    std::string execute_unary_op(OperatorType op, const std::string& operand);
    
    // 类型转换
    bool string_to_bool(const std::string& value) const;
    int string_to_int(const std::string& value) const;
    double string_to_float(const std::string& value) const;
    std::string bool_to_string(bool value) const;
    std::string int_to_string(int value) const;
    std::string float_to_string(double value) const;
};

// 状态变量解析器
class StateVariableResolver : public VariableResolver {
private:
    StateManager* state_manager;
    std::map<std::string, std::string> params;
    
public:
    explicit StateVariableResolver(StateManager* manager);
    
    // 设置参数
    void set_parameters(const std::map<std::string, std::string>& parameters);
    void set_parameter(const std::string& name, const std::string& value);
    
    // 实现 VariableResolver 接口
    std::string resolve_variable(const std::string& name) const override;
    void set_variable(const std::string& name, const std::string& value) override;
    bool has_variable(const std::string& name) const override;
    
    // 获取状态管理器
    StateManager* get_state_manager() { return state_manager; }
    const StateManager* get_state_manager() const { return state_manager; }
};

} // namespace cardity 