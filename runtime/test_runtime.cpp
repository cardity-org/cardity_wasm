#include "runtime_engine.hpp"
#include <iostream>

int main() {
    try {
        std::cout << "🧪 Testing RuntimeEngine with minimal test case..." << std::endl;
        
        RuntimeEngine engine("examples/hello_cardinals.car");

        // 初始状态检查
        std::cout << "📊 Initial state: " << engine.get_state().dump() << std::endl;

        // 调用 set_msg 方法
        std::cout << "\n🔧 Calling set_msg with 'gm, DOGE'..." << std::endl;
        std::string result = engine.invoke("set_msg", {"gm, DOGE"});
        std::cout << "✅ Set result: " << result << std::endl;

        // 调用 get_msg 方法
        std::cout << "\n🔧 Calling get_msg..." << std::endl;
        std::string msg = engine.invoke("get_msg", {});
        std::cout << "✅ Get result: " << msg << std::endl;

        // 验证状态更新
        std::cout << "\n📊 Updated state: " << engine.get_state().dump() << std::endl;

        // 验证协议信息
        std::cout << "\n📋 Protocol info:" << std::endl;
        std::cout << "  Name: " << engine.get_protocol_name() << std::endl;
        std::cout << "  Version: " << engine.get_protocol_version() << std::endl;

        // 显示可用方法
        std::cout << "\n🔧 Available methods:" << std::endl;
        auto methods = engine.get_method_names();
        for (const auto& method : methods) {
            std::cout << "  - " << method << std::endl;
        }

        std::cout << "\n🎉 All tests passed! RuntimeEngine is working correctly." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 