@echo off
if not exist build\CMakeCache.txt (
    cmake -B build
)
if exist scripts\bundle-vendor.js (
    node scripts\bundle-vendor.js
)
if "%1"=="dev" (
    node scripts\dev-watcher.js
) else if "%1"=="watch" (
    node scripts\dev-watcher.js
) else if "%1"=="build" (
    cmake --build build
    if "%2"=="run" (
        cmake --build build --target run-html
    )
    build\Debug\NovaCPP.exe
) else if "%1"=="run" (
    cmake --build build
    cmake --build build --target run-html
    build\Debug\NovaCPP.exe
) else if "%1"=="test" (
    cmake --build build
    if exist scripts\run-all-tests.py (
        python scripts\run-all-tests.py
    ) else (
        ctest --test-dir build --output-on-failure
    )
) else if "%1"=="clean" (
    echo [NovaCPP] Cleaning build artifacts...
    if exist build rmdir /s /q build
    echo [NovaCPP] Done.
) else (
    echo Usage: nova [run^|build^|test^|clean^|dev]
)
