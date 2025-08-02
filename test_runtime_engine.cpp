#include "runtime/runtime_engine.hpp"
#include <iostream>

int main() {
    try {
        std::cout << "🧪 Testing RuntimeEngine..." << std::endl;
        
        // 创建运行时引擎
        RuntimeEngine engine("test_data/hello_cardinals.car");
        
        std::cout << "✅ Protocol loaded: " << engine.get_protocol_name() 
                  << " v" << engine.get_protocol_version() << std::endl;
        
        // 显示初始状态
        std::cout << "📊 Initial state:" << std::endl;
        auto state = engine.get_state();
        for (const auto& [key, value] : state.items()) {
            std::cout << "  " << key << ": " << value << std::endl;
        }
        
        // 显示可用方法
        std::cout << "🔧 Available methods:" << std::endl;
        auto methods = engine.get_method_names();
        for (const auto& method : methods) {
            std::cout << "  - " << method << std::endl;
        }
        
        // 测试 set_msg 方法
        std::cout << "\n🧪 Testing set_msg method..." << std::endl;
        std::string result = engine.invoke("set_msg", {"Hello from RuntimeEngine!"});
        std::cout << "✅ Result: " << result << std::endl;
        
        // 测试 get_msg 方法
        std::cout << "\n🧪 Testing get_msg method..." << std::endl;
        result = engine.invoke("get_msg", {});
        std::cout << "✅ Result: " << result << std::endl;
        
        // 显示更新后的状态
        std::cout << "\n📊 Updated state:" << std::endl;
        state = engine.get_state();
        for (const auto& [key, value] : state.items()) {
            std::cout << "  " << key << ": " << value << std::endl;
        }
        
        // 测试 increment 方法
        std::cout << "\n🧪 Testing increment method..." << std::endl;
        result = engine.invoke("increment", {});
        std::cout << "✅ Result: " << result << std::endl;
        
        // 测试 get_count 方法
        std::cout << "\n🧪 Testing get_count method..." << std::endl;
        result = engine.invoke("get_count", {});
        std::cout << "✅ Result: " << result << std::endl;
        
        std::cout << "\n🎉 All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 