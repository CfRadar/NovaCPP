# NovaCPP Toolchain & Prerequisites Reference

## Operating System Matrix

| Operating System | Recommended Compiler | Build Tool | Required System Libraries |
|---|---|---|---|
| **Windows 10 / 11 / Server** | Visual Studio 2022 (MSVC 19.44+) x64 | CMake 3.20+, MSBuild | `ws2_32.lib`, `winhttp.lib` (linked automatically) |
| **Linux (Ubuntu 22.04+, Debian 12+)** | GCC 11+ or Clang 13+ | CMake 3.20+, Make/Ninja | `libssl-dev`, `pthread`, `dl` |
| **macOS (12.0+ Monterey, Sonoma, Sequoia)** | Apple Clang 13+ (Xcode CLI Tools) | CMake 3.20+ | Default system frameworks |

---

## CMake Configuration Flags

When configuring the project directly via CMake:
```bash
# Windows Visual Studio 2022 (64-bit)
cmake -S . -B build -A x64

# Linux / macOS Ninja generator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

---

## CLI Scaffolder Flags

The `create-novacpp` CLI supports the following flags:
- `npx create-novacpp <project-name>`: Full showcase with Bento Grid, interactive components, and SQLite WAL database.
- `npx create-novacpp <project-name> --minimal`: Clean starter template under 150 lines (`Boilerplate.cpp`).
- `npx create-novacpp <project-name> --help`: Displays usage guide.
