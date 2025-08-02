#include <iostream>
#include <string>
#include <vector>
#include "runtime.h"

using namespace cardity;

void print_usage(const std::string& program_name) {
    std::cout << "Cardity WASM Runtime" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Usage: " << program_name << " <car_file> [--state <state_file>] [command] [args...]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --state <file>           - Use persistent state file" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  call <method> [args...]  - Call a method" << std::endl;
    std::cout << "  get <key>                - Get state value" << std::endl;
    std::cout << "  set <key> <value>        - Set state value" << std::endl;
    std::cout << "  events                   - Show event log" << std::endl;
    std::cout << "  state                    - Show all state" << std::endl;
    std::cout << "  abi                      - Show ABI" << std::endl;
    std::cout << "  snapshot                 - Create snapshot" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " hello.car --state hello.state call set_msg \"Hello World\"" << std::endl;
    std::cout << "  " << program_name << " hello.car --state hello.state call get_msg" << std::endl;
    std::cout << "  " << program_name << " hello.car --state hello.state call increment" << std::endl;
    std::cout << "  " << program_name << " hello.car --state hello.state state" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string car_file = argv[1];
    std::string state_file = "";
    int arg_offset = 1;
    
    // 解析选项
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--state" && i + 1 < argc) {
            state_file = argv[i + 1];
            arg_offset = i + 2;
            break;
        }
    }
    
    try {
        // 创建运行时
        std::cout << "🚀 Initializing Cardity WASM Runtime..." << std::endl;
        CardityRuntime runtime;
        
        // 加载协议
        std::cout << "📖 Loading protocol: " << car_file << std::endl;
        if (!runtime.load_protocol(car_file)) {
            std::cerr << "❌ Failed to load protocol" << std::endl;
            return 1;
        }
        
        std::cout << "✅ Protocol loaded: " << runtime.get_protocol_name() 
                  << " v" << runtime.get_protocol_version() << std::endl;
        
        // 加载状态文件（如果指定）
        if (!state_file.empty()) {
            std::cout << "📁 Loading state from: " << state_file << std::endl;
            if (runtime.load_state_from_file(state_file)) {
                std::cout << "✅ State loaded from file" << std::endl;
            } else {
                std::cout << "ℹ️  No existing state file, starting fresh" << std::endl;
            }
        }
        
        // 如果没有命令，显示帮助
        if (argc < arg_offset + 1) {
            std::cout << "\nAvailable methods:" << std::endl;
            auto methods = runtime.get_method_names();
            for (const auto& method : methods) {
                std::cout << "  - " << method << std::endl;
            }
            std::cout << "\nUse: " << argv[0] << " " << car_file << " [--state <file>] call <method> [args...]" << std::endl;
            return 0;
        }
        
        std::string command = argv[arg_offset];
        
        if (command == "call" && argc >= arg_offset + 2) {
            std::string method_name = argv[arg_offset + 1];
            std::vector<std::string> args;
            
            // 收集参数
            for (int i = arg_offset + 2; i < argc; ++i) {
                args.push_back(argv[i]);
            }
            
            std::cout << "🔧 Calling method: " << method_name;
            if (!args.empty()) {
                std::cout << " with args: [";
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << args[i];
                }
                std::cout << "]";
            }
            std::cout << std::endl;
            
            // 调用方法
            MethodResult result = runtime.call_method(method_name, args);
            
            if (result.success) {
                std::cout << "✅ Method executed successfully" << std::endl;
                if (!result.return_value.empty()) {
                    std::cout << "📥 Return value: " << result.return_value << std::endl;
                }
                
                // 显示事件
                if (!result.events.empty()) {
                    std::cout << "📢 Events emitted:" << std::endl;
                    for (const auto& event : result.events) {
                        std::cout << "  - " << event.name << "(";
                        for (size_t i = 0; i < event.values.size(); ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << event.values[i];
                        }
                        std::cout << ")" << std::endl;
                    }
                }
            } else {
                std::cout << "❌ Method execution failed: " << result.error_message << std::endl;
                return 1;
            }
            
            // 保存状态（如果指定了状态文件）
            if (!state_file.empty()) {
                runtime.save_state_to_file(state_file);
            }
            
        } else if (command == "get" && argc >= arg_offset + 2) {
            std::string key = argv[arg_offset + 1];
            std::string value = runtime.get_state(key);
            std::cout << "📥 " << key << ": " << value << std::endl;
            
        } else if (command == "set" && argc >= arg_offset + 3) {
            std::string key = argv[arg_offset + 1];
            std::string value = argv[arg_offset + 2];
            if (runtime.set_state(key, value)) {
                std::cout << "✅ Set " << key << " = " << value << std::endl;
                // 保存状态（如果指定了状态文件）
                if (!state_file.empty()) {
                    runtime.save_state_to_file(state_file);
                }
            } else {
                std::cout << "❌ Failed to set " << key << std::endl;
                return 1;
            }
            
        } else if (command == "events") {
            auto events = runtime.get_event_log();
            if (events.empty()) {
                std::cout << "📢 No events in log" << std::endl;
            } else {
                std::cout << "📢 Event log:" << std::endl;
                for (const auto& event : events) {
                    std::cout << "  - " << event.name << "(";
                    for (size_t i = 0; i < event.values.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << event.values[i];
                    }
                    std::cout << ") at " << event.timestamp << std::endl;
                }
            }
            
        } else if (command == "state") {
            auto state = runtime.get_all_state();
            std::cout << "🔁 Current state:" << std::endl;
            for (const auto& [key, value] : state.items()) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
            
        } else if (command == "abi") {
            auto abi = runtime.get_abi();
            std::cout << "📋 ABI:" << std::endl;
            std::cout << abi.dump(2) << std::endl;
            
        } else if (command == "snapshot") {
            auto snapshot = runtime.create_snapshot();
            std::cout << "📸 Snapshot created:" << std::endl;
            std::cout << "  Protocol: " << snapshot.protocol_name << " v" << snapshot.version << std::endl;
            std::cout << "  Timestamp: " << snapshot.timestamp << std::endl;
            std::cout << "  State variables: " << snapshot.state.size() << std::endl;
            std::cout << "  Events: " << snapshot.event_log.size() << std::endl;
            
        } else {
            std::cout << "❌ Unknown command: " << command << std::endl;
            print_usage(argv[0]);
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
} 