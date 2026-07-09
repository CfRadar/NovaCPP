@echo off
if "%1"=="build" (
    cmake --build build
    build\Debug\NovaCPP.exe
    if "%2"=="run" (
        cmake --build build --target run-html
    )
) else if "%1"=="run" (
    cmake --build build
    build\Debug\NovaCPP.exe
    cmake --build build --target run-html
) else (
    echo Usage: nova [build^|run^|build run]
)
