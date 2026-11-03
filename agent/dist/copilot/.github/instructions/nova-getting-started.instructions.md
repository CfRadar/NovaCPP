---
applyTo: "**/*.cpp,**/*.hpp"
---

# Getting Started with NovaCPP

<!-- symbols: NovaBuilder, RenderContext, loadEnv -->

## When to use
Use this skill when scaffolding a new application, booting a starter template, or verifying that the local C++ toolchain and HTTP server are functional.

## Prerequisites
- C++17 compiler installed:
  - Windows: Visual Studio 2022 (MSVC 19.44+) with "Desktop development with C++".
  - Linux: GCC 11+ or Clang 13+ with `build-essential`.
  - macOS: Xcode Command Line Tools (`clang++`).
- CMake 3.20 or newer.
- Node.js 18+ (for the `create-novacpp` CLI scaffolder).

## Quick start
```cpp
#include "novacpp/nova.hpp"

void renderHome(np::RenderContext& np) {
    np << "<div class='bento-grid'><div class='bento-card col-12'><h1>Hello NovaCPP</h1></div></div>";
}

int main() {
    np::NovaBuilder app;
    app.route("/", renderHome);
    app.listen(8080);
    return 0;
}
```
Full compilable source available in [`assets/main_hello.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-getting-started/assets/main_hello.cpp).

## Steps
1. **Scaffold the Application**:
   Run the CLI to generate a new application from the starter template:
   ```bash
   npx create-novacpp my-app
   cd my-app
   ```
   For a minimal starting template under 150 lines, add `--minimal`:
   ```bash
   npx create-novacpp my-app --minimal
   ```
2. **Review Project Layout**:
   - `src/main.cpp`: Backend HTTP server entrypoint, environment loader, and route registration.
   - `src/frontend/App.cpp`: UI layout views, component instances, and event bindings.
   - `src/backend/Database.cpp`: SQLite connection pool and schema migrations.
   - `.env`: Local configuration file (copied automatically from `.env.example`).
3. **Build and Run the Native Binary**:
   - Windows:
     ```cmd
     nova.bat run
     ```
   - Linux / macOS:
     ```bash
     ./nova.sh run
     ```
   - Direct CMake:
     ```bash
     cmake -B build -A x64
     cmake --build build --config Release
     ./build/Release/NovaCPP.exe
     ```
4. **Verify Server Health**:
   Open a browser to `http://localhost:8080/` or send a probe request:
   ```bash
   curl -i http://localhost:8080/
   ```
   Expect `HTTP/1.1 200 OK` with `Content-Type: text/html`.

## Rules
- MUST run `np::loadEnv(".env");` before reading environment variables.
- MUST register all routes before invoking `app.listen(port);`.
- NEVER use raw `std::getenv()`; use `np::getEnvOr(key, fallback)` from `novacpp/platform.hpp`.
- NEVER instantiate UI components on the local function stack inside route callbacks.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Server boots but yields 404 for `/` | Route registered after `app.listen()` | Register all routes before calling `app.listen(port)` |
| `CMake Error: Could not find toolchain` | Missing Visual Studio C++ workload or GCC | Install VS 2022 C++ workload or `apt install build-essential` |
| `fatal error: winsock2.h` on Windows | Missing platform definitions | Include `novacpp/platform.hpp` or `novacpp/nova.hpp` before system headers |
| Port 8080 already in use | Another process is holding the port | Specify `PORT=9090` in `.env` or run `set PORT=9090 && nova.bat run` |

## Verify
Execute the build and healthcheck probe:
```bash
curl -i http://localhost:8080/api/ping
```
Expect HTTP 200 with response payload `pong`.

## Go deeper
- [`references/prerequisites.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-getting-started/references/prerequisites.md): Detailed toolchain, operating system requirements, and CMake generator flags.
