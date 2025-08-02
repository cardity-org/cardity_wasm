# Cardity WASM Runtime

Cardity WASM Runtime 是一个完整的 Cardity 协议运行时环境，以 WebAssembly 形式运行，能在任何平台解析 .car 协议文件，实现 UTXO 上的「智能协议」行为。

## 🏗️ 项目结构

```
cardity_wasm/
├── include/                 # 头文件
│   ├── car_loader.h        # .car 文件加载器
│   ├── state_store.h       # 状态存储器
│   ├── logic_engine.h      # 逻辑引擎
│   └── runtime.h           # 主运行时
├── src/                    # 源文件
│   ├── car_loader.cpp      # .car 文件加载器实现
│   ├── state_store.cpp     # 状态存储器实现
│   ├── logic_engine.cpp    # 逻辑引擎实现
│   └── runtime.cpp         # 主运行时实现
├── test_data/              # 测试数据
│   └── hello_cardinals.car # 示例协议文件
├── dist/                   # 输出目录（WASM 文件）
├── main.cpp                # 主程序入口
├── CMakeLists.txt          # 构建配置
└── README.md               # 项目说明
```

## 🚀 快速开始

### 原生编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make

# 运行测试
./cardity_wasm ../test_data/hello_cardinals.car call set_msg "Hello World"
./cardity_wasm ../test_data/hello_cardinals.car call get_msg
./cardity_wasm ../test_data/hello_cardinals.car state
```

### WASM 编译

```bash
# 使用 Emscripten 编译
emcmake cmake ..
emmake make

# 输出文件在 dist/ 目录
# - cardity_runtime.js
# - cardity_runtime.wasm
```

## 📖 使用示例

### 加载协议

```cpp
#include "runtime.h"

using namespace cardity;

// 创建运行时
CardityRuntime runtime;

// 加载协议
if (runtime.load_protocol("hello_cardinals.car")) {
    std::cout << "Protocol loaded: " << runtime.get_protocol_name() << std::endl;
}
```

### 调用方法

```cpp
// 调用方法
std::vector<std::string> args = {"Hello World"};
MethodResult result = runtime.call_method("set_msg", args);

if (result.success) {
    std::cout << "Method executed successfully" << std::endl;
    
    // 显示事件
    for (const auto& event : result.events) {
        std::cout << "Event: " << event.name << std::endl;
    }
}
```

### 状态管理

```cpp
// 设置状态
runtime.set_state("msg", "New message");

// 获取状态
std::string value = runtime.get_state("msg");

// 获取所有状态
json state = runtime.get_all_state();
```

### 快照管理

```cpp
// 创建快照
Snapshot snapshot = runtime.create_snapshot();

// 保存快照
runtime.save_snapshot_to_file("snapshot.json");

// 恢复快照
runtime.load_snapshot_from_file("snapshot.json");
```

## 🌐 Web 集成

### JavaScript 使用

```javascript
// 加载 WASM 模块
const CardityModule = require('./dist/cardity_runtime.js');

CardityModule().then((Module) => {
    // 创建运行时
    const runtime = Module._create_runtime();
    
    // 加载协议
    const carData = '{"p":"cardinals","op":"deploy",...}';
    Module._load_protocol(runtime, carData);
    
    // 调用方法
    const args = JSON.stringify(["Hello World"]);
    const result = Module._call_method(runtime, "set_msg", args);
    
    // 获取状态
    const state = Module._get_state(runtime, "msg");
    
    // 销毁运行时
    Module._destroy_runtime(runtime);
});
```

### HTML 集成

```html
<!DOCTYPE html>
<html>
<head>
    <title>Cardity Runtime Demo</title>
</head>
<body>
    <script src="dist/cardity_runtime.js"></script>
    <script>
        CardityModule().then((Module) => {
            console.log("Cardity Runtime loaded!");
            
            // 使用运行时
            const runtime = Module._create_runtime();
            
            // 加载协议并执行方法
            const carData = '{"p":"cardinals","op":"deploy",...}';
            Module._load_protocol(runtime, carData);
            
            const result = Module._call_method(runtime, "set_msg", JSON.stringify(["Hello from Web!"]));
            console.log("Result:", result);
            
            Module._destroy_runtime(runtime);
        });
    </script>
</body>
</html>
```

## 🔧 命令行工具

```bash
# 显示帮助
./cardity_wasm

# 查看可用方法
./cardity_wasm hello_cardinals.car

# 调用方法
./cardity_wasm hello_cardinals.car call set_msg "Hello World"
./cardity_wasm hello_cardinals.car call get_msg
./cardity_wasm hello_cardinals.car call increment

# 查看状态
./cardity_wasm hello_cardinals.car state

# 查看事件日志
./cardity_wasm hello_cardinals.car events

# 查看 ABI
./cardity_wasm hello_cardinals.car abi

# 创建快照
./cardity_wasm hello_cardinals.car snapshot
```

## 📋 支持的协议特性

- ✅ 状态变量管理（string, int, bool, float）
- ✅ 方法调用与参数传递
- ✅ 事件系统
- ✅ 表达式求值
- ✅ 条件逻辑
- ✅ 快照与持久化
- ✅ ABI 接口生成
- ✅ 跨平台运行（原生 + WASM）

## 🔗 与主项目联动

本项目与 [cardity-core](https://github.com/cardity-org/cardity-core) 项目联动：

- 使用相同的 .car 文件格式
- 兼容主项目的协议定义
- 支持主项目生成的部署包
- 可运行主项目的示例协议

## �� 许可证

MIT License 