@echo off
if "%1"=="run" (
    cmake --build build --target run-html
) else if "%1"=="build" (
    cmake --build build
) else (
    echo Usage: nova [run^|build]
)
