#!/bin/bash

set -e

echo "🔧 Cardity WASM Runtime Builder"
echo "================================"

# 检查参数
BUILD_TYPE=${1:-native}
BUILD_DIR="build"

case $BUILD_TYPE in
    "native")
        echo "🏗️  Building native version..."
        
        # 创建构建目录
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        
        # 配置和编译
        cmake ..
        make -j$(sysctl -n hw.ncpu)
        
        echo "✅ Native build completed!"
        echo "📦 Executable: $BUILD_DIR/cardity_wasm"
        ;;
        
    "wasm")
        echo "🌐 Building WASM version..."
        
        # 检查 Emscripten
        if ! command -v emcc &> /dev/null; then
            echo "❌ Emscripten not found. Please install Emscripten first."
            echo "   Visit: https://emscripten.org/docs/getting_started/downloads.html"
            exit 1
        fi
        
        # 创建构建目录
        mkdir -p $BUILD_DIR
        cd $BUILD_DIR
        
        # 配置和编译
        emcmake cmake ..
        emmake make -j$(sysctl -n hw.ncpu)
        
        echo "✅ WASM build completed!"
        echo "📦 Output files:"
        echo "   - $BUILD_DIR/dist/cardity_runtime.js"
        echo "   - $BUILD_DIR/dist/cardity_runtime.wasm"
        ;;
        
    "clean")
        echo "🧹 Cleaning build directory..."
        rm -rf $BUILD_DIR
        echo "✅ Clean completed!"
        ;;
        
    "test")
        echo "🧪 Running tests..."
        
        if [ ! -f "$BUILD_DIR/cardity_wasm" ]; then
            echo "❌ Executable not found. Please build first: ./build.sh native"
            exit 1
        fi
        
        # 运行测试
        echo "Testing hello_cardinals protocol..."
        $BUILD_DIR/cardity_wasm test_data/hello_cardinals.car call set_msg "Test Message"
        $BUILD_DIR/cardity_wasm test_data/hello_cardinals.car call get_msg
        $BUILD_DIR/cardity_wasm test_data/hello_cardinals.car call increment
        $BUILD_DIR/cardity_wasm test_data/hello_cardinals.car state
        $BUILD_DIR/cardity_wasm test_data/hello_cardinals.car events
        
        echo "✅ Tests completed!"
        ;;
        
    "all")
        echo "🚀 Building all versions..."
        ./build.sh clean
        ./build.sh native
        ./build.sh wasm
        ./build.sh test
        echo "✅ All builds completed!"
        ;;
        
    *)
        echo "Usage: $0 {native|wasm|clean|test|all}"
        echo ""
        echo "Build types:"
        echo "  native  - Build native executable (default)"
        echo "  wasm    - Build WebAssembly version"
        echo "  clean   - Clean build directory"
        echo "  test    - Run tests"
        echo "  all     - Build all versions and run tests"
        exit 1
        ;;
esac 