@echo off
if not exist build\CMakeCache.txt (
    cmake -B build
)
if "%1"=="build" (
    cmake --build build
    if "%2"=="run" (
        cmake --build build --target run-html
    )
    build\Debug\NovaCPP.exe
) else if "%1"=="run" (
    cmake --build build
    cmake --build build --target run-html
    build\Debug\NovaCPP.exe
) else (
    echo Usage: nova [build^|run^|build run]
)
