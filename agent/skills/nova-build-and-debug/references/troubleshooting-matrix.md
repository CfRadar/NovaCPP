# NovaCPP Debugging & Troubleshooting Reference

## Compiler Flags Reference

### MSVC (Microsoft Visual Studio 2022)
- `/std:c++17`: Enables C++17 standard features.
- `/W4`: Warning level 4 for rigorous code quality.
- `/wd4996`: Suppress deprecation warnings for C-runtime functions that have been safely wrapped in `novacpp/platform.hpp`.
- `/EHsc`: Standard C++ exception handling.
- `/Zs`: Syntax-only compilation check (ideal for rapid CI validation).

### GCC / Clang
- `-std=c++17`: Enables C++17 standard.
- `-Wall -Wextra`: Strict warning checking.
- `-fsyntax-only`: Checks syntax without assembling or linking.
- `-fsanitize=address,undefined`: Enables AddressSanitizer and UndefinedBehaviorSanitizer for detecting memory leaks and out-of-bounds access.

---

## Memory Model & Lifetime Invariants

NovaCPP uses a unique pointer-based component model:
1. `ServerContext::components` stores raw non-owning pointers to `np::Component` instances.
2. When a surgical request arrives (`X-Nova-Target: comp-id`), the server looks up the pointer and calls `comp->render(partialBuilder)`.
3. If the component was allocated on the stack inside a route callback, its memory is freed as soon as the initial HTTP request finishes. Subsequent requests targeting that component would encounter a **use-after-free** access violation.
4. NovaCPP includes runtime **Stack Guards** (`isAddressOnCurrentStack`) that immediately throw `std::logic_error` if a local stack variable is registered, catching bugs before they manifest in production.

