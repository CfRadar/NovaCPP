# NovaCPP Rule: nova-build-and-debug

# Building & Debugging NovaCPP Applications

<!-- symbols: renderComponent, isAddressOnCurrentStack, getEnvOr -->

## When to use
Use this skill when diagnosing compiler errors, unresolved linker symbols, compiler warning mitigation, stack guard exceptions, or runtime crashes in a NovaCPP project.

## Prerequisites
- A NovaCPP project configured with CMake.
- C++17 compiler toolchain.

## Quick start
Run the automated framework test suite to diagnose runtime issues:
```bash
nova.bat test       # Windows
./nova.sh test      # Linux / macOS
```

## Top 10 Diagnostic Error Matrix
| Diagnostic / Error Code | Root Cause | Verified Solution |
|---|---|---|
| **C4996 (MSVC)**: `'std::getenv': This function or variable may be unsafe` | Use of deprecated C-runtime environment accessor | Include `novacpp/platform.hpp` and replace `std::getenv` with `np::getEnvOr(key, fallback)` or `np::hasEnv(key)`. |
| **LNK2019 / LNK2001**: `unresolved external symbol __imp_WSAStartup` | Missing Windows Sockets library on MSVC | Add `ws2_32.lib` to `target_link_libraries(NovaCPP PRIVATE ws2_32)` in `CMakeLists.txt`. |
| **C4002 / C2059**: `too many arguments for function-like macro invocation 'min'` | Windows `<windows.h>` defines `min` and `max` macros colliding with `std::min` | Include `novacpp/platform.hpp` before system headers or define `#define NOMINMAX`. Wrap calls with parenthesis: `(std::min)({...})`. |
| **std::logic_error**: `Component allocated on stack` | UI component declared as local variable in route callback | Never instantiate components on the local function stack. Use static lifetime (`static MyComp g_comp;`) or heap shared pointers (`std::make_shared<MyComp>()`). |
| **LNK2005**: `symbol already defined in obj` | Non-inline function defined in header included across multiple compilation units | Add `inline` keyword to function definition or move implementation to a `.cpp` file. |
| **404 Not Found on Deep Link Reload** | Single Page Application client-side route reloaded directly without edge rewrite | Ensure `vercel.json` contains SPA rewrite rule (`"source": "/(.*)", "destination": "/index.html"`). |
| **SQLITE_BUSY / database is locked** | High concurrency write contention in default rollback journal mode | Ensure SQLite is initialized in WAL mode: `PRAGMA journal_mode = WAL; PRAGMA busy_timeout = 5000;`. |
| **Broken Form Payload (Empty map)** | Client POST lacked proper `Content-Type` header | Send `Content-Type: application/json` or `application/x-www-form-urlencoded` with matching payload structure. |
| **C2678 / C2679**: `binary '<<': no operator found which takes a right-hand operand` | Stream insertion operand does not support `operator<<` | Convert type to `std::string` via `std::to_string()` or serialize to JSON before streaming to `NovaBuilder`. |
| **WinHTTP 12029 / Connection Refused** | Outbound HTTP request to offline host or invalid port | Check remote service status and wrap `np::fetch` calls with error checks or timeout bounds. |

## Rules
- MUST compile with C++17 standard enabled (`set(CMAKE_CXX_STANDARD 17)`).
- MUST define `NOMINMAX` and `_CRT_SECURE_NO_WARNINGS` before Windows headers (handled automatically by `novacpp/platform.hpp`).
- NEVER suppress compiler warnings globally with `/w`; use targeted suppression `/wd4996` with safe abstractions.
- NEVER ignore `std::logic_error` stack guard exceptions; they protect against memory corruption and use-after-free bugs.

## Verify
To verify build health with clean warnings:
```bash
cmake -B build -A x64
cmake --build build --config Release
```
Expect zero errors and zero unhandled warnings.

## Go deeper
- [`references/troubleshooting-matrix.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-build-and-debug/references/troubleshooting-matrix.md): In-depth guide to memory sanitizers, MSVC compiler flags, and crash dump analysis.
