cmake_minimum_required(VERSION 3.16)
project(cardity_wasm VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找必要的包
find_package(nlohmann_json REQUIRED)

# 包含目录
include_directories(runtime)

# 源文件
set(SOURCES
    runtime/car_loader.cpp
    runtime/state_store.cpp
    runtime/logic_engine.cpp
    runtime/runtime.cpp
    main.cpp
)

# 头文件
set(HEADERS
    runtime/car_loader.h
    runtime/state_store.h
    runtime/logic_engine.h
    runtime/runtime.h
)

# 创建主可执行文件（原生编译）
add_executable(cardity_wasm ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(cardity_wasm nlohmann_json::nlohmann_json)

# 检查是否支持 Emscripten
if(EMSCRIPTEN)
    # WASM 编译配置
    set(EMSCRIPTEN_FLAGS
        -s WASM=1
        -s MODULARIZE=1
        -s EXPORT_NAME="CardityModule"
        -s EXPORTED_FUNCTIONS="['_create_runtime','_destroy_runtime','_load_protocol','_call_method','_get_state','_set_state','_get_event_log','_create_snapshot','_get_abi']"
        -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"
        -s ALLOW_MEMORY_GROWTH=1
        -s INITIAL_MEMORY=16777216
        -s MAXIMUM_MEMORY=268435456
        -O3
    )
    
    # 创建 WASM 目标
    add_executable(cardity_runtime_wasm ${SOURCES} ${HEADERS})
    target_link_libraries(cardity_runtime_wasm nlohmann_json::nlohmann_json)
    
    # 设置 WASM 编译选项
    set_target_properties(cardity_runtime_wasm PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "${EMSCRIPTEN_FLAGS}"
    )
    
    # 复制输出文件到 dist 目录
    add_custom_command(TARGET cardity_runtime_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cardity_runtime_wasm> ${CMAKE_BINARY_DIR}/dist/cardity_runtime.js
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cardity_runtime_wasm>.wasm ${CMAKE_BINARY_DIR}/dist/cardity_runtime.wasm
    )
endif()

# 创建测试可执行文件
add_executable(cardity_test main.cpp ${SOURCES} ${HEADERS})
target_link_libraries(cardity_test nlohmann_json::nlohmann_json)

# 安装规则
install(TARGETS cardity_wasm DESTINATION bin)

if(EMSCRIPTEN)
    install(FILES ${CMAKE_BINARY_DIR}/dist/cardity_runtime.js DESTINATION share/cardity_wasm)
    install(FILES ${CMAKE_BINARY_DIR}/dist/cardity_runtime.wasm DESTINATION share/cardity_wasm)
endif()

# 创建 dist 目录
add_custom_target(create_dist_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
)

# 依赖关系
if(EMSCRIPTEN)
    add_dependencies(cardity_runtime_wasm create_dist_dir)
endif() 