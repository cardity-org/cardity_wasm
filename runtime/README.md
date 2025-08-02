# Cardity Runtime Module

Cardity 协议运行时模块，负责解析和执行 `.car` 协议文件。

## 模块结构

```
runtime/
├── car_loader.h/cpp      # .car 协议文件加载和解析
├── state_store.h/cpp     # 状态管理和持久化
├── logic_engine.h/cpp    # 逻辑表达式解释执行
├── runtime.h/cpp         # 主运行时接口
└── README.md            # 本文件
```

## 核心组件

### 1. CarLoader
- **功能**: 加载和解析 `.car` 协议文件
- **支持格式**: JSON、Base64
- **输出**: CarProtocol 结构体
- **验证**: 协议格式验证

### 2. StateStore
- **功能**: 状态变量管理
- **类型支持**: string、int、bool、float
- **持久化**: 文件存储和加载
- **快照**: 状态快照创建和恢复

### 3. LogicEngine
- **功能**: 逻辑表达式解释执行
- **支持操作**: 赋值、条件、算术、逻辑运算
- **变量解析**: state.xxx、params.xxx 格式
- **表达式**: 支持嵌套表达式和函数调用

### 4. CardityRuntime
- **功能**: 主运行时接口
- **协议管理**: 加载、验证、执行
- **方法调用**: 参数验证、执行、返回
- **事件系统**: 事件触发和日志
- **WASM 导出**: WebAssembly 接口

## 使用示例

```cpp
#include "runtime/runtime.h"

// 创建运行时
cardity::CardityRuntime runtime;

// 加载协议
runtime.load_protocol("hello_cardinals.car");

// 调用方法
auto result = runtime.call_method("set_msg", {"Hello World"});

// 获取状态
std::string msg = runtime.get_state("msg");
```

## 协议格式

`.car` 文件包含以下结构：

```json
{
  "p": "cardinals",
  "op": "deploy", 
  "protocol": "hello_cardinals",
  "version": "1.0",
  "cpl": {
    "state": {
      "msg": {"type": "string", "default": "Hello, Cardinals!"}
    },
    "methods": {
      "set_msg": {
        "params": ["new_msg"],
        "logic": "state.msg = params.new_msg"
      }
    },
    "events": {
      "MessageUpdated": {"params": ["new_msg"]}
    },
    "owner": "doge1abc123def456"
  }
}
```

## 编译集成

运行时模块已集成到主项目的 CMakeLists.txt 中，支持：

- **原生编译**: Linux/macOS/Windows
- **WASM 编译**: Emscripten 支持
- **头文件**: 自动包含 runtime/ 目录

## 扩展性

模块设计支持以下扩展：

- **新的状态类型**: 在 StateValue 中添加
- **新的操作符**: 在 LogicEngine 中扩展
- **新的事件类型**: 在 Runtime 中实现
- **新的存储后端**: 实现 StateStore 接口 