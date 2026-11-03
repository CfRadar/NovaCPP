#!/usr/bin/env bash
# ==============================================================================
# NovaCPP Cross-Platform Developer Command Runner
# ==============================================================================
set -e

BUILD_DIR="build"
BUILD_TYPE="Debug"

# Determine core count for parallel builds
if command -v nproc > /dev/null 2>&1; then
    CORES=$(nproc)
elif command -v sysctl > /dev/null 2>&1; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi

COMMAND=${1:-run}

configure_project() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "[NovaCPP] Configuring build directory ($BUILD_TYPE)..."
        cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    fi
}

bundle_vendor() {
    if [ -f "scripts/bundle-vendor.js" ] && command -v node > /dev/null 2>&1; then
        echo "[NovaCPP] Bundling vendor assets..."
        node scripts/bundle-vendor.js
    fi
}

build_project() {
    bundle_vendor
    configure_project
    echo "[NovaCPP] Compiling binary with $CORES jobs..."
    cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$CORES"
}

case "$COMMAND" in
    build)
        build_project
        echo "[NovaCPP] Build completed successfully."
        ;;
    run)
        build_project
        echo "[NovaCPP] Starting application server..."
        if [ -f "$BUILD_DIR/NovaCPP" ]; then
            exec "$BUILD_DIR/NovaCPP"
        elif [ -f "$BUILD_DIR/$BUILD_TYPE/NovaCPP" ]; then
            exec "$BUILD_DIR/$BUILD_TYPE/NovaCPP"
        elif [ -f "$BUILD_DIR/$BUILD_TYPE/NovaCPP.exe" ]; then
            exec "$BUILD_DIR/$BUILD_TYPE/NovaCPP.exe"
        elif [ -f "$BUILD_DIR/NovaCPP.exe" ]; then
            exec "$BUILD_DIR/NovaCPP.exe"
        else
            echo "[NovaCPP Error] Executable not found in $BUILD_DIR"
            exit 1
        fi
        ;;
    test)
        build_project
        echo "[NovaCPP] Executing test runner..."
        if [ -f "scripts/run-all-tests.py" ]; then
            python3 scripts/run-all-tests.py
        elif command -v python > /dev/null 2>&1 && [ -f "scripts/run-all-tests.py" ]; then
            python scripts/run-all-tests.py
        else
            ctest --test-dir "$BUILD_DIR" --output-on-failure
        fi
        ;;
    clean)
        echo "[NovaCPP] Cleaning build artifacts..."
        rm -rf "$BUILD_DIR"
        echo "[NovaCPP] Done."
        ;;
    dev|watch)
        bundle_vendor
        if [ -f "scripts/dev-watcher.js" ] && command -v node > /dev/null 2>&1; then
            node scripts/dev-watcher.js
        else
            build_project
            exec "$BUILD_DIR/NovaCPP"
        fi
        ;;
    *)
        echo "Usage: ./nova.sh {run|build|test|clean|dev}"
        exit 1
        ;;
esac
