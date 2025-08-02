#include "logic_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace cardity {

// LogicEngine 实现
LogicEngine::LogicEngine() : resolver(nullptr) {}

LogicEngine::LogicEngine(std::unique_ptr<VariableResolver> var_resolver) 
    : resolver(std::move(var_resolver)) {}

std::unique_ptr<ExpressionNode> LogicEngine::parse_expression(const std::string& expression) {
    // 简单的表达式解析器
    // 这里实现一个基础的解析器，支持基本的操作
    auto node = std::make_unique<ExpressionNode>();
    
    // 检查是否是赋值操作
    size_t assign_pos = expression.find('=');
    if (assign_pos != std::string::npos) {
        node->type = ExpressionType::BINARY_OP;
        node->op = OperatorType::ASSIGN;
        node->left = std::make_unique<ExpressionNode>(ExpressionType::VARIABLE, 
                                                     expression.substr(0, assign_pos));
        node->right = std::make_unique<ExpressionNode>(ExpressionType::LITERAL, 
                                                      expression.substr(assign_pos + 1));
        return node;
    }
    
    // 检查是否是变量引用
    if (expression.find('.') != std::string::npos || 
        (expression.length() > 0 && std::isalpha(expression[0]))) {
        node->type = ExpressionType::VARIABLE;
        node->value = expression;
        return node;
    }
    
    // 默认为字面量
    node->type = ExpressionType::LITERAL;
    node->value = expression;
    return node;
}

std::string LogicEngine::evaluate_expression(const std::string& expression) {
    if (!resolver) {
        std::cerr << "No variable resolver set" << std::endl;
        return "";
    }
    
    auto node = parse_expression(expression);
    return evaluate_node(*node);
}

std::string LogicEngine::evaluate_node(const ExpressionNode& node) {
    switch (node.type) {
        case ExpressionType::LITERAL:
            return parse_literal(node.value);
            
        case ExpressionType::VARIABLE:
            return parse_variable(node.value);
            
        case ExpressionType::BINARY_OP:
            if (node.left && node.right) {
                std::string left_val = evaluate_node(*node.left);
                std::string right_val = evaluate_node(*node.right);
                return execute_binary_op(node.op, left_val, right_val);
            }
            break;
            
        case ExpressionType::UNARY_OP:
            if (node.left) {
                std::string operand = evaluate_node(*node.left);
                return execute_unary_op(node.op, operand);
            }
            break;
            
        default:
            break;
    }
    
    return "";
}

bool LogicEngine::execute_assignment(const std::string& assignment) {
    if (!resolver) {
        std::cerr << "No variable resolver set" << std::endl;
        return false;
    }
    
    size_t assign_pos = assignment.find('=');
    if (assign_pos == std::string::npos) {
        std::cerr << "Invalid assignment: " << assignment << std::endl;
        return false;
    }
    
    std::string var_name = assignment.substr(0, assign_pos);
    std::string value_expr = assignment.substr(assign_pos + 1);
    
    // 去除空白字符
    var_name.erase(0, var_name.find_first_not_of(" \t"));
    var_name.erase(var_name.find_last_not_of(" \t") + 1);
    value_expr.erase(0, value_expr.find_first_not_of(" \t"));
    value_expr.erase(value_expr.find_last_not_of(" \t") + 1);
    
    // 计算值
    std::string value = evaluate_expression(value_expr);
    
    // 设置变量
    resolver->set_variable(var_name, value);
    return true;
}

bool LogicEngine::execute_condition(const std::string& condition) {
    std::string result = evaluate_expression(condition);
    return string_to_bool(result);
}

std::string LogicEngine::execute_method_logic(const std::string& logic, const std::vector<std::string>& args) {
    (void)args; // 避免未使用参数警告
    if (!resolver) {
        std::cerr << "No variable resolver set" << std::endl;
        return "";
    }
    
    // 参数已经在 runtime.cpp 中设置，这里不需要重复设置
    
    // 解析逻辑语句
    std::istringstream iss(logic);
    std::string line;
    std::string last_result;
    
    while (std::getline(iss, line, ';')) {
        // 去除空白字符
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        std::cerr << "DEBUG: Processing line: '" << line << "'" << std::endl;
        
        // 检查是否是 emit 语句
        if (line.substr(0, 4) == "emit") {
            // 暂时跳过 emit 语句，后续会处理
            continue;
        }
        
        // 检查是否是条件语句
        if (line.length() >= 2 && line.substr(0, 2) == "if") {
            // 简单的条件语句处理
            size_t open_brace = line.find('{');
            size_t close_brace = line.find_last_of('}');
            
            if (open_brace != std::string::npos && close_brace != std::string::npos) {
                std::string condition = line.substr(2, open_brace - 2);
                std::string body = line.substr(open_brace + 1, close_brace - open_brace - 1);
                
                // 去除条件中的括号
                condition.erase(0, condition.find_first_not_of(" \t("));
                condition.erase(condition.find_last_not_of(" \t)") + 1);
                
                // 调试信息
                std::cerr << "DEBUG: Condition: '" << condition << "'" << std::endl;
                std::cerr << "DEBUG: Body: '" << body << "'" << std::endl;
                
                if (execute_condition(condition)) {
                    std::cerr << "DEBUG: Condition is true, executing body" << std::endl;
                    // 执行条件体
                    std::istringstream body_iss(body);
                    std::string body_line;
                    while (std::getline(body_iss, body_line, ';')) {
                        body_line.erase(0, body_line.find_first_not_of(" \t"));
                        body_line.erase(body_line.find_last_not_of(" \t") + 1);
                        
                        if (body_line.empty()) continue;
                        
                        std::cerr << "DEBUG: Executing body line: '" << body_line << "'" << std::endl;
                        
                        if (body_line.substr(0, 4) == "emit") {
                            // 处理 emit 语句
                            std::cerr << "DEBUG: Skipping emit statement" << std::endl;
                            continue;
                        }
                        
                        if (body_line.find('=') != std::string::npos) {
                            execute_assignment(body_line);
                        } else {
                            last_result = evaluate_expression(body_line);
                        }
                    }
                } else {
                    std::cerr << "DEBUG: Condition is false, skipping body" << std::endl;
                }
            }
            continue;
        }
        
        // 检查是否是赋值
        if (line.find('=') != std::string::npos) {
            execute_assignment(line);
        } else {
            // 计算表达式
            last_result = evaluate_expression(line);
        }
    }
    
    return last_result;
}

std::vector<std::string> LogicEngine::parse_parameters(const std::string& param_str) {
    std::vector<std::string> params;
    std::istringstream iss(param_str);
    std::string param;
    
    while (std::getline(iss, param, ',')) {
        param.erase(0, param.find_first_not_of(" \t"));
        param.erase(param.find_last_not_of(" \t") + 1);
        if (!param.empty()) {
            params.push_back(param);
        }
    }
    
    return params;
}

void LogicEngine::set_resolver(std::unique_ptr<VariableResolver> var_resolver) {
    resolver = std::move(var_resolver);
}

OperatorType LogicEngine::parse_operator(const std::string& op_str) {
    if (op_str == "+") return OperatorType::ADD;
    if (op_str == "-") return OperatorType::SUB;
    if (op_str == "*") return OperatorType::MUL;
    if (op_str == "/") return OperatorType::DIV;
    if (op_str == "%") return OperatorType::MOD;
    if (op_str == "==") return OperatorType::EQ;
    if (op_str == "!=") return OperatorType::NE;
    if (op_str == "<") return OperatorType::LT;
    if (op_str == ">") return OperatorType::GT;
    if (op_str == "<=") return OperatorType::LE;
    if (op_str == ">=") return OperatorType::GE;
    if (op_str == "&&") return OperatorType::AND;
    if (op_str == "||") return OperatorType::OR;
    if (op_str == "!") return OperatorType::NOT;
    if (op_str == "=") return OperatorType::ASSIGN;
    
    return OperatorType::ADD; // 默认
}

std::string LogicEngine::parse_literal(const std::string& literal) {
    // 去除引号
    std::string result = literal;
    if (result.length() >= 2 && 
        ((result[0] == '"' && result[result.length()-1] == '"') ||
         (result[0] == '\'' && result[result.length()-1] == '\''))) {
        result = result.substr(1, result.length() - 2);
    }
    return result;
}

std::string LogicEngine::parse_variable(const std::string& var_name) {
    if (!resolver) {
        return "";
    }
    
    // 处理 state.xxx 格式
    if (var_name.substr(0, 6) == "state.") {
        std::string state_var = var_name.substr(6);
        return resolver->resolve_variable(state_var);
    }
    
    // 处理 params.xxx 格式
    if (var_name.substr(0, 7) == "params.") {
        std::string param_name = var_name.substr(7);
        return resolver->resolve_variable(param_name);
    }
    
    return resolver->resolve_variable(var_name);
}

std::string LogicEngine::execute_binary_op(OperatorType op, const std::string& left, const std::string& right) {
    switch (op) {
        case OperatorType::ADD:
            return std::to_string(string_to_float(left) + string_to_float(right));
            
        case OperatorType::SUB:
            return std::to_string(string_to_float(left) - string_to_float(right));
            
        case OperatorType::MUL:
            return std::to_string(string_to_float(left) * string_to_float(right));
            
        case OperatorType::DIV:
            if (string_to_float(right) == 0) return "0";
            return std::to_string(string_to_float(left) / string_to_float(right));
            
        case OperatorType::MOD:
            return std::to_string(string_to_int(left) % string_to_int(right));
            
        case OperatorType::EQ:
            return bool_to_string(left == right);
            
        case OperatorType::NE:
            return bool_to_string(left != right);
            
        case OperatorType::LT:
            return bool_to_string(string_to_float(left) < string_to_float(right));
            
        case OperatorType::GT:
            return bool_to_string(string_to_float(left) > string_to_float(right));
            
        case OperatorType::LE:
            return bool_to_string(string_to_float(left) <= string_to_float(right));
            
        case OperatorType::GE:
            return bool_to_string(string_to_float(left) >= string_to_float(right));
            
        case OperatorType::AND:
            return bool_to_string(string_to_bool(left) && string_to_bool(right));
            
        case OperatorType::OR:
            return bool_to_string(string_to_bool(left) || string_to_bool(right));
            
        case OperatorType::ASSIGN:
            if (resolver) {
                resolver->set_variable(left, right);
            }
            return right;
            
        default:
            return left;
    }
}

std::string LogicEngine::execute_unary_op(OperatorType op, const std::string& operand) {
    switch (op) {
        case OperatorType::NOT:
            return bool_to_string(!string_to_bool(operand));
            
        case OperatorType::SUB:
            return std::to_string(-string_to_float(operand));
            
        default:
            return operand;
    }
}

bool LogicEngine::string_to_bool(const std::string& value) const {
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return !value.empty();
}

int LogicEngine::string_to_int(const std::string& value) const {
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

double LogicEngine::string_to_float(const std::string& value) const {
    try {
        return std::stod(value);
    } catch (...) {
        return 0.0;
    }
}

std::string LogicEngine::bool_to_string(bool value) const {
    return value ? "true" : "false";
}

std::string LogicEngine::int_to_string(int value) const {
    return std::to_string(value);
}

std::string LogicEngine::float_to_string(double value) const {
    return std::to_string(value);
}

// StateVariableResolver 实现
StateVariableResolver::StateVariableResolver(StateManager* manager) : state_manager(manager) {}

void StateVariableResolver::set_parameters(const std::map<std::string, std::string>& parameters) {
    params = parameters;
}

void StateVariableResolver::set_parameter(const std::string& name, const std::string& value) {
    params[name] = value;
}

std::string StateVariableResolver::resolve_variable(const std::string& name) const {
    // 处理 params.xxx 格式
    if (name.substr(0, 7) == "params.") {
        std::string param_name = name.substr(7);
        auto param_it = params.find(param_name);
        if (param_it != params.end()) {
            return param_it->second;
        }
        return "";
    }
    
    // 处理 state.xxx 格式
    if (name.substr(0, 6) == "state.") {
        std::string state_var = name.substr(6);
        if (state_manager) {
            return state_manager->get_string(state_var);
        }
        return "";
    }
    
    // 首先检查参数
    auto param_it = params.find(name);
    if (param_it != params.end()) {
        return param_it->second;
    }
    
    // 然后检查状态
    if (state_manager) {
        return state_manager->get_string(name);
    }
    
    return "";
}

void StateVariableResolver::set_variable(const std::string& name, const std::string& value) {
    // 处理 state.xxx 格式
    if (name.substr(0, 6) == "state.") {
        std::string state_var = name.substr(6);
        if (state_manager) {
            state_manager->set(state_var, value);
        }
        return;
    }
    
    // 处理 params.xxx 格式
    if (name.substr(0, 7) == "params.") {
        std::string param_name = name.substr(7);
        params[param_name] = value;
        return;
    }
    
    // 默认设置到状态管理器
    if (state_manager) {
        state_manager->set(name, value);
    }
}

bool StateVariableResolver::has_variable(const std::string& name) const {
    if (params.find(name) != params.end()) {
        return true;
    }
    
    if (state_manager) {
        return state_manager->has(name);
    }
    
    return false;
}

} // namespace cardity 