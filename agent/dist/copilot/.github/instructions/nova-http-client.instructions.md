---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Outbound Native HTTP Client (`np::fetch`)

<!-- symbols: FetchResponse, fetch, get, post -->

## When to use
Use this skill when calling third-party REST APIs, fetching remote JSON payloads, issuing webhooks, or calling external LLM providers from the NovaCPP backend server.

## Prerequisites
- Include header `novacpp/fetch.hpp` or umbrella header `novacpp/nova.hpp`.
- On Windows: automatic link to `winhttp.lib`.
- On Linux/macOS: OpenSSL dependencies (`libssl-dev`).

## Quick start
```cpp
#include "novacpp/nova.hpp"

void fetchExternalUserData() {
    // 1. Simple GET with map headers
    np::FetchResponse res = np::get("https://jsonplaceholder.typicode.com/users/1", {
        {"Accept", "application/json"}
    });

    if (res.isOk()) {
        std::string name = res.jsonField("name");
        std::string email = res.jsonField("email");
        std::cout << "Fetched: " << name << " (" << email << ")" << std::endl;
    }
}
```
Full compilable source available in [`assets/fetch_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-http-client/assets/fetch_example.cpp).

## Steps
1. **Executing a GET Request**:
   Use `np::get(url, headers)` or `np::fetch(url, headers)`:
   ```cpp
   np::FetchResponse res = np::get("https://api.github.com/repos/CfRadar/NovaCPP", {
       {"User-Agent", "NovaCPP-App"}
   });
   ```
2. **Executing a POST Request with JSON Body**:
   ```cpp
   std::string body = R"({"title":"NovaCPP Task","completed":false})";
   np::FetchResponse res = np::post(
       "https://jsonplaceholder.typicode.com/todos",
       body,
       {{"Content-Type", "application/json"}}
   );
   ```
3. **Inspect Response Status & Headers**:
   ```cpp
   if (res.status == 201) {
       std::string newId = res.jsonField("id");
   }
   ```
4. **Convenience Verbs Available**:
   - `np::get(url, headers, timeoutMs)`
   - `np::post(url, body, headers, timeoutMs)`
   - `np::put(url, body, headers, timeoutMs)`
   - `np::del(url, headers, timeoutMs)`
5. **Non-Blocking Execution Invariant**:
   Never perform slow network I/O synchronously inside a UI render or request handler thread. Enqueue the fetch call onto the worker pool:
   ```cpp
   np.onClick("sync_remote_data", []() {
       np::WorkerPool::defaultPool().enqueue([]() {
           np::FetchResponse r = np::get("https://api.example.com/sync");
           // Process result in worker thread...
       });
   });
   ```

## Rules
- MUST check `res.isOk()` (status >= 200 && status < 300) before parsing response body fields.
- MUST offload external network calls to `np::WorkerPool` to prevent stalling the main HTTP server thread pool.
- NEVER capture `httplib::Request&` or `Response&` into worker pool async lambdas.
- NEVER hardcode API keys or secret tokens; read via `np::getEnvOr("API_KEY", "")`.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Server freezes while fetching external API | `np::fetch` called synchronously on HTTP thread | Enqueue fetch task onto `np::WorkerPool::defaultPool()` |
| `WinHTTP SendRequest failed (12007)` | Invalid domain name or offline connection | Check domain spelling, internet connection, and DNS |
| `jsonField` returns empty string | Field name mismatch or nested JSON structure | Check JSON payload structure or parse top-level fields |

## Verify
Execute an outbound test query:
```bash
nova.bat test
```
The test suite validates outbound WinHTTP and OpenSSL engines via `simulate_fetch.exe`.

## Go deeper
- [`references/fetch-api.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-http-client/references/fetch-api.md): Complete method signatures for `FetchResponse`, timeouts, and SSL certificate verification.
